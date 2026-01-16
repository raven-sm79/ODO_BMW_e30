/*
 * ST7789V.c
 *
 *  Created on: Jan 2, 2026
 *      Author: user
 */

#include "ST7789V.h"
#include "font6x8.h"
#include "colors.h"

#define LCD_CMD   (*((volatile uint16_t *)0x60000000))
#define LCD_DATA  (*((volatile uint16_t *)0x60020000))
#define LCD_RST_PORT GPIOC
#define LCD_RST_PIN  GPIO_PIN_10

#define TIME_Y     60

#define HH1_X      20
#define HH2_X      70
#define COLON_X   120
#define MM1_X     140
#define MM2_X     190

static uint8_t last_hh = 0xFF;
static uint8_t last_mm = 0xFF;

static void lcd_reset(void)
{
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);
}

static inline void lcd_cmd(uint16_t cmd)
{
    LCD_CMD = cmd;
}

static inline void lcd_data(uint16_t data)
{
    LCD_DATA = data;
}

void st7789_init(void)
{
    lcd_reset();

    lcd_cmd(0x11);      // Sleep Out
    HAL_Delay(120);

    lcd_cmd(0x36);      // MADCTL
    lcd_data(0x00);     // orientation (потом настроим)

    lcd_cmd(0x3A);      // COLMOD
    lcd_data(0x55);     // 16-bit RGB565

    lcd_cmd(0x29);      // Display ON
}

void lcd_fill_screen(uint16_t color)
{
    lcd_cmd(0x2C); // Memory Write
    for (uint32_t i = 0; i < 240UL * 320UL; i++)
        lcd_data(color);
}

static void st7789_set_window(uint16_t x0, uint16_t y0,
                              uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A); // CASET
    lcd_data(x0 >> 8); lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8); lcd_data(x1 & 0xFF);

    lcd_cmd(0x2B); // RASET
    lcd_data(y0 >> 8); lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8); lcd_data(y1 & 0xFF);

    lcd_cmd(0x2C); // RAMWR
}

static const uint8_t seg_map[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

static void seg_rect(uint16_t x, uint16_t y,
                     uint16_t w, uint16_t h,
                     uint16_t color)
{
    lcd_fill_rect(x, y, w, h, color);
}

void draw_digit(uint16_t x, uint16_t y,
                uint8_t d,
                uint16_t color,
                uint16_t bg)
{
    uint16_t t = 6;   // толщина
    uint16_t l = 30;  // длина

    uint8_t m = seg_map[d];

    // A
    if (m & 0x01) seg_rect(x+t, y, l, t, color);
    // B
    if (m & 0x02) seg_rect(x+l+t, y+t, t, l, color);
    // C
    if (m & 0x04) seg_rect(x+l+t, y+l+2*t, t, l, color);
    // D
    if (m & 0x08) seg_rect(x+t, y+2*l+2*t, l, t, color);
    // E
    if (m & 0x10) seg_rect(x, y+l+2*t, t, l, color);
    // F
    if (m & 0x20) seg_rect(x, y+t, t, l, color);
    // G
    if (m & 0x40) seg_rect(x+t, y+l+t, l, t, color);
}

void draw_colon(uint16_t x, uint16_t y, uint16_t color)
{
    uint16_t r = 4;

    lcd_fill_rect(x, y + 20, r, r, color);
    lcd_fill_rect(x, y + 40, r, r, color);
}

void UI_BlinkColon(void)
{
    static uint8_t colon_state = 0;

    colon_state ^= 1;

    if (colon_state)
    {
        // нарисовать двоеточие
    	lcd_fill_rect(120, 60, 6, 16, COLOR_WHITE);
    }
    else
    {
        // стереть двоеточие
    	lcd_fill_rect(120, 60, 6, 16, COLOR_BLACK);;
    }
}

void draw_time(uint8_t hh, uint8_t mm)
{

    if (hh != last_hh)
    {
        // стереть старые часы
        draw_digit(HH1_X, TIME_Y, last_hh / 10, COLOR_BLACK, COLOR_BLACK);
        draw_digit(HH2_X, TIME_Y, last_hh % 10, COLOR_BLACK, COLOR_BLACK);

        // нарисовать новые
        draw_digit(HH1_X, TIME_Y, hh / 10, COLOR_WHITE, COLOR_BLACK);
        draw_digit(HH2_X, TIME_Y, hh % 10, COLOR_WHITE, COLOR_BLACK);

        last_hh = hh;
    }

    if (mm != last_mm)
    {
        draw_digit(MM1_X, TIME_Y, last_mm / 10, COLOR_BLACK, COLOR_BLACK);
        draw_digit(MM2_X, TIME_Y, last_mm % 10, COLOR_BLACK, COLOR_BLACK);

        draw_digit(MM1_X, TIME_Y, mm / 10, COLOR_WHITE, COLOR_BLACK);
        draw_digit(MM2_X, TIME_Y, mm % 10, COLOR_WHITE, COLOR_BLACK);

        last_mm = mm;
    }

	draw_colon(120, 60, COLOR_WHITE);

}

void lcd_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    st7789_set_window(x, y, x, y);
    lcd_data(color);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    st7789_set_window(x, y, x + w - 1, y + h - 1);
    for (uint32_t i = 0; i < w * h; i++)
        lcd_data(color);
}

void lcd_char(uint16_t x, uint16_t y, uint8_t c, uint16_t fg, uint16_t bg)
{
    const uint8_t *glyph = 0;

    // ASCII 0x20–0x7F
    if (c >= 0x20 && c < 0x80)
    {
        glyph = font6x8_ascii[c - 0x20];
    }
    // Кириллица 0xC0–0xFF
    else if (c >= 0xC0)
    {
        glyph = font6x8_cyr[c - 0xC0];
    }
    else
    {
        return;
    }

    for (uint8_t col = 0; col < 6; col++)
    {
        uint8_t line = glyph[col];
        for (uint8_t row = 0; row < 8; row++)
        {
            lcd_pixel(x + col, y + row,
                      (line & 0x01) ? fg : bg);
            line >>= 1;
        }
    }
}

void lcd_string(uint16_t x, uint16_t y, const uint8_t *s,
                uint16_t fg, uint16_t bg)
{
    while (*s)
    {
        lcd_char(x, y, *s++, fg, bg);
        x += 6;
    }
}

void lcd_draw_mono_1bpp(uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        const uint8_t *data, uint16_t bpr,
                        uint16_t fg, uint16_t bg)
{
    for (uint16_t yy = 0; yy < h; yy++) {
        const uint8_t *row = data + yy * bpr;
        for (uint16_t xx = 0; xx < w; xx++) {
            uint8_t b = row[xx >> 3];
            uint8_t bit = 7 - (xx & 7);   // MSB-first
            uint16_t col = (b & (1U << bit)) ? fg : bg;
            lcd_pixel(x + xx, y + yy, col);
        }
    }
}
