#pragma once
#include <stdint.h>
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

void NV_Init(void);
int  NV_Load(ui_data_t *d, uint16_t *pulse_rem);
int  NV_Save(const ui_data_t *d, uint16_t pulse_rem);

#ifdef __cplusplus
}
#endif
