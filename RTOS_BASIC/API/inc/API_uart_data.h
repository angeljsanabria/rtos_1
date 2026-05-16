/*
 * API_uart_data.h
 *
 *  Created on: 19 mar 2026
 *      Author: angel-dev
 */

#ifndef API_INC_API_UART_DATA_H_
#define API_INC_API_UART_DATA_H_

#include <stdint.h>
#include <stdbool.h>

typedef bool bool_t;

#define CR_							0x0d			// Retorno de carro \r
#define LF_							0x0a			// Salto de linea \r
#define ENDSTR						LF_				// Tomo como fin de string el salto de linea
#define UART_DATA_MAX_SIZE			256
#define UART_DATA_MAX_TIMEOUT		HAL_MAX_DELAY	// Maximo de timeout para la funcion de Send por HAL
#define UART_DATA_MIN_RX_TIMEOUT	10				// Minimo de timeout para la funcion de Rx de HAL


/**
 * @brief 	Inicializa de uart para datos (UART 3).
 * @param
 * @param
 */
bool_t API_uart_data_init(void);

/**
 * @brief 	Envia un string por UART
 * 			debe cumplir con el formato string (finalizar con \0)
 * @param
 * @param
 */
void uartSendString(uint8_t * pstring);

/**
 * @brief 	Envia una cantidad de array de caracteres por UART
 * 			debe cumplir con el formato string (finalizar con \0)
 * @param	pstring puntero del array de caracteres a enviar
 * @param	size indica la cantidad de caracteres a enviar
 */
void uartSendStringSize(uint8_t * pstring, uint16_t size);

/**
 * @brief 	Recibe una cantidad de caracteres por UART
 * 			Se tiene un timeout de recepcion indicado por UART_DATA_MIN_RX_TIMEOUT
 *
 * @param	pstring puntero del array de caracteres recibidos
 * @param	size indica la cantidad de caracteres a recibir
 * @retval 	true  Si se recibieron la cantidad de caracteres antes de cumplir el timeout
 * @retval 	false No se recibieron la cantidad de caracteres esperados en el tiemout
 */
bool_t uartReceiveStringSize(uint8_t * pstring, uint16_t size);


/**
 * @brief 	Envia el formato esperado por el autorizador
 * 			CARD%02X%02X%02X%02X\n
 * 			(finalizar con \n)
  * 			Si size != 8 envia "CARD No soportada\n".
 * @param	card datos de la tarjeta (8 bytes)
 * @param	size debe ser 8 para trama CARD; si no, mensaje de no soportada
 */
void uartSendCardToAuthorize(uint8_t *card, uint16_t size);


/**
 * @brief	Mensaje de testing/inicio
 * @param
 * @param
 */
void API_uart_data_test(void);

#endif /* API_INC_API_UART_DATA_H_ */
