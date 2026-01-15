#pragma once
#include <stdint.h>

/* === Инициализация === */
void ui_draw_static(void);

/* === Динамика (потом) === */
void ui_draw_time(uint8_t hh, uint8_t mm);
void ui_draw_date(uint8_t d, uint8_t m, uint16_t y);
void ui_draw_voltage(float v);
void ui_draw_temperature(int8_t t);

/* === Пробеги === */
void ui_draw_main_odometer(uint32_t km);
void ui_draw_odometer(uint8_t idx, uint32_t value);

/* === Слои === */
void ui_set_layer(uint8_t layer);

/* ВСПОМОГАТЕЛЬНОЕ */
void ui_set_font(uint8_t font_id);   // пока затычка
