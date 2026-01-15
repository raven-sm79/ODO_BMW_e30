#include "ui.h"
#include "st7789v.h"
#include "colors.h"
#include "ui_fonts.h"
#include "ui_icons.h"
#include "ui_draw.h"


/* ===== ВНУТРЕННИЕ ФУНКЦИИ ===== */
//static void ui_draw_time(void);
//static void ui_draw_top_icons(void);
//static void ui_draw_voltage_temp(void);
//static void ui_draw_time_placeholder(void);
//static void ui_draw_date_placeholder(void);
//static void ui_draw_main_odometer_placeholder(void);
static void ui_draw_odometer_frame(void);
static void ui_draw_odometer_icons(void);
//static void ui_draw_odometer_placeholders(void);

/* ===== ПУБЛИЧНАЯ ===== */

void ui_draw_static(void)
{
    lcd_fill_screen(BLACK);

//    ui_draw_top_icons();
//    ui_draw_voltage_temp();
//    ui_draw_time();
//    ui_draw_date_placeholder();
//    ui_draw_main_odometer_placeholder();
    ui_draw_odometer_frame();
//    ui_draw_odometer_icons();
//    ui_draw_odometer_placeholders();
}



/* ===== РЕАЛИЗАЦИЯ ===== */
/*
static void ui_draw_top_icons(void)
{
	ui_set_font(UI_FONT_36);
    lcd_set_color(YELLOW, BLACK);
    lcd_draw_char(120, 5, '0');

    lcd_draw_icon(5, 5, ICON_WARN, RED);
}

static void ui_draw_voltage_temp(void)
{
	ui_set_font(UI_FONT_12);

	lcd_string(80, 50, (uint8_t *)"14.7V", WHITE, BLACK);
	lcd_string(170, 50, (uint8_t *)"26C", WHITE, BLACK);
}
*/
void ui_draw_time(uint8_t hh, uint8_t mm)
{
    uint16_t x = 40;
    uint16_t y = 80;
    uint16_t dx = DIG75_W + 6;

    draw_digit75(x + dx * 0, y, hh / 10, WHITE);
    draw_digit75(x + dx * 1, y, hh % 10, WHITE);
    draw_colon75 (x + dx * 2, y, WHITE);
    draw_digit75(x + dx * 3, y, mm / 10, WHITE);
    draw_digit75(x + dx * 4, y, mm % 10, WHITE);
}
/*
static void ui_draw_date_placeholder(void)
{
	ui_set_font(UI_FONT_12);
    lcd_set_color(YELLOW, BLACK);
    lcd_draw_string(120, 165, "22.03.2026");
}

static void ui_draw_main_odometer_placeholder(void)
{
	ui_set_font(UI_FONT_36);
    lcd_set_color(WHITE, BLACK);
    lcd_draw_string(120, 10, "0");
}
*/
static void ui_draw_odometer_frame(void)
{
	ui_draw_rect(5, 200, 230, 115, WHITE);
	ui_draw_vline(120, 200, 115, WHITE);
    ui_draw_hline(5, 257, 230, WHITE);
}
/*
static void ui_draw_odometer_icons(void)
{
	ui_draw_icon(10, 210, ICON_SUT, 36, 36);
	ui_draw_icon(10, 268, ICON_AB, 36, 36);

	ui_draw_icon(125, 210, ICON_BENZ, 36, 36);
	ui_draw_icon(125, 268, ICON_OIL, 36, 36);
}

static void ui_draw_odometer_placeholders(void)
{
	ui_set_font(UI_FONT_27);
    lcd_set_color(YELLOW, BLACK);

    lcd_draw_string(115, 215, "0");
    lcd_draw_string(115, 272, "0");

    lcd_draw_string(230, 215, "0");
    lcd_draw_string(230, 272, "0");
}
*/
