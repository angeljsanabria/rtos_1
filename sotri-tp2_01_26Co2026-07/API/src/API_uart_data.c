/*
 * API_delay.c
 *
 *  Created on: 19 mar 2026
 *      Author: angel-dev
 */
#include "API_uart_data.h"
#include "stm32l4xx_hal.h"
#include <stdio.h>
#include "string.h"

#define TEST_UART_MSG	"ACCESO INICIADO"
/**
  * @brief Handler de UART3 - usada para datos
  */
static UART_HandleTypeDef huart3;

bool_t API_uart_data_init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 57600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
//    Error_Handler();
	  return false;
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
	  return false;
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
	  return false;
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
	  return false;
  }

  return true;

}

void API_uart_data_test(void){
	HAL_UART_Transmit(&huart3, (uint8_t*)TEST_UART_MSG, strlen(TEST_UART_MSG), HAL_MAX_DELAY);
}


void uartSendString(uint8_t * pstring){
	if(pstring ==  NULL)	return;

	size_t lenStr = strlen((char*)pstring);

	if((lenStr == 0) || (lenStr >= UART_DATA_MAX_SIZE)) return;

	HAL_UART_Transmit(&huart3, pstring, lenStr, HAL_MAX_DELAY);
}


void uartSendStringSize(uint8_t * pstring, uint16_t size){
	if((pstring == NULL) || (size == 0) || (size >= UART_DATA_MAX_SIZE))	return;

	HAL_UART_Transmit(&huart3, pstring, size, HAL_MAX_DELAY);
}


bool_t uartReceiveStringSize(uint8_t * pstring, uint16_t size){
	if((pstring == NULL) || (size == 0))	return false;

	HAL_StatusTypeDef ret = HAL_UART_Receive(&huart3, pstring, size, UART_DATA_MIN_RX_TIMEOUT);

	if (ret != HAL_OK)	return false;
	return true;
}

void uartSendCardToAuthorize(uint8_t *card, uint16_t size)
{
	char aux[32];
	int n;

	if ((card == NULL) || (size == 0) || (size >= UART_DATA_MAX_SIZE)) {
		return;
	}

	if (size != 8) {
		uartSendString((uint8_t *)"CARD No soportada\n");
		return;
	}

	n = snprintf(aux, sizeof(aux),
	    "CARD%c%c%c%c%c%c%c%c\n",
	    (int)card[0], (int)card[1], (int)card[2], (int)card[3],
	    (int)card[4], (int)card[5], (int)card[6], (int)card[7]);
	if (n < 0 || (size_t)n >= sizeof(aux)) {
		return;
	}

	HAL_UART_Transmit(&huart3, (uint8_t *)aux, (uint16_t)n, HAL_MAX_DELAY);
}

