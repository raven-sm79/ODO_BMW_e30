#include "soft_i2c.h"

#define SCL_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)
#define SCL_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)
#define SDA_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)
#define SDA_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET)
#define SDA_READ() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)

static void i2c_delay(void)
{
    for (volatile int i = 0; i < 40; i++);
}

void i2c_start(void)
{
    SDA_H(); SCL_H(); i2c_delay();
    SDA_L(); i2c_delay();
    SCL_L();
}

void i2c_stop(void)
{
    SDA_L(); SCL_H(); i2c_delay();
    SDA_H(); i2c_delay();
}

uint8_t i2c_write(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        (data & 0x80) ? SDA_H() : SDA_L();
        SCL_H(); i2c_delay();
        SCL_L(); i2c_delay();
        data <<= 1;
    }

    SDA_H();
    SCL_H(); i2c_delay();
    uint8_t ack = !SDA_READ();
    SCL_L();

    return ack;
}

uint8_t i2c_read(uint8_t ack)
{
    uint8_t data = 0;
    SDA_H();

    for (int i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_H(); i2c_delay();
        if (SDA_READ()) data |= 1;
        SCL_L(); i2c_delay();
    }

    ack ? SDA_L() : SDA_H();
    SCL_H(); i2c_delay();
    SCL_L();
    SDA_H();

    return data;
}
