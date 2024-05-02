/*
 * WS2812_16x16matrix_2x2array.c
 *
 *  Created on: May 1, 2024
 *      Author: Chase
 */
#include "WS2812.h"
#include "stm32f4xx_hal.h"
#include "WS2812_16x16matrix_2x2array.h"
#include "W_32x32.h"

#define WS2812PANEL_MATRIX_2X2_WIDTH 32
#define WS2812PANEL_MATRIX_2X2_HEIGHT 32
#define WS2812_PANEL_WIDTH 16
#define WS2812_PANEL_HEIGHT 16
#define WS2812_PANEL_LED_ARR_LEN (WS2812_PANEL_WIDTH * WS2812_PANEL_HEIGHT)

// Helper function prototypes

/**
 	*	@brief	Helper function to map x-y coordinate to the index of the total LED array
 	*	that composes the matrix
  *
  *	@param	x -- x-coordinate of matrix LED to update
  *	@param	y -- y-coordinate of matrix LED to update
  *
  *	@retval WS2812B LED array index (uint32_t) that x-y maps to.
  */
static uint32_t get_array_index(uint8_t x,
																uint8_t y);

/**
 	*	@brief	Helper function to return the WS2812B LED array index that maps to
 	*					the first panel LED of the panel that a matrix x-y coordinate maps to
  *
  *	@param	x -- x-coordinate of matrix LED to update
  *	@param	y -- y-coordinate of matrix LED to update
  *
  *	@retval WS2812B LED array index (uint32_t)
  */
static uint32_t get_panel_origin_index(uint8_t x,
														 	 				 uint8_t y);

/**
 	*	@brief	Helper function to get the WS2812 panel LED array index corresponds to the
 	*					0th column in the panel row that matrix x-y coordinate maps to. Used to
 	*					abstract the interconnectedness structure of the panels that compose
 	*					the matrix
  *
  *	@param	y -- y-coordinate of matrix LED to update
  *
  *	@retval Matrix-wise row index of panel (uint32_t)
  */
static uint32_t get_panel_row_index(uint8_t y);

/**
 	*	@brief	Helper function to return the WS2812B LED array index that maps to
 	*					the first panel LED of the panel that a matrix x-y coordinate maps to.
 	*					Used to abstract the zig-zag pattern of the LED array through the
 	*					panel structure
  *
  *	@param	x -- x-coordinate of matrix LED to update
  *	@param	y -- y-coordinate of matrix LED to update
  *
  *	@retval WS2812B LED row array index (uint32_t) that x-coord maps to in the panel
  *					row that y-coord maps to.
  */
static uint32_t get_panel_col_index(uint8_t x, uint8_t y);

// function definitions

// function to the LED at coord (x, y). coordinate origin at matrix to left
void WS2812PANEL_MATRIX_2X2_setXY(uint8_t x,
														 uint8_t y,
														 uint8_t brightness,
														 uint8_t red,
														 uint8_t green,
														 uint8_t blue)
{
	// calculate LED array index corresponding to the matrix x-y coordinate
	uint32_t WS2812_arr_idx = get_array_index(x, y);

	// set pixel
	WS2812_set_pixel(WS2812_arr_idx, brightness, red, green, blue);
}

// function to test WS2812_MATRIX_4X4_setXY() by progressing a blue dot left-to-right through rows,
// moving through rows top-to-bottom
void WS2812PANEL_MATRIX_2X2_test_coord(uint8_t brightness)
{
	uint8_t x_blue = 0;
	uint8_t y_blue = 0;
	for (int i = 0; i < WS2812_NUM_LEDS; i++)
	{
		// turn pixel blue
	  WS2812PANEL_MATRIX_2X2_setXY(x_blue, y_blue, brightness, 0, 0, 255);
		WS2812_show();

		// turn pixel off
	  WS2812PANEL_MATRIX_2X2_setXY(x_blue, y_blue, brightness, 0, 0, 0);

	  // progress to next pixel
		if (x_blue == WS2812PANEL_MATRIX_2X2_WIDTH - 1)
		{
		  y_blue = (y_blue + 1) % WS2812PANEL_MATRIX_2X2_HEIGHT;
		}
		x_blue = (x_blue + 1) % WS2812PANEL_MATRIX_2X2_WIDTH;
	}
}

// function to test WS2812_MATRIX_4X4_setXY() by displaying the University of Washington letter
void WS2812PANEL_MATRIX_2X2_test_W(uint8_t brightness) {
	uint8_t image[32][32][3] = W32_32;
	for (int i = 0; i < WS2812PANEL_MATRIX_2X2_WIDTH; i++)
	{
		for (int j = 0; j < WS2812PANEL_MATRIX_2X2_HEIGHT; j++)
		{
			WS2812PANEL_MATRIX_2X2_setXY(i, j, brightness, image[j][i][0], image[j][i][1], image[j][i][2]);
		}
	}
	WS2812_show();
}

// function to get the WS2812 array index that maps to the x-y coordinate of the 2x2 panel matrix
static uint32_t get_array_index(uint8_t x,
																uint8_t y)
{
	uint32_t origin_idx = get_panel_origin_index(x, y);
	uint32_t row_dist_from_origin = get_panel_row_index(y);
	uint32_t col_dist_from_row_origin = get_panel_col_index(x, y);
	return origin_idx + row_dist_from_origin + col_dist_from_row_origin;
}

// function to get the WS2812 array index that corresponds to the panel that the x-y coordinate maps to
static uint32_t get_panel_origin_index(uint8_t x,
														 	 				 uint8_t y)
{
	// y is in upper half of 2x2 panel matrix
	if (y < WS2812_PANEL_HEIGHT)
	{
		// Case 1: (x, y) in 1st Quadrant
		if (x < WS2812_PANEL_WIDTH)
		{
			return 0;
		}
		// Case 2: (x, y) in 2nd Quadrant
		else
		{
			return WS2812_PANEL_LED_ARR_LEN;
		}
	}
	else
	{
		// Case 3: (x, y) in 3rd Quadrant
		if (x < WS2812_PANEL_WIDTH)
		{
			return WS2812_PANEL_LED_ARR_LEN * 2;
		}
		// Case 4: (x, y) in 4th Quadrant
		else
		{
			return WS2812_PANEL_LED_ARR_LEN * 3;
		}
	}
}

// function to get the WS2812 array index corresponding to the 0th column coord in the panel
// row that (x, y) maps to
static uint32_t get_panel_row_index(uint8_t y)
{
	uint32_t row_num = y % WS2812_PANEL_HEIGHT;
	return row_num * WS2812_PANEL_WIDTH;
}

// function to get the WS2812 array index corresponds to the 0th column in the panel
// row that (x, y) maps to
static uint32_t get_panel_col_index(uint8_t x, uint8_t y)
{
	// x coordinate relative to its positioning in the panel it maps to
	uint8_t x_panel = x % WS2812_PANEL_WIDTH;

	// Case 1: Row coord is odd, column coord maps to ascending WS2812 array indices
	if (y % 2)
	{
		return x_panel;
	}
	// Case 2: Row coord is even, column cord maps to descending WS2812 array indices
	else
	{
		return WS2812_PANEL_WIDTH - x_panel - 1;
	}
}
