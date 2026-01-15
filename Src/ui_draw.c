#include "ui_draw.h"
#include "st7789v.h"
#include "Fonts/font75_digits.h"

extern const uint8_t font75_colon[DIG75_H * DIG75_BPR];


/* Горизонтальная линия */
void ui_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    lcd_fill_rect(x, y, w, 1, color);
}

/* Вертикальная линия */
void ui_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    lcd_fill_rect(x, y, 1, h, color);
}

/* Прямоугольник */
void ui_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ui_draw_hline(x, y, w, color);
    ui_draw_hline(x, y + h - 1, w, color);
    ui_draw_vline(x, y, h, color);
    ui_draw_vline(x + w - 1, y, h, color);
}

/* Иконка 1bpp (24x24, Arduino-стиль) */
void ui_draw_icon(uint16_t x, uint16_t y, const unsigned char *icon, uint16_t w, uint16_t h)
{
    uint16_t byteWidth = (w + 7) / 8;

    for (uint16_t j = 0; j < h; j++)
    {
        for (uint16_t i = 0; i < w; i++)
        {
            uint8_t byte = icon[j * byteWidth + i / 8];
            if (byte & (128 >> (i & 7)))
            {
                lcd_pixel(x + i, y + j, UI_WHITE);
            }
        }
    }
}

void draw_digit75(uint16_t x, uint16_t y, uint8_t d, uint16_t color)
{
    const uint8_t *bmp = font75_digits[d];

    for (uint16_t row = 0; row < DIG75_H; row++)
    {
        const uint8_t *line = bmp + row * DIG75_BPR;

        for (uint16_t col = 0; col < DIG75_W; col++)
        {
            if (line[col >> 3] & (0x80 >> (col & 7)))
//        	  if (line[col >> 3] & (1 << (col & 7)))
            {
                lcd_pixel(x + col, y + row, color);
            }
        }
    }
}

void draw_colon75(uint16_t x, uint16_t y, uint16_t color)
{
    const uint8_t *bmp = font75_colon;

    for (uint16_t row = 0; row < DIG75_H; row++)
    {
        const uint8_t *line = bmp + row * DIG75_BPR;

        for (uint16_t col = 0; col < DIG75_W; col++)
        {
            if (line[col >> 3] & (0x80 >> (col & 7)))
//        	if (line[col >> 3] & (1 << (col & 7)))
            {
                lcd_pixel(x + col, y + row, color);
            }
        }
    }
}

