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
#include "app_it.h"
#include "task_gate_a.h"
#include "task_gate_b.h"
#include "task_gate_c.h"
#include "task_gate_d.h"
#include "task_test.h"

/********************** macros and definitions *******************************/
#define G_APP_CNT_INI					0ul
#define G_APP_TASK_CNT_INI				0ul
#define G_APP_TICK_CNT_INI				0ul
#define G_TASK_IDLE_CNT_INI				0ul
#define G_APP_STACK_OVERFLOW_CNT_INI	0ul
#define G_TASKS_CNT_INI					0ul

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_app	= "RTOS - Event-Triggered Systems (ETS)";
const char *p_app_	= "sotri-tp3_04-application: Security airlock";
const char *p_app__	= "(Source => CESE - Sistemas Operativos de Tiempo Real)";

/********************** external data declaration ****************************/
uint32_t g_app_cnt;
uint32_t g_app_task_cnt;
uint32_t g_app_tick_cnt;
uint32_t g_task_idle_cnt;
uint32_t g_app_stack_overflow_cnt;

uint32_t g_tasks_cnt;
uint32_t g_gate_open;

/* Declare a variable of type QueueHandle_t. This is used to reference queues*/

/* Declare a variable of type SemaphoreHandle_t (binary or counting) or mutex.
 * This is used to reference the semaphore that is used to synchronize a thread
 * with other thread or to ensure mutual exclusive access to...*/
// Declaracion de semaforos
SemaphoreHandle_t h_entry_a_bin_sem;
SemaphoreHandle_t h_exit_a_bin_sem;
SemaphoreHandle_t h_entry_b_bin_sem;
SemaphoreHandle_t h_exit_b_bin_sem;
SemaphoreHandle_t h_entry_c_bin_sem;
SemaphoreHandle_t h_exit_c_bin_sem;
SemaphoreHandle_t h_entry_d_bin_sem;
SemaphoreHandle_t h_exit_d_bin_sem;
SemaphoreHandle_t h_mutex_control_esclusa_sem;
SemaphoreHandle_t h_continue_bin_sem;

/* Declare a variable of type TaskHandle_t. This is used to reference threads. */
TaskHandle_t h_task_gate_a;
TaskHandle_t h_task_gate_b;
TaskHandle_t h_task_gate_c;
TaskHandle_t h_task_gate_d;
TaskHandle_t h_task_test;

/********************** external functions definition ************************/
void app_init(void)
{
	/*  Declare & Initialize App variables */
	g_app_cnt = G_APP_CNT_INI;
	g_app_task_cnt = G_APP_TASK_CNT_INI;
	g_app_tick_cnt = G_APP_TICK_CNT_INI;
	g_task_idle_cnt = G_TASK_IDLE_CNT_INI;
	g_app_stack_overflow_cnt = G_APP_STACK_OVERFLOW_CNT_INI;

	g_tasks_cnt = G_TASKS_CNT_INI;
//	g_gate_open = gate_open_NONE;

	// Init semaforos Puerta A
	h_entry_a_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_entry_a_bin_sem);
	vQueueAddToRegistry(h_entry_a_bin_sem, "Entry Gate A BIN Handle");

	h_exit_a_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_exit_a_bin_sem);
	vQueueAddToRegistry(h_exit_a_bin_sem, "Exit Gate A BIN Handle");

	// Init semaforos Puerta B
	h_entry_b_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_entry_b_bin_sem);
	vQueueAddToRegistry(h_entry_b_bin_sem, "Entry Gate B BIN Handle");

	h_exit_b_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_exit_b_bin_sem);
	vQueueAddToRegistry(h_exit_b_bin_sem, "Exit Gate B BIN Handle");

	// Init semaforos Puerta C
	h_entry_c_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_entry_c_bin_sem);
	vQueueAddToRegistry(h_entry_c_bin_sem, "Entry Gate C BIN Handle");

	h_exit_c_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_exit_c_bin_sem);
	vQueueAddToRegistry(h_exit_c_bin_sem, "Exit Gate C BIN Handle");

	// Init semaforos Puerta D
	h_entry_d_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_entry_d_bin_sem);
	vQueueAddToRegistry(h_entry_d_bin_sem, "Entry Gate D BIN Handle");

	h_exit_d_bin_sem = xSemaphoreCreateBinary();
	configASSERT(NULL != h_exit_d_bin_sem);
	vQueueAddToRegistry(h_exit_d_bin_sem, "Exit Gate D BIN Handle");

	// Init mutex control de esclusa
	h_mutex_control_esclusa_sem = xSemaphoreCreateMutex();
	configASSERT(NULL != h_mutex_control_esclusa_sem);
	vQueueAddToRegistry(h_mutex_control_esclusa_sem, "Mutex Control Esclusa Handle");

	/* Print out: Application Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %lu", GET_NAME(app_init), xTaskGetTickCount());

	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app);
	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app_);
	LOGGER_INFO(" %s is a %s", GET_NAME(app), p_app__);

    /* Before a queue or semaphore (binary or counting) or mutex is used it must 
     * be explicitly created.
	 *
	 * Check the queue or semaphore (binary or counting) or mutex was created
     * successfully.
     *
     * Add queue or semaphore (binary or counting) or mutex to registry. */

	/* Add threads, ... */
    BaseType_t ret;

    /* Task Gate A thread at priority 3 */
    ret = xTaskCreate(task_gate_a,						/* Pointer to the function thats implement the task. */
					  "Task Gate A",					/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 3ul),			/* This task will run at priority 1. */
					  &h_task_gate_a);					/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task Gate B thread at priority 2 */
    ret = xTaskCreate(task_gate_b,						/* Pointer to the function thats implement the task. */
					  "Task Gate B",					/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_gate_b);					/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task Gate C thread at priority 3 */
    ret = xTaskCreate(task_gate_c,						/* Pointer to the function thats implement the task. */
					  "Task Gate B",					/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_gate_c);					/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task Gate D thread at priority 2 */
    ret = xTaskCreate(task_gate_d,						/* Pointer to the function thats implement the task. */
					  "Task Gate D",					/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 2ul),			/* This task will run at priority 1. */
					  &h_task_gate_d);					/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Task Test thread at priority 1 */
    ret = xTaskCreate(task_test,						/* Pointer to the function thats implement the task. */
					  "Task Test",						/* Text name for the task. This is to facilitate debugging only. */
					  (configMINIMAL_STACK_SIZE),		/* Stack depth in words. */
					  NULL,								/* We are not using the task parameter. */
					  (tskIDLE_PRIORITY + 1ul),			/* This task will run at priority 1. */
					  &h_task_test);					/* We are using a variable as task handle. */

    /* Check the thread was created successfully. */
    configASSERT(pdPASS == ret);

    /* Total amount of heap space that remains unallocated. Is also available
     * with xFreeBytesRemaining variable for heap management schemes 2 to 5.
     * Memory array used by heap_4 is specified as:
     * uint8_t ucHeap[configTOTAL_HEAP_SIZE]; */
    ret = xPortGetFreeHeapSize();

    /* There is no dedicated list for task in Running mode (as we have only
     * one task in this state at the moment), but the currently run task ID
     * is stored in variable pxCurrentTCB */

  	/* Application Interrupts Init */
	app_it_init();

	/* Init Cycle Counter */
	cycle_counter_init();
}

/********************** end of file ******************************************/
