#include "eeprom.h"
#include <string.h>

typedef enum
{
    ODO_DAY = 0,        // суточный
    ODO_TRIP,           // за поездку
    ODO_TANK,           // за бак
    ODO_UNUSED,         // резерв (наследие)
    ODO_SERVICE,        // с последнего сервиса
    ODO_OIL,            // с последней замены масла
    ODO_6,
    ODO_7
} odo_index_t;


typedef enum
{
    CNTR_SERVICE = 0,   // до сервиса
    CNTR_OIL,           // до замены масла
    CNTR_2,
    CNTR_UNUSED,
    CNTR_4,
    CNTR_5,
    CNTR_6,
    CNTR_7
} cntr_index_t;


/* =========================================================
 *  CRC32
 * ========================================================= */

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    while (len--)
    {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++)
        {
            crc = (crc & 1) ?
                  (crc >> 1) ^ 0xEDB88320 :
                  (crc >> 1);
        }
    }
    return ~crc;
}

/* =========================================================
 *  TOTAL (3 copies + CRC)
 * ========================================================= */

static bool total_read_slot(uint16_t addr, ee_total_t *r)
{
    EEPROM_ReadBlock(addr, r, sizeof(*r));
    return (crc32_calc((uint8_t*)&r->km, 4) == r->crc);
}

uint32_t ee_total_load(void)
{
    ee_total_t r[3];
    bool v[3];

    v[0] = total_read_slot(EE_TOTAL0_ADDR, &r[0]);
    v[1] = total_read_slot(EE_TOTAL1_ADDR, &r[1]);
    v[2] = total_read_slot(EE_TOTAL2_ADDR, &r[2]);

    if (v[0] && v[1] && r[0].km == r[1].km) return r[0].km;
    if (v[0] && v[2] && r[0].km == r[2].km) return r[0].km;
    if (v[1] && v[2] && r[1].km == r[2].km) return r[1].km;

    if (v[0]) return r[0].km;
    if (v[1]) return r[1].km;
    if (v[2]) return r[2].km;

    return 0;
}

void ee_total_save(uint32_t km)
{
    ee_total_t r;
    r.km  = km;
    r.crc = crc32_calc((uint8_t*)&km, 4);

    EEPROM_WriteBlock(EE_TOTAL0_ADDR, &r, sizeof(r));
    EEPROM_WriteBlock(EE_TOTAL1_ADDR, &r, sizeof(r));
    EEPROM_WriteBlock(EE_TOTAL2_ADDR, &r, sizeof(r));
}

/* =========================================================
 *  ODO / CNTR (double copy, no CRC)
 * ========================================================= */

void ee_odo_load(ee_odo_t *d)
{
    ee_odo_t a, b;

    EEPROM_ReadBlock(EE_ODO_A_ADDR, &a, sizeof(a));
    EEPROM_ReadBlock(EE_ODO_B_ADDR, &b, sizeof(b));

    /* простая логика:
       если A == B → берём A
       иначе → берём A (как основной)
       (питание стабильное, запись штатная)
    */
    if (memcmp(&a, &b, sizeof(a)) == 0)
        *d = a;
    else
        *d = a;
}

void ee_odo_save(const ee_odo_t *d)
{
    EEPROM_WriteBlock(EE_ODO_A_ADDR, d, sizeof(*d));
    EEPROM_WriteBlock(EE_ODO_B_ADDR, d, sizeof(*d));
}
