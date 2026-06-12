/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"

/********************** macros and definitions *******************************/
#define G_TASK_GATE_C_CNT_INI	0ul

#define TASK_GATE_C_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_GATE_C_DEL_MAX		(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_gate_c_wait_2500mS					= "   ==> Task Gate C  - Wait:   2500mS";
const char *p_task_entry_c_wait_entry					= "   ==> Task C - Wait:   Entry C - Esperando Solicitud de apertura puerta C";
const char *p_task_entry_c_signal_entry					= "   ==> Task C - Signal:   Entry C - Apertura puerta C solicitado";
const char *p_task_entry_c_wait_control_esclusa			= "   ==> Task C - Wait Mutex:   Control esclusa C - Para abrir puerta C";
const char *p_task_c_con_control_de_esclusa				= "   ==> Task C - Taken Mutex:   Control esclusa C - Para abrir puerta C";
const char *p_task_c_con_control_de_esclusa_open_door	= "   ==> Task C - Accion:   Control esclusa C - Puerta C bierta";
const char *p_task_entry_c_wait_exit					= "   ==> Task C - Wait:   Exit C - Esperando Cierre de puerta C";
const char *p_task_entry_c_signal_exit					= "   ==> Task C - Signal:   Exit C - Cierre de apertura C realizado";
const char *p_task_c_libera_control_de_esclusa			= "   ==> Task C - Signal Mutex:   Control esclusa C - Liberado por C\r\n";
const char *p_task_c_ERROR								= "   ==> Task C - ERROR";

/********************** external data declaration *****************************/
uint32_t g_task_gate_c_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_gate_c(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_gate_c_cnt = G_TASK_GATE_C_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_gate_c_cnt++;

		LOGGER_INFO(p_task_entry_c_wait_entry);
		xSemaphoreTake(h_entry_c_bin_sem, portMAX_DELAY);		// Esperando señal apertura puerta C
		{
			LOGGER_INFO(p_task_entry_c_signal_entry);
			LOGGER_INFO(p_task_entry_c_wait_control_esclusa);
			xSemaphoreTake(h_mutex_control_esclusa_sem, portMAX_DELAY);
			{
				LOGGER_INFO(p_task_c_con_control_de_esclusa);

				if(g_gate_open == gate_open_NONE){
					g_gate_open = gate_open_C;
				}else{
					LOGGER_INFO(p_task_c_ERROR);
				}
				LOGGER_INFO(p_task_c_con_control_de_esclusa_open_door);
			}

			LOGGER_INFO(p_task_entry_c_wait_exit);
			xSemaphoreTake(h_exit_c_bin_sem, portMAX_DELAY);	// Esperando señal cierre puerta C
			{
				if(g_gate_open != gate_open_C){
					LOGGER_INFO(p_task_c_ERROR);
				}else{
					g_gate_open = gate_open_NONE;
				}
				LOGGER_INFO(p_task_entry_c_signal_exit);
			}
		}
		// SIGNAL DE MUTEX
		xSemaphoreGive(h_mutex_control_esclusa_sem);
		LOGGER_INFO(p_task_c_libera_control_de_esclusa);

//    	/* Print out: Wait 2500mS */
//		LOGGER_INFO(p_task_gate_c_wait_2500mS);
//		vTaskDelay(TASK_GATE_C_DEL_MAX);
	}
}

/********************** end of file ******************************************/
