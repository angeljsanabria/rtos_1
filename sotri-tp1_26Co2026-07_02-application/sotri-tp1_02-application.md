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


