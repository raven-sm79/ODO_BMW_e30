#pragma once

#include <stdint.h>

#define DS3231_ADDR   (0x68 << 1)   // HAL требует 8-битный адрес

/* =========================
 *  DS3231 public types
 * ========================= */

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;    // день недели (1–7)
    uint8_t date;   // день месяца (1–31)
    uint8_t month;  // месяц (1–12)
    uint16_t year;  // полный год (например, 2026)
} rtc_time_t;

typedef enum {
    DS3231_SQW_1HZ   = 0,
    DS3231_SQW_1024HZ,
    DS3231_SQW_4096HZ,
    DS3231_SQW_8192HZ
} DS3231_SQW_Freq;

extern volatile uint8_t rtc_minute_event;

void ds3231_write_reg(uint8_t reg, uint8_t value);
uint8_t ds3231_read_reg(uint8_t reg);
static void ds3231_read_regs(uint8_t reg, uint8_t *buf, uint8_t len);
void ds3231_get_time(rtc_time_t *t);
void ds3231_set_time(const rtc_time_t *t);
void DS3231_SetSQW(DS3231_SQW_Freq freq);
void DS3231_SetAlarm2_EveryMinute(void);
uint8_t DS3231_CheckAlarm2(void);


