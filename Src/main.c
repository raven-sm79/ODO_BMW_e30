#include "main.h"
#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "fsmc.h"
#include "ST7789V.h"
#include "ds3231.h"
#include "UI.h"
#include "ui_draw.h"


void SystemClock_Config(void);

volatile uint8_t sqw_tick = 0;
volatile uint8_t rtc_minute_event = 0;
volatile uint8_t alarm2_flag = 0;

int main(void) {
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_ADC1_Init();

	rtc_time_t rtc;
	ds3231_get_time(&rtc);
	ds3231_init_alarm2_every_minute();
	ds3231_clear_alarm2();

	MX_FSMC_Init();
	st7789_init();
	ui_draw_static();
	//ui_draw_time(rtc.hour, rtc.min);
	ui_draw_time(88, 88);


	uint16_t adc_buf[2];
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc_buf, 2);




	static uint8_t sec = 0;

	while (1)
	{
		if (rtc_minute_event)
		{
		    rtc_minute_event = 0;

		    // 1. Остановить FSMC
		    fsmc_pause();        // или disable FSMC clocks / CS

		    __disable_irq();     // защитить soft I2C тайминги

		    // 2. Прочитать время
		    ds3231_get_time(&rtc);

		    // 3. Сбросить флаг Alarm2
		    ds3231_clear_alarm2();

		    __enable_irq();

		    // 4. Запустить FSMC
		    fsmc_resume();

		    // 5. Обновить UI
		    //ui_draw_time(rtc.hour, rtc.min);
		}
	}
}



void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

void Error_Handler(void) {
	__disable_irq();
	while (1) {
	}
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE*/
}
#endif /* USE_FULL_ASSERT */
