#pragma once

/* ui_fonts.h — ТОЛЬКО алиасы и include */

#include "Fonts/font75_digits.h"

/* Никаких extern font75_digits здесь больше нет */

#define UI_FONT_12   0
#define UI_FONT_27   1
#define UI_FONT_36   2
#define UI_FONT_75   3

void ui_set_font(uint8_t font_id);
