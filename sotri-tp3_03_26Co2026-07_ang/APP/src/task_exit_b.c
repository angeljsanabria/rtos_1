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
 * FOR B PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
#define G_TASK_EXIT_B_CNT_INI	0ul

#define TASK_EXIT_B_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_EXIT_B_DEL_MAX		(pdMS_TO_TICKS(2500ul))

/********************** internal data declaration ****************************/
bool is_the_lane_free_2;
static bool should_release_road_cross;
/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_exit_b					= "Task Exit B - Output Gateway A";
const char *p_task_exit_b_wait_2500mS		= "   ==> Task  Exit B - Wait:   2500mS";
const char *p_task_exit_b_wait_exit_b		= "   ==> Task  Exit B - Wait:   Exit B";
const char *p_task_exit_b_wait_mutex		= "   ==> Task  Exit B - Wait:   Mutex";
const char *p_task_exit_b_signal_continue	= "   ==> Task  Exit B - Signal: Continue ==>";
const char *p_task_exit_b_signal_mutex			= "   ==> Task  Exit B - Signal: Mutex    ==>";
const char *p_task_exit_b_g_tasks_b_cnt			= "   <=> Task  Exit B - g_tasks_b_cnt :";
const char *p_task_entry_b_release_road_cross	= "   ==> Task Entry B - Release:   Road cross";
/********************** external data declaration *****************************/
uint32_t g_task_exit_b_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_exit_b(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_exit_b_cnt = G_TASK_EXIT_B_CNT_INI;
	is_the_lane_free_2 = false;
	should_release_road_cross = false;
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_exit_b_cnt++;

		LOGGER_INFO(p_task_exit_b_wait_exit_b);
		xSemaphoreTake(h_exit_b_bin_sem, portMAX_DELAY);
		{
			LOGGER_INFO(p_task_exit_b_wait_mutex);		// "   ==> Task  Exit B - Wait:   Mutex";
			xSemaphoreTake(h_mutex_mut_sem, portMAX_DELAY);

			g_tasks_b_cnt--;
			LOGGER_INFO("  %s %d", p_task_exit_b_g_tasks_b_cnt, (int)g_tasks_b_cnt);

			if (0 == g_tasks_b_cnt) {
				should_release_road_cross = true;
			}

			if ((G_TASKS_CNT_MAX - 1) == g_tasks_b_cnt) {
				/* Set Task Exit B Flag */
				is_the_lane_free_2 = true;
			}

			LOGGER_INFO(p_task_exit_b_signal_mutex);	// "   ==> Task  Exit B - Signal: Mutex    ==>";
			xSemaphoreGive(h_mutex_mut_sem);

			if (true ==is_the_lane_free_2) {
				/* Reset Task Entry B Flag */
				is_the_lane_free_2 = false;

				/* Do it here */
				// Continue . Signal
				LOGGER_INFO(p_task_exit_b_signal_continue);
				xSemaphoreGive(h_continue_bin_sem);
			}

			if (true == should_release_road_cross) {
				LOGGER_INFO(p_task_entry_b_release_road_cross);
				xSemaphoreGive(h_road_crossing_mut_sem);
			}

		}

	}
}

/********************** end of file ******************************************/
