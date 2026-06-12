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
#define G_TASK_ENTRY_B_CNT_INI	0ul

#define TASK_ENTRY_B_DEL_MIN	(pdMS_TO_TICKS(0ul))
#define TASK_ENTRY_B_DEL_MAX	(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_entry_b				= "Task Entry B - Input/Output Gateway B";

const char *p_task_entry_b_wait_2500mS	= "   ==> Task Entry B - Wait:   2500mS";

const char *p_task_entry_b_wait_entry_b	= "   ==> Task Entry B - Wait:   Entry_B";
const char *p_task_entry_b_wait_exit_b	= "   ==> Task Entry B - Wait:   Exit_B";

const char *p_task_entry_b_wait_mutex	= "   ==> Task Entry B - Wait:   Mutex";
const char *p_task_entry_b_signal_mutex	= "   ==> Task Entry B - Signal: Mutex    ==>";

/********************** external data declaration *****************************/
uint32_t g_task_entry_b_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_entry_b(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_entry_b_cnt = G_TASK_ENTRY_B_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_entry_b_cnt++;

		// Entry_B. Wait
		LOGGER_INFO(p_task_entry_b_wait_entry_b);		// "   ==> Task Entry B - Wait:   Entry_B";
		xSemaphoreTake(h_entry_b_bin_sem, portMAX_DELAY);
		{
			// Mutex . Wait
    		LOGGER_INFO(p_task_entry_b_wait_mutex);
			xSemaphoreTake(h_mutex_mut_sem, portMAX_DELAY);

			// Statement

			// Exit_B. Wait
			LOGGER_INFO(p_task_entry_b_wait_exit_b);	// "   ==> Task Entry B - Wait:   Exit_B";
			xSemaphoreTake(h_exit_b_bin_sem, portMAX_DELAY);
			{
				// Mutex . Signal
	    		LOGGER_INFO(p_task_entry_b_signal_mutex);
				xSemaphoreGive(h_mutex_mut_sem);

				// Statement
			}
		}
	}
}

/********************** end of file ******************************************/
