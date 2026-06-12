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
#define G_TASK_TEST_CNT_INI		0ul

#define TASK_TEST_TICK_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_TEST_TICK_DEL_MAX	(pdMS_TO_TICKS(5000ul))

/********************** internal data declaration ****************************/
/* Events to excite tasks */
typedef enum e_task_test {Error, OPEN_REQUEST_A, DOOR_CLOSED_A, OPEN_REQUEST_B, DOOR_CLOSED_B, OPEN_REQUEST_C, DOOR_CLOSED_C, OPEN_REQUEST_D, DOOR_CLOSED_D} e_task_test_t;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_test								= "Periodically excites other tasks";
const char *p_task_test_priority					= "  <=> Task Test - Priority:";
const char *p_task_test_e_task_test_array			= "  <=> Task Test - e_task_test_array:";

const char *p_task_test_signal_error  				= "  <=> Task Test - Signal: Error    <=>";

const char *p_task_test_wait_5000mS					= "  <=> Task Test - Wait:   5000mS";

const char *p_task_test_signal_open_request_a		= "  <=> Task Test - Signal: OPEN_REQUEST_A <=>";
const char *p_task_test_signal_door_closed_a		= "  <=> Task Test - Signal: DOOR_CLOSED_A  <=>";

#define E_TASK_TEST_X (1)

#if (E_TASK_TEST_X == 0)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {Error, Exit_B+1, Exit_B+2};
#endif

#if (E_TASK_TEST_X == 1)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {OPEN_REQUEST_A, DOOR_CLOSED_A, OPEN_REQUEST_A, DOOR_CLOSED_A};
#endif

#if (E_TASK_TEST_X == 2)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {OPEN_REQUEST_A, DOOR_CLOSED_A, OPEN_REQUEST_B, DOOR_CLOSED_B};
#endif

#if (E_TASK_TEST_X == 3)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {OPEN_REQUEST_B, DOOR_CLOSED_B, OPEN_REQUEST_C, DOOR_CLOSED_C};
#endif

#if (E_TASK_TEST_X == 4)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {OPEN_REQUEST_C, DOOR_CLOSED_C, OPEN_REQUEST_D, DOOR_CLOSED_D};
#endif

#if (E_TASK_TEST_X == 5)
/* Array of events to excite tasks */
const e_task_test_t e_task_test_array[] = {OPEN_REQUEST_D, DOOR_CLOSED_D, OPEN_REQUEST_A, DOOR_CLOSED_A};
#endif

/********************** external data declaration *****************************/
uint32_t g_task_test_cnt;

/********************** external functions definition ************************/
/* Task Test thread */
void task_test(void *parameters)
{
	g_task_test_cnt = G_TASK_TEST_CNT_INI;

	/*  Declare & Initialize Task Function variables for argument, led, button and task */
	TickType_t last_wake_time;

	/* The xLastWakeTime variable needs to be initialized with the current tick
	   count. ws*/
	last_wake_time = xTaskGetTickCount();

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* This task will run the first time after other tasks as it has the lower
	 * priority.
	 *
	 * Query the priority at which this task is running - passing in NULL means
	 * "return our own priority". */
	UBaseType_t task_test_priority;
	task_test_priority = uxTaskPriorityGet(NULL) + 2ul;

	/* Setting the TestingTask priority above the other tasks priority will
	 * cause TestingTask to immediately start running (as then TestingTask
	 * will have the higher priority of the three created tasks). */
	vTaskPrioritySet(NULL, task_test_priority);

	/* Print out: Task priority */
	LOGGER_INFO("%s %s %d", p_task_test_priority, pcTaskGetName(NULL), (int)task_test_priority);

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		uint32_t index = E_TASK_TEST_X;

		/* Scanning the array of events to excite tasks */
		for (index = 0; index < (sizeof(e_task_test_array)/sizeof(e_task_test_t)); index++)
		{
			/* Update Task Task Counter */
			g_task_test_cnt++;

			/* Print out: Event Task Test Array Index */
			LOGGER_INFO(" ");
			LOGGER_INFO("%s %s %d", p_task_test_e_task_test_array, GET_NAME(index), (int)index);

			switch (e_task_test_array[index]) {

	    		case OPEN_REQUEST_A:

		    		break;

	    		case DOOR_CLOSED_A:

		    		break;

	    		case OPEN_REQUEST_C:

		    		break;

	    		case DOOR_CLOSED_C:

		    		break;

	    		case OPEN_REQUEST_D:

		    		break;

	    		case DOOR_CLOSED_D:

		    		break;


	    		case Error:
		    	default:

		    		/* Print out: Signal Error */
		    		LOGGER_INFO(p_task_test_signal_error);
		    		break;
		    }

			/* We want this task to execute exactly every 5000 milliseconds. */
			LOGGER_INFO(p_task_test_wait_5000mS);
			vTaskDelayUntil(&last_wake_time, TASK_TEST_TICK_DEL_MAX);
		}
	}
}

/********************** end of file ******************************************/
