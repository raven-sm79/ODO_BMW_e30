/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

extern volatile uint8_t sqw_tick;
extern volatile uint8_t alarm2_flag;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KeyD_Pin GPIO_PIN_0
#define KeyD_GPIO_Port GPIOA
#define KeyUp_Pin GPIO_PIN_2
#define KeyUp_GPIO_Port GPIOA
#define KeySel_Pin GPIO_PIN_4
#define KeySel_GPIO_Port GPIOA
#define DS18B20_Pin GPIO_PIN_10
#define DS18B20_GPIO_Port GPIOB
#define SpeedInput_Pin GPIO_PIN_11
#define SpeedInput_GPIO_Port GPIOB
#define ACC_Pin GPIO_PIN_13
#define ACC_GPIO_Port GPIOB
#define PowerUP_Pin GPIO_PIN_15
#define PowerUP_GPIO_Port GPIOB
#define TimeINT_Pin GPIO_PIN_6
#define TimeINT_GPIO_Port GPIOC
#define Soft_I_C_SCL_Pin GPIO_PIN_6
#define Soft_I_C_SCL_GPIO_Port GPIOB
#define Soft_I_C_SDA_Pin GPIO_PIN_7
#define Soft_I_C_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
