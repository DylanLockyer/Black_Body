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
#include "switches.h"
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
#define sel_v_1ma_Pin GPIO_PIN_13
#define sel_v_1ma_GPIO_Port GPIOC
#define i_source_sel4_Pin GPIO_PIN_15
#define i_source_sel4_GPIO_Port GPIOC
#define ADC_DRDY_Pin GPIO_PIN_0
#define ADC_DRDY_GPIO_Port GPIOC
#define i_source_sel3_Pin GPIO_PIN_1
#define i_source_sel3_GPIO_Port GPIOC
#define i_source_sel2_Pin GPIO_PIN_2
#define i_source_sel2_GPIO_Port GPIOC
#define i_sense_sel1_Pin GPIO_PIN_3
#define i_sense_sel1_GPIO_Port GPIOC
#define i_sense_sel2_Pin GPIO_PIN_0
#define i_sense_sel2_GPIO_Port GPIOA
#define i_sense_sel3_Pin GPIO_PIN_1
#define i_sense_sel3_GPIO_Port GPIOA
#define hbridge_sel4_Pin GPIO_PIN_2
#define hbridge_sel4_GPIO_Port GPIOA
#define i_sense_sel4_Pin GPIO_PIN_3
#define i_sense_sel4_GPIO_Port GPIOA
#define hbridge_sel3_Pin GPIO_PIN_4
#define hbridge_sel3_GPIO_Port GPIOA
#define hbridge_sel1_Pin GPIO_PIN_5
#define hbridge_sel1_GPIO_Port GPIOA
#define hbridge_sel2_Pin GPIO_PIN_6
#define hbridge_sel2_GPIO_Port GPIOA
#define sel_10na_pos_Pin GPIO_PIN_7
#define sel_10na_pos_GPIO_Port GPIOA
#define sel_100na_pos_Pin GPIO_PIN_4
#define sel_100na_pos_GPIO_Port GPIOC
#define i_source_sel1_Pin GPIO_PIN_5
#define i_source_sel1_GPIO_Port GPIOC
#define sel_100na_neg_Pin GPIO_PIN_0
#define sel_100na_neg_GPIO_Port GPIOB
#define sel_t_1ma_Pin GPIO_PIN_1
#define sel_t_1ma_GPIO_Port GPIOB
#define sel_t_1ua_Pin GPIO_PIN_2
#define sel_t_1ua_GPIO_Port GPIOB
#define sel_10na_neg_Pin GPIO_PIN_10
#define sel_10na_neg_GPIO_Port GPIOB
#define ADC_CLKIN_Pin GPIO_PIN_8
#define ADC_CLKIN_GPIO_Port GPIOA
#define sel_v_10ua_Pin GPIO_PIN_2
#define sel_v_10ua_GPIO_Port GPIOD
#define sel_v_100ua_Pin GPIO_PIN_4
#define sel_v_100ua_GPIO_Port GPIOB
#define sel_v_1ua_Pin GPIO_PIN_5
#define sel_v_1ua_GPIO_Port GPIOB
#define sel_t_100ua_Pin GPIO_PIN_7
#define sel_t_100ua_GPIO_Port GPIOB
#define sel_t_10ua_Pin GPIO_PIN_9
#define sel_t_10ua_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
