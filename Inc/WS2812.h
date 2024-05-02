/*
 * WS2812.h
 *
 *  Created on: Apr 26, 2024
 *      Author: Chase
 */

#ifndef SRC_WS2812_H_
#define SRC_WS2812_H_

#include "stm32f4xx_hal.h"
#include "WS2812.h"
#include <string.h>

// duty cycle percentages to affect modulation according to WS2812B datasheet
#define WS2812_PWM_HI_DUTY (0.62)
#define WS2812_PWM_LOW_DUTY (0.38)

#define WS2812_NUM_LEDS 1024
#define WS2812_NUM_COLORS 3  // R, G, B
#define WS2812_BITS_PER_COLOR 8
#define WS2812_RGB_DATA_BIT_LEN (WS2812_NUM_COLORS * WS2812_BITS_PER_COLOR)
#define WS2812_DMA_BUFF_SIZE (WS2812_RGB_DATA_BIT_LEN * 2)
#define WS2812_LOW_VOLT_RESET_PERIODS 3 /* number of 24-bit periods to to reduce
																					 PWM to 0% duty to signal end of WSB2812
																					 data transmission. Ensures at least
																					 50 microseconds of low voltage signal is
																					 given to WS2812B LEDs to indicate end of
																					 data transfer, in accordance with
																					 WS2812B datasheet.
																				 */

// Flag values to indicate PWM modulation data transfer status.
#define WS2812_XFER_RGB_READY 0
#define WS2812_XFER_RGB_ONGOING 1

// data structure for led RGB color values
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} RGB_color_24bit;


// globals variables

// LEDs, color data and DMA buffer arrays
extern RGB_color_24bit WS2812_RGBdata_arr[WS2812_NUM_LEDS];
extern uint16_t WS2812_DMAdoublebuff[WS2812_DMA_BUFF_SIZE];
extern uint8_t WS2812_PWM_xfer_status;
extern uint8_t WS2812_reset_period_count;
extern uint16_t WS2812_PWM_BIT_HI;
extern uint16_t WS2812_PWM_BIT_LOW;
extern uint32_t WS2812_RGB_ind;

// peripheral handles
extern DMA_HandleTypeDef *WS2812_hdma;
extern TIM_HandleTypeDef *WS2812_htim;
extern uint32_t WS2812_tim_ch;


/**
 	*	@brief 	function to initialize variables and buffers for driver
 	*
 	*	@param	htim --	STM32 16-bit PWM timer peripheral HAL handle
 	*	@param	hdma -- STM32 DMA (circular mode) peripheral HAL handle
 	*	@param	timer_channel -- peripheral timer channel linked to DMA
 	*
 	*	@detail			16-bit timer must be initialized in PWM with output mode, configured
 	*							for DMA request (circular mode) linked to same channel timer
 	*							is configured for. Clocks and timer must be configured to affect
 	*							a PWM frequency of 800 kHz.
  */
void WS2812_init(TIM_HandleTypeDef *htim,
								 DMA_HandleTypeDef *hdma,
								 uint32_t timer_channel);


/**
 	*	@brief 	function set the RGB values for a parameter index in an array of
 	*					WS2812B LEDs.
 	*
 	*	@param 	pixel_idx -- Index of WS2812B LED whose RGB data will be set
 	*	@param	brightness -- Brightness (0 - 255) to set R, G, and B values to
 	*	@param	red -- Red data setting (0 - 255) to set
 	*	@param	green -- Green data setting (0 - 255) to set
 	*	@param	blue -- Blue data setting (0 - 255) to set
 	*
 	*	@detail	Function sets the RGB value of the indexed LED, but does not affect
 	*					current physical output of the LED. LED output is updated
 	*					by WS2812_show(). If DMA is active amidst transferring previous RGB
 	*					data to WS2812B array, function will block CPU before updating RGB
 	*					data until DMA enters its ready-state.
 	*/
void WS2812_set_pixel(uint32_t pixel_idx,
											uint8_t brightness,
											uint8_t red,
											uint8_t green,
											uint8_t blue);
/**
 	*	@brief 	function to update WS2812B array to output current RGB data settings
 	*					that were updated by calls to WS2812_set_pixel()
 	*
 	*	@detail If DMA is active amidst transferring previous RGB
 	*					data to WS2812B array, function will block CPU before initiating a
 	*					new data transfer until DMA enters its ready-state
 	*/
void WS2812_show();

/**
 	*	@brief 	System callback function for circular DMA half-transfer complete,
 	*					for use by the STM32 HAL DMA IRQ function. Signals driver to transfer
 	*					next LED's RGB PWM modulation settings circular DMA double buffer.
 	*
 	*	@param	htim -- STM32 16bit PWM timer HAL handle
 	*/
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim);

/**
 	*	@brief 	System callback function for circular DMA transfer complete,
 	*					for use by the STM32 HAL DMA IRQ function. Signals driver to transfer
 	*					next LED's RGB PWM modulation settings to circular DMA double buffer.
 	*
 	*	@param	htim -- STM32 16bit PWM timer HAL handle
 	*/
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);

/**
 	*	@brief 	System callback function for DMA error, for use by the STM32 HAL DMA
 	*	IRQ function. Currently has no affect.
 	*
 	*	@param	hdma -- STM32 DMA peripheral HAL handle
 	*/
void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma);

/**
 	*	@brief 	System callback function for general error handling
 	*/
void Error_Handler(void);

#endif /* SRC_WS2812_H_ */
