#include "ui_fonts.h"

/*
 * ВРЕМЕННО:
 * lcd_char / lcd_string используют один и тот же шрифт.
 * Эта функция — затычка под будущую реализацию.
 */
void ui_set_font(uint8_t font_id)
{
    (void)font_id;
    /* ЗАТЫЧКА — ничего не делает */
}
