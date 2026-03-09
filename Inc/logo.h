#ifndef LOGO_H
#define LOGO_H

#include <stdint.h>

#define LOGO_WIDTH  160
#define LOGO_HEIGHT 160
#define LOGO_BPR    20      // (160 + 7) / 8 = 20 байт на строку
#define LOGO_SIZE   (LOGO_HEIGHT * LOGO_BPR)

extern const uint8_t LOGO_BMW[LOGO_SIZE];

#endif
