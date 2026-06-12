<<<<<<< Updated upstream
##Paso 03: Respuesta de Gemini Pro

El código fuente adjunto establece la estructura básica de una aplicación utilizando el sistema operativo en tiempo real **FreeRTOS**. De acuerdo con los metadatos y comentarios del sistema, el propósito de este proyecto es implementar el patrón clásico de sincronización conocido como **"Lectores-Escritores" (Readers-Writers problem)**, basándose en la literatura de "The Little Book of Semaphores".

A continuación, se detalla el funcionamiento de cada uno de los archivos:

### 1. `app.c` (Configuración e Inicialización Principal)
Este archivo contiene la función `app_init()`, que actúa como el punto de entrada para configurar la aplicación.
* **Inicialización de contadores:** Establece en cero las variables globales que monitorean el estado de la aplicación, como los contadores de tareas, ticks, tiempo inactivo y desbordamientos de pila (`g_app_cnt`, `g_app_task_cnt`, `g_app_tick_cnt`, `g_task_idle_cnt`, `g_app_stack_overflow_cnt`).
* **Creación de Tareas:** Utiliza la API `xTaskCreate` para instanciar dos hilos de ejecución en el RTOS: **Task A** y **Task B**. Ambas tareas se crean con la misma prioridad (`tskIDLE_PRIORITY + 1ul`) y se les asigna el tamaño de pila mínimo indispensable (`configMINIMAL_STACK_SIZE`).
* **Preparación para Sincronización:** El código incluye comentarios estratégicos que indican dónde se deberán declarar las colas (`QueueHandle_t`) y los semáforos o mutexes (`SemaphoreHandle_t`), mecanismos necesarios para resolver la concurrencia del problema Lectores-Escritores en el futuro.
* **Llamadas a módulos externos:** Finaliza llamando a `app_it_init()` para preparar el manejo de interrupciones y a `cycle_counter_init()` para inicializar el contador de ciclos.

---

### 2. `app_it.c` (Manejo de Interrupciones)
Su función es gestionar rutinas y configuraciones a nivel de interrupciones de hardware.
* **Protección de recursos:** Dentro de `app_it_init()`, se incluyen instrucciones directas en lenguaje ensamblador del microcontrolador (`__asm("CPSID i");` y `__asm("CPSIE i");`). Estas instrucciones deshabilitan y vuelven a habilitar las interrupciones globales, creando una "sección crítica" para proteger el acceso a un recurso compartido que se añadirá más adelante.

---

### 3. `freertos.c` (Retrollamadas o Hooks del Sistema)
Contiene rutinas de servicio (hooks) que FreeRTOS invoca automáticamente al ocurrir eventos específicos en el sistema.
* **`vApplicationIdleHook`:** Se ejecuta cíclicamente cuando el procesador entra en estado inactivo (ninguna tarea lista para ejecutarse). Aprovecha este tiempo para incrementar el contador de inactividad `g_task_idle_cnt`.
* **`vApplicationTickHook`:** Es llamada periódicamente por la interrupción de reloj (tick) que hace latir al RTOS. Aquí se encarga de incrementar el contador global `g_app_tick_cnt`.
* **`vApplicationStackOverflowHook`:** Rutina de seguridad ejecutada si el sistema detecta que una tarea excedió el límite de su memoria RAM asignada. Si esto ocurre, bloquea intencionalmente el sistema deteniendo su ejecución (`configASSERT( 0 );`) dentro de una sección crítica e incrementa `g_app_stack_overflow_cnt` para alertar al desarrollador.

---

### 4. `task_a.c` y `task_b.c` (Lógica de las Tareas)
Estos archivos contienen las funciones que definen la "personalidad" y el comportamiento de los hilos de ejecución inicializados en `app.c`. Ambas poseen un bucle principal infinito (`for (;;)`) estándar en arquitecturas RTOS.
* **Comportamiento de Task A:** En cada ciclo, incrementa su variable de registro `g_task_a_cnt`, solicita imprimir en consola un mensaje de texto fijo (`"==> Task A - Wait: 250mS"`) e invoca `vTaskDelay` para ceder el control del procesador por **250 milisegundos** (`TASK_A_DEL_MAX`).
* **Comportamiento de Task B:** De manera similar, incrementa `g_task_b_cnt` e imprime su respectivo mensaje en consola. La diferencia fundamental es que su tiempo de latencia es de **2500 milisegundos** (`TASK_B_DEL_MAX`). Esto significa que, temporalmente, la Tarea B despierta y se ejecuta de forma diez veces más lenta que la Tarea A.

---

## Paso 04: Confirmación mediante depuración y ejecución

La depuración permitió verificar que la aplicación funciona de acuerdo con lo descripto en el análisis previo. Durante la ejecución se observó que `app_init()` se ejecuta correctamente y que las tareas `Task A` y `Task B` son creadas e iniciadas por el scheduler de FreeRTOS.

La salida por consola muestra que `Task A` se ejecuta con una frecuencia significativamente mayor que `Task B`, lo cual coincide con los retardos configurados mediante `vTaskDelay()`. Mientras `Task A` se activa aproximadamente cada 250 ms, `Task B` lo hace cada 2500 ms, observándose cerca de diez ejecuciones de `Task A` por cada ejecución de `Task B`.

``text
[info] app_init is running - Tick [mS] = 0 
[info] app is a RTOS - Event-Triggered Systems (ETS) 
[info] app is a sotri-tp3_02-application: Readers-Writers 
[info] app is a (Source => CESE - Sistemas Operativos de Tiempo Real 
[info] Task B is running - Tick [mS] = 0 
[info] ==> Task B - Wait: 250mS 
[info] Task A is running - Tick [mS] = 0 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 
[info] ==> Task B - Wait: 250mS 
[info] ==> Task A - Wait: 250mS 

```

Además, se comprobó que actualmente no existe ningún mecanismo de sincronización ni comunicación entre las tareas. Ambas funcionan de manera independiente, limitándose a incrementar sus contadores e imprimir mensajes de estado periódicamente.

Por lo tanto, la ejecución observada confirma el comportamiento esperado del código y valida el análisis realizado previamente.

---

## Paso 06 - Implementación del patrón Readers-Writers

Se modificó la aplicación utilizando un mutex de FreeRTOS para proteger el acceso a un recurso compartido entre las tareas.

Se definió una variable global compartida (`shared_data`). `Task B` fue configurada como Writer, incrementando periódicamente el valor de dicha variable. `Task A` fue configurada como Reader, leyendo periódicamente el valor almacenado.

Para garantizar la exclusión mutua durante el acceso al recurso compartido se utilizó un mutex mediante las funciones `xSemaphoreTake()` y `xSemaphoreGive()`.

Durante la depuración se observó el siguiente comportamiento:

```text
Writer: value = 1
Reader: value = 1
Reader: value = 1
...
Writer: value = 2
Reader: value = 2
Reader: value = 2
...
```

La salida confirma que el Writer actualiza el valor compartido cada 2500 ms, mientras que el Reader lo consulta cada 250 ms. Además, no se observaron accesos simultáneos al recurso compartido, verificándose el correcto funcionamiento del mecanismo de sincronización mediante mutex.

Por lo tanto, se implementó satisfactoriamente una versión simplificada del problema clásico Readers-Writers utilizando exclusión mutua para proteger el recurso compartido.
=======
# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 1 - Tareas de FreeRTOS - Actividad 2

### Paso 1
En el repositorio.

### Paso 2: Preguntas y Respuestas sobre Tareas en FreeRTOS (Basado en el proyecto actual)

**1. ¿Cómo FreeRTOS asigna tiempo de procesamiento a cada Tarea en una aplicación?**
FreeRTOS utiliza un *Scheduler* (planificador) que se ejecuta periódicamente impulsado por una interrupción de tiempo base llamada *tick* del sistema (configurado en `configTICK_RATE_HZ = 1000` en este proyecto, es decir, 1 ms). En cada *tick*, el scheduler evalúa si debe realizar un cambio de contexto (context switch) para retirar el control de la CPU a la tarea actual y dárselo a otra, basándose en la política configurada (generalmente preemptiva con *time-slicing*). 

**2. ¿Cómo FreeRTOS elige qué Tarea debe ejecutarse en un momento dado?**
El Scheduler siempre elige ejecutar la tarea que tenga la **mayor prioridad** y que se encuentre en estado **Ready** (Lista). Si hay varias tareas con la misma prioridad máxima (como en `app.c` donde `task_btn` y `task_led` tienen ambas la prioridad `tskIDLE_PRIORITY + 1ul`), el scheduler va intercalando el tiempo de CPU entre ellas (*round-robin* o *time-slicing*) en cada *tick* del sistema.

**3. ¿Cómo la prioridad relativa de cada Tarea afecta el comportamiento del sistema?**
La prioridad define el derecho absoluto sobre la CPU frente a tareas de menor prioridad. Una tarea de alta prioridad en estado *Ready* preemptará (interrumpirá) inmediatamente a cualquier tarea de menor prioridad que se esté ejecutando. Si la tarea de alta prioridad nunca se bloquea (por ejemplo, con `vTaskDelay` o esperando un evento), provocará "inanición" (*starvation*) y las tareas de menor prioridad jamás se ejecutarán. A diferencia de un entorno *baremetal* (super-loop), donde todas las rutinas corren secuencialmente, en RTOS una tarea vital puede adueñarse de la CPU en el instante que lo necesite.

**4. ¿Cuáles son los estados en los que puede encontrarse una Tarea?**
Una tarea en FreeRTOS puede estar en uno de estos cuatro estados:

- **Running (En ejecución):** Es la tarea que tiene el control actual de la CPU (solo hay una en un MCU de un solo núcleo como el STM32L4).
- **Ready (Lista):** La tarea está lista para ejecutarse pero está esperando su turno porque otra tarea de igual o mayor prioridad está ocupando la CPU.
- **Blocked (Bloqueada):** La tarea está esperando que ocurra un evento (ej. disponibilidad de un semáforo) o esperando que transcurra un tiempo específico (`vTaskDelay`). Mientras está bloqueada, cede la CPU a otras tareas.
- **Suspended (Suspendida):** La tarea ha sido pausada explícitamente (`vTaskSuspend`) y el scheduler la ignora por completo hasta que se reanuda (`vTaskResume`).

**5. ¿Cómo implementar Tareas?**
Una tarea se implementa como una función en C que devuelve `void` y recibe un parámetro `void `*. Típicamente, contiene inicializaciones seguidas de un bucle infinito `for (;;)` para que nunca retorne. Ejemplo en `task_led.c`:

```c
void task_led(void *parameters)
{
    // Inicialización local de la tarea
    g_task_led_cnt = 0;
    
    // Bucle infinito
    for (;;)
    {
        // Lógica de la tarea
        task_led_statechart();
        // Nota: en una implementación ideal de RTOS, aquí habría un vTaskDelay o bloqueo 
        // para ceder CPU, aunque el round-robin permite que comparta con task_btn.
    }
}
```

**6. ¿Cómo crear una o más instancias de una Tarea?**
Se crea usando la API `xTaskCreate()`. Se le debe pasar el puntero a la función que implementa la tarea, un nombre, el tamaño del stack, parámetros, la prioridad y un puntero para almacenar su *handle* (identificador). En `app.c` vemos la creación:

```c
ret = xTaskCreate(task_led, "Task LED", (2 * configMINIMAL_STACK_SIZE), NULL, (tskIDLE_PRIORITY + 1ul), &h_task_led);
```

Para crear múltiples instancias, se llama a `xTaskCreate` repetidas veces pasando la misma función (`task_led`). Además de asignar diferentes *handles* y parámetros (para distinguir el comportamiento, ej. diferentes pines), **también se debe cambiar el nombre de texto de la tarea** (el segundo parámetro de `xTaskCreate`, como `"Task LED 1"`, `"Task LED 2"`). Esto es fundamental para poder diferenciarlas fácilmente en las herramientas de debug o análisis de FreeRTOS.

**7. ¿Cómo eliminar una Tarea?**
Se elimina llamando a la función de la API `vTaskDelete(TaskHandle_t xTaskToDelete)`. Se le pasa como argumento el *handle* de la tarea que se desea eliminar. Si una tarea quiere destruirse a sí misma, puede llamar a `vTaskDelete(NULL)`. 

Al eliminar una tarea, el manejo de su memoria depende de cómo fue creada:

- **Asignación Dinámica (`xTaskCreate`):** La memoria reservada dinámicamente para su stack y TCB es liberada automáticamente (típicamente de forma asíncrona por la tarea Idle). El programador solo debe preocuparse por liberar otros recursos internos (buffers, semáforos) que la tarea haya pedido antes de que sea borrada.
- **Asignación Estática (`xTaskCreateStatic`):** Dado que la memoria del stack y el TCB fue suministrada por el usuario mediante variables estáticas, **FreeRTOS no la libera**. El RTOS solo quita la tarea del scheduler; es entera responsabilidad del programador decidir qué hacer con esos *buffers* estáticos (por ejemplo, si se van a reutilizar o no).

### Paso 3
Cambios observados:

**Caso 1: Modificamos solo la prioridad del boton: Prioridad de Tarea Botón > Prioridad de Tarea LED**
Al tener mayor prioridad y no utilizar ninguna API bloqueante de FreeRTOS (como `vTaskDelay` u `osDelay`), la tarea del botón nunca pasa al estado *Blocked*. Permanece constantemente en ejecución (*Running*), monopolizando el 100% del tiempo de CPU. Esto provoca *starvation* en la tarea del LED, la cual nunca llega a ejecutarse.

**Caso 2: Modificamos solo la prioridad del LED: Prioridad de Tarea LED > Prioridad de Tarea Botón**
De manera idéntica al caso anterior, al darle mayor prioridad a la tarea del LED y carecer de llamadas que cedan el control de la CPU (funciones de bloqueo), esta tarea se apropia de todo el tiempo de procesamiento. En consecuencia, se genera inanición sobre la tarea del botón, impidiendo que sea planificada por el Scheduler y ejecutada.


### Paso 4

Al crear 3 instancias de `task_btn` y hacer que `task_led` elimine la primera:

1. **Eliminación exitosa:** Se observa `[info] Task LED will delete btn_task` en el tick 0. La primera tarea (`Task BTN 1`) es eliminada antes de llegar a ejecutarse, por lo que su mensaje de inicio nunca aparece en el log.
2. **Arranque normal:** Las tareas restantes arrancan correctamente y muestran `[info] Task BTN 2 is running - Tick [mS] = 1` y `[info] Task BTN 3 is running - Tick [mS] = 2`.
3. **Condición de carrera:** Al presionar el botón, solo responde una tarea (`[info] Task BTN 2 - BTN PRESSED` y `[info] Task BTN 2 - BTN HOVER`). Esto ocurre porque ambas instancias operan sobre la misma variable de estado global (`task_btn_dta`). Cuando la Tarea 2 consume el evento y avanza la máquina de estados, la Tarea 3 encuentra el estado alterado y no detecta la acción. *Conclusión: las tareas instanciadas múltiples veces no deben depender de variables globales para su contexto.*


[info]  
[info] app_init is running - Tick [mS] = 0
[info]  app is a RTOS - Event-Triggered Systems (ETS)
[info]  app is a seo-tp1_01-application: Demo Code
[info]  app is a (Source => CESE - Sistemas Operativos de Tiempo Real
[info]  
[info] Task LED is running - Tick [mS] =   0
[info] Task LED will delete btn_task
[info]  
[info] Task BTN 2 is running - Tick [mS] =   1
[info]  
[info] Task BTN 3 is running - Tick [mS] =   2
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF
[info]  Task BTN 2 - BTN PRESSED
[info]  Task LED - LED BLINK
[info]  Task BTN 2 - BTN HOVER
[info]  Task LED - LED OFF

>>>>>>> Stashed changes

