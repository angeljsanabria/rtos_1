# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 2 - Comunicacion de Tareas de FreeRTOS

### Paso 01
Fue completado. Revisar el repo.

// TODO: ANGEL; COMPLETAR 
### Paso 02: Colas de FreeRTOS (respuestas en base a la depuracion del proyecto STM32)

> Proyecto: `sotri-tp2_02_26Co2026-07`  
> Completar cada respuesta segun la experiencia depurando en STM32CubeIDE.

**1. ¿Como crear una Cola?**
Antes de crear la queue se necesita una variable para retener el  handler o manejador de la queue. El tipo de datos es QueueHandle_t.
static QueueHandle_t h_btn_led_q = NULL;

Para crearla es con la funcion xQueueCreate()
	QueueHandle_t xQueueCreate( UBaseType_t uxQueueLength, UBaseType_t uxItemSize );
Donde el primer valor indica la cantidad maxima de mensajes (numero de elementos) que puede contener la queue, y el segundo el tamaño de esos mensajes (o elementos) en bytes.
    ej: h_btn_led_q = xQueueCreate(5, sizeof(task_led_ev_t));
El retorno de la funcion es el handler allocado en memoria, si RTOS no puede allocar el handler la funcion de creacion retorna NULL. 
La practica recomenda es revisar si el puntero del handler es Null para bloquear en desarrollo el flujo del firmware y detectar errores.
    ej:
	h_btn_led_q = xQueueCreate(5, sizeof(task_led_ev_t));
	configASSERT(NULL != h_btn_led_q);

Si el puntero devuelto por xQueueCreate es distinto a Null, la Cola fue creada correctamente.

---

**2. ¿Como eliminar una Cola?**
Una Queue se elimina con la funcion de API void vQueueDelete( QueueHandle_t xQueue ). Esta funcion se encarga de liberar la memotira que el Kernel reservó para la estructura interna de la Cola.
Como parametro va el handler devuelto por xQueueCreate; 
ej: 
    (void)vQueueDelete(h_btn_led_q);
    h_btn_led_q = NULL;
El retorno es Nulo. Se recomienda como buena practica poner la variable del handler en NULL para evitar usos incorrectos a posterior.
---

**3. ¿Como gestiona una Cola los datos que contiene?**
El kernel de RTOS administra la Cola como un buffer circular del tipo FIFO. 
La funcion xQueueSend( xQueue, ( void * ) &pxMessage, ( TickType_t ) 0 ); Se encarga de encolar el mensaje o elemento de la Cola y aumenta un contador interno de mensajes pendientes. Si es el primer mensaje enviado va a ser el primer mensaje que se lee segun FIFO.
Una forma de romper con 

---

**4. ¿Como enviar datos a una Cola?**

---

**5. ¿Como recibir datos de una Cola?**

---

**6. ¿Que significa bloquearse en una Cola?**

---

**7. ¿Como bloquearse en varias Colas?**

---

**8. ¿Como sobrescribir datos en una Cola?**

---

**9. ¿Como vaciar una Cola?**

---

**10. ¿Cual es el efecto de las prioridades de las Tareas al escribir y leer en una Cola?**

---

### Paso 03: Comunicacion `task_btn` ↔ `task_led` mediante Cola


// TODO: ANGEL COMPLETAR; 
Antes de la modificacion:
En el sistema anterior task_btn se comunica con un archivo/módulo de "interfaz" para impactar en un led seteando evento y flag con void put_event_task_led(task_led_ev_t event); 

Utilizamos la queue h_btn_led_q para comunicar las tareas.
Con xQueueSend(h_btn_led_q, &event, portMAX_DELAY) publicamos los eventos de boton presionado y release.
Del lado de la tarea del led con if(xQueueReceive(h_btn_led_q, &task_led_dta.event, 10) == pdTRUE)
leemos si tenemos un evento en la queue para modificar el event del handler del led y aplicamos el flag en True para que el cambio sea tomado por la FSM.

