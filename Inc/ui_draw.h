#pragma once
#include <stdint.h>

/* Цвета */
#define UI_WHITE   	0xFFFF
#define UI_BLACK   	0x0000
#define UI_YELLOW  	0xFFE0
#define UI_RED     	0xF800

#define WHITE   	UI_WHITE
#define BLACK   	UI_BLACK
#define YELLOW  	UI_YELLOW
#define RED     	UI_RED


/* Примитивы */
void ui_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void ui_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void ui_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ui_draw_icon(uint16_t x, uint16_t y, const unsigned char *icon, uint16_t w, uint16_t h);

