/*
 * ds3231.c
 *
 *  Created on: Jan 2, 2026
 *      Author: user
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ds3231.h"
#include "colors.h"
#include "soft_i2c.h"

#define DS3231_ADDR      0x68

/* Registers */
#define DS3231_REG_SEC   0x00
#define DS3231_REG_MIN   0x01
#define DS3231_REG_HOUR  0x02
#define DS3231_REG_DOW   0x03
#define DS3231_REG_DATE  0x04
#define DS3231_REG_MON   0x05
#define DS3231_REG_YEAR  0x06

#define DS3231_REG_A2_MIN     0x0B
#define DS3231_REG_A2_HOUR    0x0C
#define DS3231_REG_A2_DAY     0x0D
#define DS3231_REG_CTRL      0x0E
#define DS3231_REG_STATUS    0x0F

static uint8_t last_hh = 0xFF;
static uint8_t last_mm = 0xFF;

static uint8_t bcd2bin(uint8_t v) {
	return (v >> 4) * 10 + (v & 0x0F);
}

static uint8_t bin2bcd(uint8_t v) {
	return ((v / 10) << 4) | (v % 10);
}

void ds3231_write_reg(uint8_t reg, uint8_t value)
{
    i2c_start();
    // адрес + write
    i2c_write((DS3231_ADDR << 1) | 0);
    // адрес регистра
    i2c_write(reg);
    // данные
    i2c_write(value);
    i2c_stop();
}

uint8_t ds3231_read_reg(uint8_t reg) {
    uint8_t value;
    i2c_start();
    i2c_write((DS3231_ADDR << 1) | 0);
    i2c_write(reg);
    i2c_start();
    i2c_write((DS3231_ADDR << 1) | 1);
    value = i2c_read(0);
    i2c_stop();
    return value;
}

static void ds3231_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
	i2c_start();
	i2c_write(DS3231_ADDR << 1);
	i2c_write(reg);
	i2c_start();
	i2c_write((DS3231_ADDR << 1) | 1);

	while (len--)
		*buf++ = i2c_read(len ? 1 : 0);

	i2c_stop();
}

void DS3231_SetSQW(DS3231_SQW_Freq freq)
{
    uint8_t ctrl = ds3231_read_reg(DS3231_REG_CTRL);

    ctrl &= ~(1 << 2);        // INTCN = 0
    ctrl &= ~(3 << 3);        // RS1 RS2
    ctrl |= (freq << 3);

    ds3231_write_reg(DS3231_REG_CTRL, ctrl);
}

void DS3231_SetAlarm2_EveryMinute(void)
{
	ds3231_write_reg(DS3231_REG_A2_MIN,  0x80);
	ds3231_write_reg(DS3231_REG_A2_HOUR, 0x80);
	ds3231_write_reg(DS3231_REG_A2_DAY,  0x80);

    uint8_t ctrl = ds3231_read_reg(DS3231_REG_CTRL);
    ctrl |= (1 << 1); // A2IE
    ds3231_write_reg(DS3231_REG_CTRL, ctrl);
}

uint8_t DS3231_CheckAlarm2(void)
{
    uint8_t status = ds3231_read_reg(DS3231_REG_STATUS);

    if (status & (1 << 1)) // A2F
    {
        status &= ~(1 << 1);
        ds3231_write_reg(DS3231_REG_STATUS, status);
        return 1;
    }

    return 0;
}

void ds3231_get_time(rtc_time_t *t) {
	uint8_t buf[7];

	//ds3231_read_regs(DS3231_REG_SEC, buf, 7);
	ds3231_read_regs(0x00, buf, 7);

	t->sec = bcd2bin(buf[0] & 0x7F);
	t->min = bcd2bin(buf[1]);
	t->hour  = bcd2bin(buf[2] & 0x3F);
	t->day = bcd2bin(buf[3]);
	t->date = bcd2bin(buf[4]);
	t->month = bcd2bin(buf[5] & 0x1F);
	t->year = 2000 + bcd2bin(buf[6]);
}

void ds3231_set_time(const rtc_time_t *t) {
	i2c_start();
	i2c_write(DS3231_ADDR << 1);
	i2c_write(DS3231_REG_SEC);

	i2c_write(bin2bcd(t->sec) & 0x7F);   // CH = 0
	i2c_write(bin2bcd(t->min));
	i2c_write(bin2bcd(t->hour));
	i2c_write(bin2bcd(t->day));
	i2c_write(bin2bcd(t->date));
	i2c_write(bin2bcd(t->month));
	i2c_write(bin2bcd(t->year - 2000));

	i2c_stop();
}

void ds3231_init_SQH_1Hz(void)
{
// CONTROL register 0x0E
// INTCN = 0 → SQW enabled
// RS2 RS1 = 00 → 1 Hz
    i2c_start();
    i2c_write(DS3231_ADDR << 1);
    i2c_write(0x0E);              // CONTROL
	ds3231_write_reg(0x0E, 0b00000000);
    i2c_stop();
}

uint8_t ds3231_alarm2_fired(void)
{
    uint8_t st = ds3231_read_reg(0x0F);
    return (st & (1 << 1)) ? 1 : 0;
}

void ds3231_init_alarm2_every_minute(void)
{
    uint8_t st;

    /* === CONTROL: INTCN=1, A2IE=1, SQW OFF === */
    // bit2 INTCN = 1
    // bit1 A2IE  = 1
    ds3231_write_reg(0x0E, 0b00000110);

    /* === Alarm2: every minute === */
    ds3231_write_reg(0x0B, 0x80); // A2MIN  (A2M2=1)
    ds3231_write_reg(0x0C, 0x80); // A2HOUR (A2M3=1)
    ds3231_write_reg(0x0D, 0x80); // A2DAY  (A2M4=1)

    /* === Clear Alarm2 flag === */
    st = ds3231_read_reg(0x0F);
    st &= ~(1 << 1);              // clear A2F
    ds3231_write_reg(0x0F, st);
}

void ds3231_clear_alarm2(void)
{
    uint8_t st = ds3231_read_reg(0x0F);
    st &= ~(1 << 1);              // clear A2F
    ds3231_write_reg(0x0F, st);
}
