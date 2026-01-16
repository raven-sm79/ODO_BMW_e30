#include "ui_draw.h"
#include "st7789v.h"
//#include "Fonts/font75_digits.h"


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


