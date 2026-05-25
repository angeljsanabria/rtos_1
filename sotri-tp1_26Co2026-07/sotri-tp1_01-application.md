# CESE - Sistemas Operativos de Tiempo Real
## Trabajo Practico N°: 1 - Tareas de FreeRTOS


## Actividad 01 - Analisis de `sotri-tp1_26Co2026-07` (Copia en  `sotri-tp1_01-application`)

> Nota: la guia original referencia STM32F103/F4 y nombres de archivo `stm32f1xx_*`.
> En esta entrega se analiza el proyecto real del grupo para STM32L4R5, con archivos equivalentes:
> - `startup_stm32l4r5zitx.s`
> - `main.c`
> - `stm32l4xx_it.c`
> - `FreeRTOSConfig.h`
> - `freertos.c` (Core y APP)

---

## 1) Analisis y explicacion del codigo fuente base (Core) 

### 1.1 Flujo de arranque desde `Reset_Handler` hasta `main`

En `Core/Startup/startup_stm32l4r5zitx.s`, el flujo principal es:

1. Vector de interrupciones apunta al `Reset_Handler`.
2. En `Reset_Handler` se ejecuta:
   - inicializacion de memoria `.data` y `.bss`
   - `SystemInit()`
   - `__libc_init_array()`
   - `main()`
3. Si `main()` retornara, entra en loop infinito defensivo.

Esto coincide con el flujo estandar Cortex-M para proyectos STM32 con HAL.

### 1.2 Secuencia de `main.c` y configuracion principal

En `Core/Src/main.c`, la secuencia relevante es:

1. (Opcional) `initialise_monitor_handles()` cuando semihosting esta habilitado.
2. `HAL_Init()`
3. `SystemClock_Config()`
4. Inicializacion de perifericos (`GPIO`, `LPUART1`, `USART3`, `USB`, `TIM2`)
5. `HAL_TIM_Base_Start_IT(&htim2)` para iniciar TIM2 en modo interrupcion
6. `app_init()` para crear tareas y estado de la capa aplicacion
7. `osKernelStart()` para arrancar scheduler FreeRTOS

Luego de `osKernelStart()`, el `while(1)` de `main` ya no es el flujo normal.

### 1.3 `stm32l4xx_it.c`: rol de interrupciones

En `Core/Src/stm32l4xx_it.c`:

- `TIM1_UP_TIM16_IRQHandler()` llama `HAL_TIM_IRQHandler(&htim1)` (time base HAL).
- `TIM2_IRQHandler()` llama `HAL_TIM_IRQHandler(&htim2)` (timer de aplicacion/estadisticas).

La funcion `HAL_TIM_PeriodElapsedCallback()` en `main.c` discrimina por instancia:

- Si `TIM1`: llama `HAL_IncTick()`.
- Si `TIM2`: incrementa `ulHighFrequencyTimerTicks`.

### 1.4 `FreeRTOSConfig.h`: parametros principales observados

En `Core/Inc/FreeRTOSConfig.h`:

- `configTICK_RATE_HZ = 1000` (tick de RTOS a 1 ms)
- `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`
- `configMAX_SYSCALL_INTERRUPT_PRIORITY` derivado correctamente

Adicionalmente:

- `configUSE_IDLE_HOOK = 0`
- `configUSE_TICK_HOOK = 0`
- no estan activos los defines de run-time stats (`configGENERATE_RUN_TIME_STATS`, `portCONFIGURE_TIMER_FOR_RUN_TIME_STATS`, `portGET_RUN_TIME_COUNTER_VALUE`) dentro de este archivo.

### 1.5 `freertos.c` (Core) y `freertos.c` (APP)

- `Core/Src/freertos.c` contiene stubs/hook debiles generados por CubeMX.
- `APP/src/freertos.c` contiene implementaciones de hooks de aplicacion:
  - `vApplicationIdleHook()`
  - `vApplicationTickHook()`
  - `vApplicationStackOverflowHook()`

Conclusion tecnica: hay base completa para observabilidad de tareas, pero para que quede 100% alineado al ejemplo de referencia, faltaria activar en `FreeRTOSConfig.h` los defines que habilitan esos hooks y run-time stats.

---

## 2) Evolucion de SysTick y `SystemCoreClock`

### 2.1 `SystemCoreClock`

- Al reset, parte con valor por defecto del startup.
- Durante `SystemInit()` y luego `SystemClock_Config()`, se reconfigura PLL y buses.
- En este proyecto el clock queda en 120 MHz (`SYSCLK/HCLK`), coherente con la configuracion RCC del `.ioc`.

### 2.2 SysTick

- `HAL_Init()` configura tick base HAL.
- En este proyecto la base HAL esta redirigida a TIM1 (`stm32l4xx_hal_timebase_tim.c`), no a SysTick puro.
- FreeRTOS mantiene `configTICK_RATE_HZ = 1000`, por lo que la logica temporal del scheduler es de 1 ms por tick.

Resumen:

- `SystemCoreClock` termina estable en la frecuencia de trabajo configurada.
- El tiempo base de HAL/RTOS se sostiene por TIM1 y su callback, mientras TIM2 se usa para contador de alta frecuencia.

---

## 3) Comportamiento desde reset hasta antes del `while(1)` de `main`

1. Reset vector -> `Reset_Handler`.
2. Inicializacion de memoria y runtime C.
3. Ingreso a `main`.
4. Inicializacion HAL + clocks + perifericos.
5. Inicio de TIM2 con interrupciones.
6. `app_init()`:
   - inicializa contadores globales de aplicacion
   - crea `task_btn` y `task_led` con `xTaskCreate`
   - ejecuta init de interrupciones de aplicacion (`app_it_init`)
   - inicializa contador de ciclos (`cycle_counter_init`)
7. Arranque de scheduler (`osKernelStart`).

A partir de ese punto, la CPU ejecuta tareas segun scheduler FreeRTOS y prioridades.

---

## 4) Como y para que interactuan SysTick/TIM1 y TIM2 con FreeRTOS

### 4.1 TIM1 (time base HAL / tick de sistema)

- TIM1 genera interrupcion periodica.
- En callback se ejecuta `HAL_IncTick()`.
- Esto mantiene la base temporal de HAL y apoya servicios temporales usados por FreeRTOS/HAL.

### 4.2 TIM2 (contador de alta frecuencia para estadisticas)

- TIM2 se inicia con `HAL_TIM_Base_Start_IT(&htim2)`.
- En su interrupcion, callback incrementa `ulHighFrequencyTimerTicks`.
- Este contador esta preparado para run-time stats de FreeRTOS (medicion de tiempo de CPU por tarea), con mayor resolucion temporal.

En la configuracion actual, TIM2 esta listo y funcionando; para explotarlo directamente en estadisticas de FreeRTOS, faltaria activar los defines correspondientes en `FreeRTOSConfig.h`.

---

## 5) Como y para que TIM2 interactua con la HAL del proyecto STM32

1. CubeMX genera `MX_TIM2_Init()` con:
   - `ClockSource = Internal`
   - `Prescaler = 119`
   - `Period = 99`
   - `CounterMode = Up`
2. HAL MSP configura clock de TIM2 y su NVIC (`TIM2_IRQn` prioridad 5, subprioridad 0).
3. Handler `TIM2_IRQHandler()` delega en `HAL_TIM_IRQHandler(&htim2)`.
4. HAL dispara callback de usuario `HAL_TIM_PeriodElapsedCallback`.
5. El usuario actualiza `ulHighFrequencyTimerTicks`.

Se obtiene una base de tiempo periodica desacoplada del tick principal.

---

## 6) Analisis de capa aplicacion (`app.c`, `task_btn.c`, `task_led.c`, `task_led_interface.c`, `APP/src/freertos.c`)

### 6.1 `app.c`

- Inicializa contadores globales de aplicacion.
- Crea dos tareas (`task_btn`, `task_led`) con prioridad `tskIDLE_PRIORITY + 1`.
- Llama `app_it_init()` y `cycle_counter_init()`.
- Registra mensajes de arranque por logger.

### 6.2 `task_btn.c`

- Implementa una maquina de estados para antirrebote de boton:
  - `ST_BTN_UP`
  - `ST_BTN_FALLING`
  - `ST_BTN_DOWN`
  - `ST_BTN_RISING`
- Lee pin por `HAL_GPIO_ReadPin`.
- Publica eventos a LED via `put_event_task_led(EV_LED_BLINK / EV_LED_OFF)`.

### 6.3 `task_led.c`

- Implementa maquina de estados:
  - `ST_LED_OFF`
  - `ST_LED_BLINK`
- Consume evento compartido (`task_led_dta`) y conmuta LED por HAL GPIO.
- El parpadeo usa `xTaskGetTickCount()` para temporizar.

### 6.4 `task_led_interface.c`

- Define interfaz simple de comunicacion entre tareas:
  - `put_event_task_led(event)` escribe evento y levanta `flag`.
- Es una señalizacion minima por memoria compartida, sin cola ni semaforo.

### 6.5 `APP/src/freertos.c`

- Define hooks de aplicacion (idle, tick, stack overflow).
- Estos hooks aportan medicion y diagnostico, si su habilitacion esta activa en `FreeRTOSConfig.h`.

---

## 7) Confirmacion por depuracion (criterios observables)

Para validar en depuracion que el analisis anterior se cumple, se verifican estos puntos:

1. Breakpoint en `main` antes y despues de `osKernelStart()`.
2. Breakpoint en `app_init()` y chequeo de retorno correcto de `xTaskCreate`.
3. Breakpoint en `TIM2_IRQHandler()` y en `HAL_TIM_PeriodElapsedCallback()` caso TIM2.
4. Watch de `ulHighFrequencyTimerTicks` creciendo periodicamente.
5. Confirmacion de cambios de estado en `task_btn_statechart` y `task_led_statechart`.
6. En vista de tareas (si habilitada): presencia de `Task BTN` y `Task LED`.

Resultado esperado:

- Scheduler activo, tareas creadas y ejecutando.
- TIM2 interrumpiendo y alimentando contador de alta frecuencia.
- Evento de boton modificando comportamiento de LED.

---

## 8) Cierre de Actividad 01

Se completo la Actividad 01 con:

- proyecto STM32 con FreeRTOS compilable/depurable
- analisis tecnico de archivos core solicitados (equivalentes L4)
- analisis tecnico de archivos de aplicacion
- guia de verificacion por depuracion para confirmar comportamiento

