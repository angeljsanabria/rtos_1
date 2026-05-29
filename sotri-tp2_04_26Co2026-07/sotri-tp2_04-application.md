# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 2 - Comunicacion de Tareas de FreeRTOS (Actividad 04)

### Paso 01
Fue completado. Revisar el repo.

### Paso 02: Interrupciones y FreeRTOS (respuestas en base a la depuracion del proyecto STM32)

> Proyecto: `sotri-tp2_04_26Co2026-07`  
> Completar cada respuesta segun la experiencia depurando en STM32CubeIDE.  
> Referencia: guia TP2 – Actividad 04, Paso 02.


**1. ¿Qué funciones de la API de FreeRTOS se pueden usar dentro de una rutina de servicio de interrupción?**
La mayoria de las primitivas de RTOS cubren el uso dentro de una rutina de interrupcion. Las APIs se pueden identificar porque terminan en FromISR(). No se recomienda usar las funciones normales de la API de RTOS (ej xSemaphoreGive, xQueueSend, vTaskDelay, etc.) porque pueden bloquear y el kernel no esta preparado para ese uso en el contexto de interrupcion.

- Colas:
	xQueueSendToBackFromISR(), xQueueSendFromISR(), xQueueReceiveFromISR(), xQueueOverwriteFromISR()
- Semáforos:
    xSemaphoreGiveFromISR(), xSemaphoreTakeFromISR()
- Demas funciones como Mutex, Notificaciones entre tareas, eventos de grupos, timer, scheduler y yield tambien tienen las API para el uso en rutina de interrupciones.




**2. ¿Métodos para delegar el procesamiento de interrupciones a una Tarea?**
El callback de una ISR debe ser corto: leer el hardware, reconocer el evento y avisar a una tarea. 
El procesamiento de la interrupcion puede continuar con funciones como xSemaphoreGiveFromISR, xQueueSendToBackFromISR, xTaskNotifyFromISR), más la macro portYIELD_FROM_ISR. Estas funciones no son bloqueantes ni consumen CPU.

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(h_btn_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    return;
}
```

---

**3. ¿Cómo usar una cola para transferir datos dentro y fuera de una rutina de servicio de interrupción?**
Una vez creada la Cola en la aplicacion.
```c
h_btn_led_q = xQueueCreate(5, sizeof(task_led_ev_t));
configASSERT(h_btn_led_q != NULL);
```

Un patron comun en la ISR, es reconocer y evaluar el evento que corresponda, y encolarlo en la Queue. Con las primitivas diseñadas para el procesamiento en las ISR.


```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    task_led_ev_t ev = EV_LED_BLINK;   // o el evento que corresponda
    if (GPIO_Pin == BTN_A_PIN)
    {
        xQueueSendToBackFromISR(h_btn_led_q, &ev, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

En la, o las, tareas que consumen la Queue recibir el evento con las funciones normales de la API de FreeRTOS (ej xQueueReceive) y hacer el procesamiento necesario.

```c
task_led_ev_t recibido;
if (xQueueReceive(h_btn_led_q, &recibido, portMAX_DELAY) == pdTRUE)
{
    // procesar recibido: EV_LED_BLINK, EV_LED_OFF, etc.
}
```

---

**4. ¿Cuál es el modelo de anidamiento de interrupciones disponible en algunas portaciones de FreeRTOS?**

Lo que agrega FreeRTOS es una regla de prioridades: no todas las IRQ pueden llamar a la API FromISR. Eso define tres bandas de prioridad.

- Banda 1 — Prioridades 0 a configMAX_SYSCALL_INTERRUPT_PRIORITY: Interrupciones enmascaradas por el kernel y pueden usar la API de RTOS.

- Banda 2 — Prioridades mayor igual a configMAX_SYSCALL_INTERRUPT_PRIORITY: El kernel no las retrasa y tiene prohibido usar las funciones de API de RTOS.

---

### Paso 03: Boton por interrupcion + semaforo binario hacia `task_btn`

Fue completado. Revisar el repo.


### Paso 04

Fue completado. Revisar el repo.