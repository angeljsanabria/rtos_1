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
#define G_TASK_ENTRY_A_CNT_INI	0ul

#define TASK_ENTRY_A_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_ENTRY_A_DEL_MAX	(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/
bool is_the_lane_full;
/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_entry_a				    = "Task Entry A - Input Gateway A";
const char *p_task_entry_a_wait_2500mS		= "   ==> Task Entry A - Wait:   2500mS";
const char *p_task_entry_a_wait_entry_a		= "   ==> Task Entry A - Wait:   Entry A";
const char *p_task_entry_a_wait_continue	= "   ==> Task Entry A - Wait:   Continue";
const char *p_task_entry_a_wait_mutex		= "   ==> Task Entry A - Wait:   Mutex";
const char *p_task_entry_a_wait_road_cross	= "   ==> Task Entry A - Wait:   Road cross";
const char *p_task_entry_a_take_road_cross	= "   ==> Task Entry A - Take:   Road cross";
const char *p_task_entry_a_signal_mutex		= "   ==> Task Entry A - Signal: Mutex    ==>";
const char *p_task_entry_a_g_tasks_a_cnt		= "   <=> Task Entry A - g_tasks_a_cnt :";
/********************** external data declaration *****************************/
uint32_t g_task_entry_a_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_entry_a(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_entry_a_cnt = G_TASK_ENTRY_A_CNT_INI;
	is_the_lane_full = false;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* Take the semaphore once to start with so the semaphore is empty before the
	 * infinite loop is entered.  The semaphore was created before the scheduler
	 * was started so before this task ran for the first time.*/
    xSemaphoreTake(h_entry_a_bin_sem, (portTickType) 0);	// h_entry_a_bin_sem = Semaphore(0)
    xSemaphoreTake(h_exit_a_bin_sem, (portTickType) 0);		// h_exitry_a_bin_sem = Semaphore(0)
    xSemaphoreTake(h_entry_b_bin_sem, (portTickType) 0);	// h_entry_b_bin_sem = Semaphore(0)
    xSemaphoreTake(h_exit_b_bin_sem, (portTickType) 0);		// h_exitry_b_bin_sem = Semaphore(0)
	xSemaphoreTake(h_continue_bin_sem, (portTickType) 0);	// h_continue_bin_sem = Semaphore(0)

    /* Setting the priority of Task A above the priority of other tasks will
     * cause Task A to start executing immediately, allowing it to put the
     * semaphores into the initial state required by the application, and then
     * regain the same priority of other tasks. */
	vTaskPrioritySet(h_task_entry_a, (uxTaskPriorityGet(h_task_exit_a)));

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_entry_a_cnt++;

		LOGGER_INFO(p_task_entry_a_wait_entry_a);
		xSemaphoreTake(h_entry_a_bin_sem, portMAX_DELAY);

		{

			LOGGER_INFO(p_task_entry_a_wait_mutex);
			xSemaphoreTake(h_mutex_mut_sem, portMAX_DELAY);

			if (g_tasks_a_cnt == 0) {
				LOGGER_INFO(p_task_entry_a_wait_mutex);
				xSemaphoreGive(h_mutex_mut_sem);

				LOGGER_INFO(p_task_entry_a_wait_road_cross);
				xSemaphoreTake(h_road_crossing_mut_sem, portMAX_DELAY);
				LOGGER_INFO(p_task_entry_a_take_road_cross);

				LOGGER_INFO(p_task_entry_a_wait_mutex);
				xSemaphoreTake(h_mutex_mut_sem, portMAX_DELAY);
			}

			g_tasks_a_cnt++;
			LOGGER_INFO("%s %d", p_task_entry_a_g_tasks_a_cnt, (int)g_tasks_a_cnt);

			if (G_TASKS_CNT_MAX == g_tasks_a_cnt) {
				/* Set Task Entry A Flag */
				is_the_lane_full = true;
			}

			LOGGER_INFO(p_task_entry_a_signal_mutex);	// "   ==> Task  Exit A - Signal: Mutex    ==>";
			xSemaphoreGive(h_mutex_mut_sem);

			if (true == is_the_lane_full) {
				is_the_lane_full = false;

				LOGGER_INFO(p_task_entry_a_wait_continue);
				xSemaphoreTake(h_continue_bin_sem, portMAX_DELAY);
			}

		}

	}
}

/********************** end of file ******************************************/
