#pragma once

#include <stdint.h>
#include <stdbool.h>

/* =========================
 *  TOTAL ODOMETER (critical)
 * ========================= */

#define EE_TOTAL0_ADDR   0x000
#define EE_TOTAL1_ADDR   0x010
#define EE_TOTAL2_ADDR   0x020

typedef struct
{
    uint32_t km;      // общий пробег
    uint32_t crc;     // CRC32 от km
} ee_total_t;


/* =========================
 *  OTHER ODOMETERS (A/B)
 * ========================= */

#define EE_ODO_A_ADDR    0x040
#define EE_ODO_B_ADDR    0x080

typedef struct
{
    uint32_t odo[8];   // возрастающие, сбрасываемые
    uint16_t cntr[8];  // убывающие сервисные
} ee_odo_t;


/* =========================
 *  Public API
 * ========================= */

/* TOTAL */
uint32_t ee_total_load(void);
void     ee_total_save(uint32_t km);

/* ODO / CNTR */
void     ee_odo_load(ee_odo_t *d);
void     ee_odo_save(const ee_odo_t *d);


/* =========================
 *  Low-level EEPROM
 * ========================= */

void EEPROM_ReadBlock(uint16_t addr, void *buf, uint16_t len);
void EEPROM_WriteBlock(uint16_t addr, const void *buf, uint16_t len);

/* =========================
 *  CRC
 * ========================= */

uint32_t crc32_calc(const uint8_t *data, uint32_t len);
