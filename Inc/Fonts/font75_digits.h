#pragma once
#include <stdint.h>

#define DIG75_W   29
#define DIG75_H   73
#define DIG75_BPR ((DIG75_W + 7) / 8)

extern const uint8_t font75_digits[10][DIG75_H * DIG75_BPR];

#define DIG75_SZ  (DIG75_H * DIG75_BPR)

extern const uint8_t font75_digits[10][DIG75_SZ];
