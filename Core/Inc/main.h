/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct _STM32_GPIO_STRUCT_
{
	GPIO_TypeDef *port;
	uint16_t pin;
} gpio_struct_t;
extern volatile _Bool os_running;
extern volatile _Bool standalone;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define GET_IDX(varx, vary, varw)	( (vary * varw) + varx )
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define COL_0_Pin GPIO_PIN_0
#define COL_0_GPIO_Port GPIOC
#define COL_1_Pin GPIO_PIN_1
#define COL_1_GPIO_Port GPIOC
#define COL_2_Pin GPIO_PIN_2
#define COL_2_GPIO_Port GPIOC
#define COL_3_Pin GPIO_PIN_3
#define COL_3_GPIO_Port GPIOC
#define COL_8_Pin GPIO_PIN_0
#define COL_8_GPIO_Port GPIOA
#define COL_9_Pin GPIO_PIN_1
#define COL_9_GPIO_Port GPIOA
#define COL_10_Pin GPIO_PIN_2
#define COL_10_GPIO_Port GPIOA
#define COL_13_Pin GPIO_PIN_4
#define COL_13_GPIO_Port GPIOA
#define COL_14_Pin GPIO_PIN_5
#define COL_14_GPIO_Port GPIOA
#define COL_15_Pin GPIO_PIN_6
#define COL_15_GPIO_Port GPIOA
#define COL_16_Pin GPIO_PIN_7
#define COL_16_GPIO_Port GPIOA
#define COL_17_Pin GPIO_PIN_4
#define COL_17_GPIO_Port GPIOC
#define COL_18_Pin GPIO_PIN_5
#define COL_18_GPIO_Port GPIOC
#define COL_19_Pin GPIO_PIN_0
#define COL_19_GPIO_Port GPIOB
#define LD0_Pin GPIO_PIN_1
#define LD0_GPIO_Port GPIOB
#define LD1_Pin GPIO_PIN_2
#define LD1_GPIO_Port GPIOB
#define LD2_Pin GPIO_PIN_10
#define LD2_GPIO_Port GPIOB
#define ROW_7_Pin GPIO_PIN_13
#define ROW_7_GPIO_Port GPIOB
#define ROW_6_Pin GPIO_PIN_14
#define ROW_6_GPIO_Port GPIOB
#define ROW_5_Pin GPIO_PIN_15
#define ROW_5_GPIO_Port GPIOB
#define ROW_4_Pin GPIO_PIN_6
#define ROW_4_GPIO_Port GPIOC
#define ROW_3_Pin GPIO_PIN_7
#define ROW_3_GPIO_Port GPIOC
#define ROW_2_Pin GPIO_PIN_8
#define ROW_2_GPIO_Port GPIOC
#define ROW_1_Pin GPIO_PIN_9
#define ROW_1_GPIO_Port GPIOC
#define ROW_0_Pin GPIO_PIN_8
#define ROW_0_GPIO_Port GPIOA
#define COL_7_Pin GPIO_PIN_4
#define COL_7_GPIO_Port GPIOB
#define COL_6_Pin GPIO_PIN_5
#define COL_6_GPIO_Port GPIOB
#define COL_12_Pin GPIO_PIN_6
#define COL_12_GPIO_Port GPIOB
#define COL_11_Pin GPIO_PIN_7
#define COL_11_GPIO_Port GPIOB
#define COL_5_Pin GPIO_PIN_8
#define COL_5_GPIO_Port GPIOB
#define COL_4_Pin GPIO_PIN_9
#define COL_4_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
