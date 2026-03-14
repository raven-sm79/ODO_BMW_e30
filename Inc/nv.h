#pragma once
#include <stdint.h>
#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

// Флаги ошибок
#define ERR_EEPROM     0x01  // ошибка внешней EEPROM
#define ERR_FLASH      0x02  // ошибка записи во Flash

extern volatile uint8_t g_system_error;

void NV_Init(void);
int  NV_Load(ui_data_t *d, uint16_t *pulse_rem);
int  NV_Save(const ui_data_t *d, uint16_t pulse_rem);

// Внутреннее API (не обязательно вызывать из main)
void NV_BackupToFlash(const ui_data_t *d, uint16_t pulse_rem);
int  NV_RestoreFromFlash(ui_data_t *d, uint16_t *pulse_rem);
void NV_ClearFlashBackup(void);

#ifdef __cplusplus
}
#endif
