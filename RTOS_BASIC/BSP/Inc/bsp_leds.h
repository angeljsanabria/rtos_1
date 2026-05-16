/**
 ******************************************************************************
 * @file    bsp_leds.h
 * @brief   Board Support Package - LED driver
 * @details This file provides functions to control the board LEDs.
 ******************************************************************************
 */

#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* LED Definitions */
#define BSP_LED2                0
#define BSP_LED3                1

/* LED Pin Definitions (extracted from generated project) */
#define BSP_LED2_PIN            GPIO_PIN_7
#define BSP_LED2_PORT           GPIOB
#define BSP_LED3_PIN            GPIO_PIN_14
#define BSP_LED3_PORT           GPIOB

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void BSP_LED_Init(void);
void BSP_LED_On(uint8_t led);
void BSP_LED_Off(uint8_t led);
void BSP_LED_Toggle(uint8_t led);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
