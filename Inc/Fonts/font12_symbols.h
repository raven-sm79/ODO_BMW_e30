#include <stdint.h>

#define FONT12_W   12
#define FONT12_H   18
#define FONT12_BPR ((FONT12_W + 7) / 8)
#define FONT12_SZ  (FONT12_H * FONT12_BPR)
#define FONT12_COUNT 17

extern const uint8_t font12_glyphs[FONT12_COUNT][FONT12_SZ];
extern const char    font12_map[FONT12_COUNT+1];
