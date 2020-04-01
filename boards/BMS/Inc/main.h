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
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Io_SharedErrorHandlerOverride.h"
    /* USER CODE END Includes */

    /* Exported types
     * ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants
     * --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro
     * ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes
     * ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IWDG_WINDOW_DISABLE_VALUE 4095
#define IWDG_PRESCALER 4
#define IWDG_RESET_FREQUENCY 5
#define LSI_FREQUENCY 40000
#define TASK1HZ_STACK_SIZE 128
#define TASK1KHZ_STACK_SIZE 128
#define TASKCANTX_STACK_SIZE 128
#define TASKCANRX_STACK_SIZE 128
#define TIM2_FREQUENCY 72000000
#define TIM2_AUTO_RELOAD_REG 0xFFFF
#define TIM2_PWM_MINIMUM_FREQUENCY 1
#define TIM2_PRESCALER \
    (TIM2_FREQUENCY / TIM2_AUTO_RELOAD_REG / TIM2_PWM_MINIMUM_FREQUENCY)
#define IMD_M_HS_Pin GPIO_PIN_1
#define IMD_M_HS_GPIO_Port GPIOA
#define __24V_ISENSE_Pin GPIO_PIN_3
#define __24V_ISENSE_GPIO_Port GPIOA
#define STATUS_R_Pin GPIO_PIN_8
#define STATUS_R_GPIO_Port GPIOA
#define STATUS_G_Pin GPIO_PIN_9
#define STATUS_G_GPIO_Port GPIOA
#define STATUS_B_Pin GPIO_PIN_10
#define STATUS_B_GPIO_Port GPIOA
    /* USER CODE BEGIN Private defines */

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
