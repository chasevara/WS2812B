/*
 * WS2812_16x16matrix_2x2array.h
 *
 *  Created on: May 1, 2024
 *      Author: Chase Vara
 *
 * Driver to operate a 2x2 array of 16X16 WS2812B LED panels
 *
 * @detail  WARNING: Driving full matrix (1024 LEDs) at high brightness will
 *           draw significant current. Consult WS2812B datasheet and select
 *           appropriately rated power supply to prevent component damage
 *           and/or physical injury to user.
 *
 * Single panel structure: (LEDs are connected in serial, zig-zagging back and forth accros arrays
 *
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +  <-- panel array start
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |      (control input)
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *     + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - + - +
 *     ^
 *     |
 *     panel array end
 *     (control output)
 *
 *
 * 2x2 array of panels is interconnected accordingly:
 *
 *                     GPIO
 *                    control                       Quad-I
 *                      pin                          out
 *                       |                            |
 *                       V                            V
 *     + --------------- +          + --------------- +
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |     Quadrant    |          |     Quadrant    |
 *     |        I        |          |        II       |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     + --------------- +          + --------------- +
 *   Quad-I                       Quad-II
 *    out                           out
 *     |                             |
 *     V                             V

 *                    Quad-II                         Quad-III
 *                      out                          out
 *                       |                             |
 *                       V                             V
 *     + --------------- +          + --------------- +
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |     Quadrant    |          |     Quadrant    |
 *     |       III       |          |        IV       |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     |                 |          |                 |
 *     + --------------- +          + --------------- +
 *  Quad-III                     Quad-IV
 *    out                          out
 *     |                            |
 *     V                            V
 *
 *
 * WS2812PANEL_MATRIX_2X2_setXY() function abstracts the
 * LED array layout (panel structure and panel interconnectedness)
 * so that the matrix is treated as a 32x32 array in an x-y plane,
 * making image-mapping intuitive:
 *
 *     0 ------------->  x
 *     |
 *     |   + ---------------------------- +
 *     |   |                              |
 *     |   |                              |
 *     |   |                              |
 *     |   |                              |
 *     V   |                              |
 *         |                              |
 *     y   |       Combined Matrix        |
 *         |      with hardware LED       |
 *         |      layout abstracted       |
 *         |       to an x-y plane        |
 *         |                              |
 *         |                              |
 *         |                              |
 *         |                              |
 *         + ---------------------------- +
 *
 */

#ifndef SRC_WS2812_16X16MATRIX_2X2ARRAY_H_
#define SRC_WS2812_16X16MATRIX_2X2ARRAY_H_

#include "WS2812.h"
#include "stm32f4xx_hal.h"
#include "W_32x32.h"

#define WS2812PANEL_MATRIX_2X2_WIDTH 32
#define WS2812PANEL_MATRIX_2X2_HEIGHT 32
#define WS2812_PANEL_WIDTH 16
#define WS2812_PANEL_HEIGHT 16
#define WS2812_PANEL_LED_ARR_LEN (WS2812_PANEL_WIDTH * WS2812_PANEL_HEIGHT)

// Function prototypes

/**
  *  @brief  function to RGB value of the  LED at coord (x, y)
  *
  *  @param  x -- x-coordinate of matrix LED to update
  *  @param  y -- y-coordinate of matrix LED to update
  *  @param  brightness -- maximum brightness setting for each color (0 - 255)
  *  @param  red -- LED red setting (0 - 255)
  *  @param  green -- LED green setting (0 - 255)
  *  @param  blue -- LED blue setting (0 - 255)
  *
  *  @detail  coordinate origin at matrix top left
  */
void WS2812PANEL_MATRIX_2X2_setXY(uint8_t x,
                                  uint8_t y,
                                  uint8_t brightness,
                                  uint8_t red,
                                  uint8_t green,
                                  uint8_t blue);

/**
  *  @brief  function to test WS2812_MATRIX_4X4_setXY() by progressing a blue dot
  *          left-to-right through rows, moving through rows top-to-bottom
  *
  *  @param  brightness -- maximum brightness setting for each color (0 - 255)
  */
void WS2812PANEL_MATRIX_2X2_test_coord(uint8_t brightness);

/**
  *  @brief  function to test WS2812_MATRIX_4X4_setXY() by displaying the
  *          University of Washington collegiate letter logo
  *
  *  @param  brightness -- maximum brightness setting for each color (0 - 255)
  */
void WS2812PANEL_MATRIX_2X2_test_W(uint8_t brightness);

#endif /* SRC_WS2812_16X16MATRIX_2X2ARRAY_H_ */
