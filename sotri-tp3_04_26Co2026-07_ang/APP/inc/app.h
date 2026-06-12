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

#ifndef APP_H_
#define APP_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/

/********************** macros ***********************************************/
#define TASK_QTY 2ul

#define	G_TASKS_CNT_MAX	3ul

/********************** typedef **********************************************/

/********************** external data declaration ****************************/
extern uint32_t g_app_cnt;
extern uint32_t g_app_task_cnt;
extern volatile uint32_t g_app_tick_cnt;
extern uint32_t g_task_idle_cnt;
extern uint32_t g_app_stack_overflow_cnt;

extern uint32_t	g_tasks_cnt;

extern uint32_t	g_gate_open;

enum gate_open{
	gate_open_NONE			= 0,
	gate_open_A				= 1 << 0,
	gate_open_B				= 1 << 1,
	gate_open_C				= 1 << 2,
	gate_open_D				= 1 << 3,
	gate_open_MAX_ERROR,
};

/* Declare a variable of type QueueHandle_t. This is used to reference queues*/

/* Declare a variable of type SemaphoreHandle_t (binary or counting) or mutex.
 * This is used to reference the semaphore that is used to synchronize a thread
 * with other thread or to ensure mutual exclusive access to...*/
extern SemaphoreHandle_t h_entry_a_bin_sem;
extern SemaphoreHandle_t h_exit_a_bin_sem;
extern SemaphoreHandle_t h_entry_b_bin_sem;
extern SemaphoreHandle_t h_exit_b_bin_sem;
extern SemaphoreHandle_t h_entry_c_bin_sem;
extern SemaphoreHandle_t h_exit_c_bin_sem;
extern SemaphoreHandle_t h_entry_d_bin_sem;
extern SemaphoreHandle_t h_exit_d_bin_sem;

extern SemaphoreHandle_t h_mutex_control_esclusa_sem;

extern SemaphoreHandle_t h_continue_bin_sem;		// _TODO, borrar
extern SemaphoreHandle_t h_mutex_mut_sem;			// _TODO, borrar

/* Declare a variable of type TaskHandle_t. This is used to reference threads. */
//extern TaskHandle_t h_task_entry_a;
//extern TaskHandle_t h_task_exit_a;
//extern TaskHandle_t h_task_entry_b;
//extern TaskHandle_t h_task_exit_b;
//extern TaskHandle_t h_task_entry_c;
//extern TaskHandle_t h_task_exit_c;
//extern TaskHandle_t h_task_entry_d;
//extern TaskHandle_t h_task_exit_d;
//extern TaskHandle_t h_task_test;

/********************** external functions declaration ***********************/
extern void app_init(void);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* APP_H_ */

/********************** end of file ******************************************/
