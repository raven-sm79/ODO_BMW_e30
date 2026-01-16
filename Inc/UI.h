#pragma once
#include <stdint.h>


typedef struct {
    uint8_t hh, mm;
    uint8_t dd, mo;
    uint16_t yyyy;

    int16_t temp_c;     // например -12
    uint16_t volt_mv;   // например 14700

    uint32_t odo_main;  // 6 цифр (0..999999)
    uint32_t odo0;      // 5 цифр (0..99999)
    uint32_t odo1;
    uint32_t odo2;

} ui_data_t;

void ui_draw_static_test(void);

/* === Инициализация === */
void ui_draw_static(void);

/* Обновляет динамику: время/дата/вольты/темп/пробеги */
void ui_draw_all(const ui_data_t *d);

/* === Динамика (потом) === */
void ui_draw_time(uint8_t hh, uint8_t mm);
static void ui_draw_date(uint8_t dd, uint8_t mo, uint16_t yyyy);
static void ui_draw_voltage(uint16_t mv);
static void ui_draw_temp(int16_t t);

/* === Пробеги === */
void ui_draw_main_odometer(uint32_t km);
void ui_draw_odometer(uint8_t idx, uint32_t value);

/* === Слои === */
void ui_set_layer(uint8_t layer);

/* ВСПОМОГАТЕЛЬНОЕ */
void ui_set_font(uint8_t font_id);   // пока затычка
