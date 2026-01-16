/*
 * ST7789.h
 *
 *  Created on: Jan 2, 2026
 *      Author: user
 */

#ifndef INC_ST7789V_H_
#define INC_ST7789V_H_

#include "main.h"

static void lcd_reset(void);
static inline void lcd_cmd(uint16_t cmd);
static inline void lcd_data(uint16_t data);
void st7789_init(void);
void lcd_fill_screen(uint16_t color);
static void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_char(uint16_t x, uint16_t y, uint8_t c, uint16_t fg, uint16_t bg);
void lcd_string(uint16_t x, uint16_t y, const uint8_t *s, uint16_t fg, uint16_t bg);
void draw_time(uint8_t hh, uint8_t mm);
void UI_BlinkColon(void);
void ui_draw_static(void);
void lcd_draw_mono_1bpp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data, uint16_t bpr, uint16_t fg, uint16_t bg);


#endif /* INC_ST7789V_H_ */
