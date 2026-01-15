/*
 * colors.h
 *
 *  Created on: Jan 4, 2026
 *      Author: user
 */

#ifndef INC_COLORS_H_
#define INC_COLORS_H_

#pragma once
#include <stdint.h>

/* RGB565 colors */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF

#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F

#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

//----------------------------

#define BLACK   0x0000
#define WHITE   0xFFFF

#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F

#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F

/* UI specific */
#define COLOR_BG      COLOR_BLACK
#define COLOR_TEXT    COLOR_WHITE
#define COLOR_WARN    COLOR_RED

#endif /* INC_COLORS_H_ */
