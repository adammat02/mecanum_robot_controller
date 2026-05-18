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
#include "stm32g4xx_hal.h"

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
#define PWM_PSC 24
#define PWM_ARR 255
#define UTIM_PSC 169
#define ADCTR_PSC 16999
#define ADCTR_ARR 999
#define GPIO3_Pin GPIO_PIN_13
#define GPIO3_GPIO_Port GPIOC
#define GPIO2_Pin GPIO_PIN_14
#define GPIO2_GPIO_Port GPIOC
#define GPIO1_Pin GPIO_PIN_15
#define GPIO1_GPIO_Port GPIOC
#define M1_DIR_Pin GPIO_PIN_12
#define M1_DIR_GPIO_Port GPIOB
#define M2_DIR_Pin GPIO_PIN_13
#define M2_DIR_GPIO_Port GPIOB
#define M3_DIR_Pin GPIO_PIN_14
#define M3_DIR_GPIO_Port GPIOB
#define M4_DIR_Pin GPIO_PIN_15
#define M4_DIR_GPIO_Port GPIOB
#define TOF2_XSHUT_Pin GPIO_PIN_10
#define TOF2_XSHUT_GPIO_Port GPIOC
#define TOF1_XSHUT_Pin GPIO_PIN_11
#define TOF1_XSHUT_GPIO_Port GPIOC
#define LED_Pin GPIO_PIN_12
#define LED_GPIO_Port GPIOC
#define GPIO5_Pin GPIO_PIN_2
#define GPIO5_GPIO_Port GPIOD
#define GPIO4_Pin GPIO_PIN_3
#define GPIO4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
