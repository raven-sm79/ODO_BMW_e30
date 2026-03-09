#include "speed.h"
#include "stm32f3xx_hal.h"

static volatile uint32_t s_pulses = 0;      // остаток импульсов до 1 км (0..SPEED_PPK-1)
static volatile uint8_t  s_km_flag = 0;
static volatile uint32_t s_km_pending = 0;  // счётчик, а не флаг

extern TIM_HandleTypeDef htim1;

void SPEED_Init(uint16_t pulse_rem)
{
    s_pulses = (pulse_rem < SPEED_PPK) ? pulse_rem : 0;
    s_km_flag = 0;
    // стартуем захват на CH4
    HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_4);
}

void SPEED_OnPulseFilteredISR(void)
{
    uint32_t p = s_pulses + 1;
    if (p >= SPEED_PPK) {
        p -= SPEED_PPK;
        s_km_pending++;  // атомарно для 32-bit на Cortex-M
    }
    s_pulses = p;
}
uint32_t SPEED_GetKmPending(void)
{
    uint32_t pending;
    __disable_irq();
    pending = s_km_pending;
    s_km_pending = 0;
    __enable_irq();
    return pending;
}

//void SPEED_ConsumeKm(uint32_t n) { s_km_pending -= n; }

/*
void SPEED_OnPulseFilteredISR(void)
{
    uint32_t p = s_pulses + 1;
    if (p >= SPEED_PPK) { p -= SPEED_PPK; s_km_flag = 1; }
    s_pulses = p;
}
*/
uint8_t SPEED_KmTickPending(void) { return s_km_flag; }

void SPEED_ConsumeKmTick(void) { s_km_flag = 0; }

uint16_t SPEED_GetPulseRem(void) { return (uint16_t)s_pulses; }

void SPEED_SetPulseRem(uint16_t rem)
{
    s_pulses = (rem < SPEED_PPK) ? rem : 0;
}


