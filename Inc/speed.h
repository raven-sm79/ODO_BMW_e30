#pragma once
#include <stdint.h>

#define SPEED_PPK 4838u  // impulses per kilometer (пока так; потом калибруем) 4838u

void     SPEED_Init(uint16_t pulse_rem);
void 	 SPEED_OnPulseFilteredISR(void);

uint32_t SPEED_GetKmPending(void);
void SPEED_ConsumeKm(uint32_t n);

uint16_t SPEED_GetPulseRem(void);
void     SPEED_SetPulseRem(uint16_t rem);
