/*
 * WS2812.c
 *
 *  Created on: Apr 24, 2024
 *      Author: Chase Vara
 */
#include "stm32f4xx_hal.h"
#include "WS2812.h"
#include <string.h>

// Helper function prototype declarations

/** @brief	Helper function to ransformsinput 24-bit RGB color data into an output
  * 				array of PWM timer settings for duty cycle modulation to drive WS2812B
  * 				24-bit color setting
  *
  * @param  color_data -- 24-bit RGB color data to transform into GRB color data PWM
  * 											modulation settings for DMA transfer to PWM timer.
  * @param  PWM_DMAhalfbuff -- pointer to the half of the circular DMA double buffer to
  * 													 populate with new PWM modulation setting data
  *
  * @detail	called by update_DMA_halfbuff()
  */
static void color24bit_to_ccr16bit_array(RGB_color_24bit color_data,
																				 uint16_t *PWM_DMAhalfbuff);

// Function that updates circular DMA half buffer with color data for the next LED to be updated
/** @brief	Helper function that updates circular DMA half buffer with color data
  * 				PWM modulation settings the next LED to transfer data to.
  *
  * @param  PWM_DMAhalfbuff -- pointer to the half of the circular DMA double buffer to
  * 													 populate with new PWM modulation setting data
  *
  * @detail	called by HAL DMA callback functions.
  */
static void update_DMA_halfbuff(uint16_t *pDMA_halfbuff);

// globals variables

// LEDs, color data and DMA buffer arrays
RGB_color_24bit WS2812_RGBdata_arr[WS2812_NUM_LEDS] = {0};
uint16_t WS2812_DMAdoublebuff[WS2812_DMA_BUFF_SIZE] = {0};
uint8_t WS2812_PWM_xfer_status = WS2812_XFER_RGB_READY;
uint8_t WS2812_reset_period_count = 0;
uint16_t WS2812_PWM_BIT_HI = 0;
uint16_t WS2812_PWM_BIT_LOW = 0;
uint32_t WS2812_RGB_ind = 0;

// peripheral handles
DMA_HandleTypeDef *WS2812_hdma = NULL;
TIM_HandleTypeDef *WS2812_htim = NULL;
uint32_t WS2812_tim_ch = 0;

void WS2812_init(TIM_HandleTypeDef *htim,
								 DMA_HandleTypeDef *hdma,
								 uint32_t timer_channel)
{
	// Peripheral handles
	WS2812_hdma = hdma;
	WS2812_htim = htim;
	WS2812_tim_ch = timer_channel;

  // PWM dutycycle variables
  WS2812_PWM_BIT_HI = WS2812_PWM_HI_DUTY * htim->Instance->ARR;
  WS2812_PWM_BIT_LOW = WS2812_PWM_LOW_DUTY * htim->Instance->ARR;

	// DMA double buffer initialization
	memset(WS2812_DMAdoublebuff, 0, WS2812_RGB_DATA_BIT_LEN * 2 * sizeof(uint16_t));

	// RGB data array initialization
	memset(WS2812_RGBdata_arr, 0, WS2812_NUM_LEDS * sizeof(RGB_color_24bit));

	// RGB array index
	WS2812_RGB_ind = 0;

	// status flags/counters
	WS2812_reset_period_count = 0;
}

void WS2812_set_pixel(uint32_t pixel_idx,
											uint8_t brightness,
											uint8_t red,
											uint8_t green,
											uint8_t blue)
{
	float brightness_ratio = (float) brightness / 255;
	uint8_t adjusted_red =  brightness_ratio * red;
	uint8_t adjusted_green =  brightness_ratio * green;
	uint8_t adjusted_blue =  brightness_ratio * blue;


	while (WS2812_hdma->State != HAL_DMA_STATE_READY)
	{;}
	(WS2812_RGBdata_arr[pixel_idx]).red = adjusted_red;
	(WS2812_RGBdata_arr[pixel_idx]).green = adjusted_green;
	(WS2812_RGBdata_arr[pixel_idx]).blue = adjusted_blue;
}

void WS2812_show()
{
	// wait for current RGB array data transfer to complete
	while (WS2812_hdma->State != HAL_DMA_STATE_READY)
	{;}

  // fill DMA double buffer with PWM data for first LEDs
  color24bit_to_ccr16bit_array(WS2812_RGBdata_arr[0], WS2812_DMAdoublebuff);
  if (WS2812_NUM_LEDS > 1)
  {
  	color24bit_to_ccr16bit_array(WS2812_RGBdata_arr[1], WS2812_DMAdoublebuff + WS2812_RGB_DATA_BIT_LEN);
    // set RGB index to last index of RGBarr data sent to DMA buffer
  	WS2812_RGB_ind = 1;
  }
  else
  {
  	memset(WS2812_DMAdoublebuff + WS2812_RGB_DATA_BIT_LEN, 0, WS2812_RGB_DATA_BIT_LEN * sizeof(uint16_t));
    // set RGB index to last index of RGBarr data sent to DMA buffer
  	WS2812_RGB_ind = 0;
  }

	// Start PWM / DMA

  if (HAL_TIM_PWM_Start_DMA(WS2812_htim, WS2812_tim_ch, (uint32_t*)WS2812_DMAdoublebuff, WS2812_DMA_BUFF_SIZE) != HAL_OK) {
  	Error_Handler();
  }
}

static void color24bit_to_ccr16bit_array(RGB_color_24bit color_data,
																				 uint16_t *PWM_DMAhalfbuff)
{
	for (int i = 0; i <WS2812_BITS_PER_COLOR; i++) {
		// Green setting
		PWM_DMAhalfbuff[i] = ((color_data.green << i) & 0x80) ? WS2812_PWM_BIT_HI : WS2812_PWM_BIT_LOW;

		// Red setting
		PWM_DMAhalfbuff[i + WS2812_BITS_PER_COLOR] = ((color_data.red << i) & 0x80) ? WS2812_PWM_BIT_HI : WS2812_PWM_BIT_LOW;

		// Blue setting
		PWM_DMAhalfbuff[i + (2 * WS2812_BITS_PER_COLOR)] = ((color_data.blue << i) & 0x80) ? WS2812_PWM_BIT_HI : WS2812_PWM_BIT_LOW;
	}
}

static void update_DMA_halfbuff(uint16_t *pDMA_halfbuff)
{
	// update RGB data array index
	WS2812_RGB_ind = (WS2812_RGB_ind + 1) % (WS2812_NUM_LEDS + WS2812_LOW_VOLT_RESET_PERIODS);

	// if low-voltage reset periods will be completed completed, LEDs are set, so shut down PWM/DMA
	if (WS2812_reset_period_count == WS2812_LOW_VOLT_RESET_PERIODS)
	{
		HAL_TIM_PWM_Stop_DMA(WS2812_htim, WS2812_tim_ch);
		WS2812_reset_period_count = 0;
	}
	// RGB array not finished transferring to to PWM
	else if (WS2812_RGB_ind < WS2812_NUM_LEDS)
	{
		WS2812_PWM_xfer_status = WS2812_XFER_RGB_ONGOING;
		color24bit_to_ccr16bit_array(WS2812_RGBdata_arr[WS2812_RGB_ind], pDMA_halfbuff); // fill DMA half buffer
	}
	// RGB array finished transferring, conduct PWM transfer reset low-voltage periods
	else
	{
		WS2812_reset_period_count++;
		memset(pDMA_halfbuff, 0, WS2812_RGB_DATA_BIT_LEN * sizeof(uint16_t)); // set half buffer to 0% dutycyles
	}
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1)
	{
		// first 1st half of DMA buffer
		update_DMA_halfbuff(WS2812_DMAdoublebuff);
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1) {
		// first 2nd half of DMA buffer
		update_DMA_halfbuff(WS2812_DMAdoublebuff + WS2812_RGB_DATA_BIT_LEN);
	}
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
	if (hdma->Instance == DMA2_Stream2) {
			// Your error handling code
	}
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}



