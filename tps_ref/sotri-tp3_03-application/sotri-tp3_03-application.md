## Paso 03: Respuesta de Gemini Pro

El presente código fuente establece una arquitectura basada en **FreeRTOS** orientada a eventos o estímulos. A diferencia del esquema anterior, en este proyecto se incorporan interrupciones por hardware y una tarea de prueba (Test Task) diseñada para inyectar eventos simulados en el sistema.

A continuación, se detalla el funcionamiento de cada componente:

### 1. `app.c` (Configuración e Inicialización Principal)
Actúa como el núcleo orquestador del sistema operativo en tiempo real.
* **Creación de Tareas:** Se reemplazaron las tareas genéricas anteriores. Ahora, mediante `xTaskCreate`, se instancian tres hilos de ejecución:
    * **Task Entry A**
    * **Task Exit A**
    * **Task Test**
* Todas se inicializan con la misma prioridad base (`tskIDLE_PRIORITY + 1ul`) y el tamaño de pila mínimo.
* Mantiene la llamada a la inicialización de interrupciones (`app_it_init`) y a los contadores de la aplicación.

---

### 2. `app_it.c` (Manejo de Interrupciones)
Este archivo ha evolucionado para capturar eventos del mundo físico (hardware).
* **Rutina EXTI (External Interrupt):** Se ha incorporado la función `HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`. Esta es una función de retrollamada (callback) nativa de la capa HAL de STM32 que se dispara automáticamente cuando un pin configurado como interrupción cambia de estado.
* **Detección de Pulsador:** Dentro de la rutina, se verifica si el pin que generó la interrupción corresponde a un botón (`if (GPIO_Pin == BTN_...)`). Esto permite que el sistema reaccione instantáneamente cuando el usuario presiona un pulsador físico.

---

## Paso 04: Confirmación mediante depuración y ejecución

La depuración permitió verificar que la aplicación se inicializa correctamente y que las tareas `Task Entry A`, `Task Entry B`, `Task Exit A`, `Task Exit B` y `Task Test` son creadas exitosamente por FreeRTOS.

Se observó que `Task Test` incrementa su prioridad y ejecuta una secuencia de eventos almacenada en `e_task_test_array`, recorriendo distintos índices del arreglo con una espera de 5000 ms entre cada evento. Esto confirma su función como generador automático de estímulos para validar el comportamiento del sistema.

Asimismo, las tareas de entrada y salida se ejecutan periódicamente mostrando mensajes de estado cada 2500 ms.

La ejecución observada coincide con la descripción general del sistema orientado a eventos realizada por Gemini, aunque la versión actual del proyecto incorpora cuatro tareas de aplicación (`Entry A`, `Entry B`, `Exit A` y `Exit B`) además de la `Task Test`.

```text
[info]   Task Entry B is running - Tick [mS] = 0
[info]    ==> Task Entry B - Wait:   2500mS
[info]  
[info]   Task Entry A is running - Tick [mS] = 0
[info]    ==> Task Entry A - Wait:   2500mS
[info]  
[info]   Task Exit A is running - Tick [mS] = 0
[info]    ==> Task Exit A  - Wait:   2500mS
[info]  
[info]   Task Exit B is running - Tick [mS] = 0
[info]    ==> Task Exit B  - Wait:   2500mS
[info]  
[info]   Task Test is running - Tick [mS] = 0
[info]   <=> Task Test - Priority: Task Test 3
[info]  
[info]   <=> Task Test - e_task_test_array: index 0
[info]   <=> Task Test - Wait:   5000mS
[info]    ==> Task Entry B - Wait:   2500mS
[info]    ==> Task Entry A - Wait:   2500mS
[info]    ==> Task Exit A  - Wait:   2500mS
[info]    ==> Task Exit B  - Wait:   2500mS
```

### 3. `freertos.c` (Retrollamadas o Hooks del Sistema)

Conserva las herramientas de diagnóstico y seguridad del sistema operativo.
* **Monitoreo de Salud:** Mantiene activos los hooks `vApplicationIdleHook` (cuenta los ciclos de inactividad de la CPU), `vApplicationTickHook` (cuenta los tics del reloj del SO) y `vApplicationStackOverflowHook` (bloquea el sistema de forma segura mediante un `configASSERT` si detecta que alguna tarea excedió su memoria RAM asignada).

---

### 4. `task_entry_a.c` y `task_exit_a.c` (Lógica de las Tareas de Aplicación)
Estas dos tareas actúan como los "actores" principales que responden a los eventos del sistema. Ambas poseen una estructura similar:
* **Inicialización:** Cada una inicializa su propio contador individual (`g_task_entry_a_cnt` y `g_task_exit_a_cnt`) y reporta por consola (`LOGGER_INFO`) que ha comenzado a correr junto con la marca de tiempo (Ticks) actual.
* **Ciclo de Trabajo:** Ingresan a un bucle infinito donde tienen definidos demoras máximas de 2500 milisegundos (`TASK_ENTRY_A_DEL_MAX` y `TASK_EXIT_A_DEL_MAX`). Se espera que estas tareas realicen transiciones de estado cuando reciben la señal correcta.

---

### 5. `task_test.c` (Máquina Inyectora de Pruebas)
Este es el archivo más novedoso del conjunto, diseñado explícitamente para automatizar las pruebas del software (testing) sin depender del hardware exterior.
* **Elevación de Prioridad:** Al arrancar, lo primero que hace es invocar `vTaskPrioritySet` para elevar su propia prioridad en 2 puntos por encima de su valor original (`uxTaskPriorityGet(NULL) + 2ul`). Esto garantiza que esta tarea tome el control inmediato del microprocesador y se ejecute *antes* que las tareas Entry y Exit.
* **Simulación de Eventos:** Su bucle infinito recorre un vector o arreglo predefinido llamado `e_task_test_array`. Este arreglo contiene una secuencia de eventos o estímulos de prueba pre-programados (como `Entry_A` y `Exit_A`).
* **Excitar al sistema:** A través de una estructura `switch-case`, la tarea procesa cada evento del arreglo y simulará la inyección de estas señales hacia el resto del sistema. Esto permite validar que `Task Entry A` y `Task Exit A` respondan correctamente a distintas secuencias lógicas de funcionamiento de manera controlada y repetible.

---

