# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 2 - Comunicacion de Tareas de FreeRTOS

### Paso 01
Fue completado. Revisar el repo.

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

 

**2. ¿Como eliminar una Cola?**
Una Queue se elimina con la funcion de API void vQueueDelete( QueueHandle_t xQueue ). Esta funcion se encarga de liberar la memotira que el Kernel reservó para la estructura interna de la Cola.
Como parametro va el handler devuelto por xQueueCreate; 
ej: 
    (void)vQueueDelete(h_btn_led_q);
    h_btn_led_q = NULL;
El retorno es Nulo. Se recomienda como buena practica poner la variable del handler en NULL para evitar usos incorrectos a posterior.
 

**3. ¿Como gestiona una Cola los datos que contiene?**
El kernel de RTOS administra la Cola como un buffer circular del tipo FIFO con capacidad fija definida en xQueueCreate() (ej. 5 elementos de sizeof(task_led_ev_t)).

Al encolar, el kernel copia el elemento completo dentro del buffer interno; no guarda el puntero. Por eso la variable local del emisor puede salir de scope y el dato sigue valido en la cola.


La funcion xQueueSend( xQueue, ( void * ) &pxMessage, ( TickType_t ) 0 ); Se encarga de encolar el mensaje, o elemento, copiandolo en la Cola y aumenta un contador interno de mensajes pendientes uxMessagesWaiting si hay espacio. Si es el primer mensaje enviado va a ser el primer mensaje que se lee segun FIFO. 

La funcion xQueueReceive() se encarga de consumir los mensajes de la Cola. Copia desde el primer elemento, entrega el dato al buffer del receptor y decrementa uxMessagesWaiting.

Para consultar el estado:
- uxQueueMessagesWaiting(cola) → mensajes pendientes.
- uxQueueSpacesAvailable(cola) → lugares libres.


El orden por defecto es FIFO: el primer mensaje enviado es el primero que se recibe. Una forma de romper el orden FIFO es xQueueSendToFront(), que inserta al frente.

 

**4. ¿Como enviar datos a una Cola?**
Desde una task (no desde una ISR) se usa la familia `xQueueSend`:
```c
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
 ```
Parametros:
- xQueue — handler devuelto por xQueueCreate() (ej. h_btn_led_q).
- pvItemToQueue — puntero al dato a copiar (ej. &event).
- xTicksToWait — tiempo maximo de espera si la cola esta llena.

Retorno: 
- pdPASS si encolo correctamente; errQUEUE_FULL si no pudo (cola llena y timeout agotado o 0).

**5. ¿Como recibir datos de una Cola?**
Desde una **tarea** se usa:
```c
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
```
Parametros:
- xQueue — handler de la cola.
- pvBuffer — puntero donde se copiara el elemento recibido.
- xTicksToWait — tiempo maximo de espera si la cola esta vacia.

Retorno: 
- pdPASS (pdTRUE) si recibio un elemento; pdFAIL si no hubo dato en el tiempo indicado.

 

**6. ¿Que significa bloquearse en una Cola?**

Bloquearse significa que la tarea que llama a xQueueSend() o xQueueReceive() deja de ejecutarse y pasa al estado Blocked hasta que se cumpla la condicion que espera (o expire el timeout).

Casos comunes:
- xQueueSend(..., portMAX_DELAY) con cola llena → la tarea emisora queda bloqueada hasta que otra tarea haga Receive y libere un lugar.
- xQueueReceive(..., portMAX_DELAY) con cola vacia → la tarea receptora queda bloqueada hasta que otra tarea encole un mensaje.

Mientras esta bloqueada, no consume CPU: el scheduler ejecuta otras tareas listas. Cuando la condicion se cumple (o vence el timeout), la tarea pasa a Ready y vuelve a ejecutarse.

Si el timeout es 0, no se bloquea: la funcion retorna de inmediato con pdPASS o errQUEUE_FULL / pdFAIL segun haya espacio o datos.


 

**7. ¿Como bloquearse en varias Colas?**
Cuando una tarea debe esperar datos de mas de una cola (o cola + semaforo) al mismo tiempo, FreeRTOS ofrece los Queue Sets (conjuntos de colas).
Problema con xQueueReceive() en secuencia:
Si una tarea hace:
```c
xQueueReceive(cola_a, &dato_a, portMAX_DELAY);
xQueueReceive(cola_b, &dato_b, portMAX_DELAY);
```

Queda bloqueada en la primera cola aunque cola_b ya tenga mensajes. No espera en ambas a la vez.
 
 **Queue Sets**

Flujo:
- xQueueCreateSet(uxEventQueueLength) → crear el set (uxEventQueueLength = suma de longitudes de las colas del set).
- xQueueAddToSet(cola, set) → agregar cada cola o semaforo.
- xQueueSelectFromSet(set, xTicksToWait) → se bloquea hasta que algun miembro tenga datos; retorna su handle o NULL si vence el timeout.
- xQueueReceive((QueueHandle_t)miembro_activo, &dato, 0) → recibir con timeout 0 porque ya hay mensaje.

Asi la tarea pasa a Blocked una sola vez y el scheduler la despierta cuando cualquiera de las colas del set recibe un xQueueSend().

**8. ¿Como sobrescribir datos en una Cola?**

Se usa xQueueOverwrite() cuando solo importa el ultimo valor:



```c
BaseType_t xQueueOverwrite(QueueHandle_t xQueue, const void *pvItemToQueue);
```
Comportamiento:

- Cola vacia -> encola como un xQueueSend() normal.
- Cola llena -> reemplaza el elemento existente sin bloquear.

Retorno: 
- pdPASS (siempre escribe).
 
 Restriccion: solo funciona en colas de longitud 1 (xQueueCreate(1, ...)). En colas de longitud mayor no aplica.

**9. ¿Como vaciar una Cola?**

Hay dos formas comunes:

a) xQueueReset() — vacia la cola de una vez:
- xQueueReset(h_btn_led_q);
 Deja uxMessagesWaiting en 0. Las tareas bloqueadas en Receive no se desbloquean automaticamente.

b) Recibir y descartar en loop — con funciones ya usadas:

```c
task_led_ev_t testt;
while (xQueueReceive(h_btn_led_q, &testt, 0) == pdPASS)
{
    // vacio
}
```

Timeout 0 → no bloqueante; repite hasta que la cola quede vacia.


**10. ¿Cual es el efecto de las prioridades de las Tareas al escribir y leer en una Cola?**


La cola no prioriza mensajes: el kernel copia y entrega en FIFO. Lo que cambia con la prioridad de las tareas es **quien ejecuta** cuando alguien se bloquea o se desbloquea en xQueueSend() / xQueueReceive().


Misma prioridad:
- Send y Receive desbloquean tareas, pero ninguna preempta a la otra por prioridad.
- Se alternan segun el scheduler (time-slicing si esta habilitado, o quien quede Ready primero).


Prioridades distintas:
- Tarea emisora mas prioritaria que la receptora: puede encolar varios mensajes antes de que la receptora los consuma. Si la receptora estaba bloqueada en xQueueReceive(..., portMAX_DELAY), pasa a Ready al recibir un Send, pero **no ejecuta** hasta que la emisora se bloquee (ej. vTaskDelay).
- Tarea receptora mas prioritaria que la emisora: al hacer xQueueSend(), si la receptora estaba bloqueada en xQueueReceive(), pasa a Ready y puede **preemptar** de inmediato a la emisora.


Cola llena:
- Si una tarea de baja prioridad tiene la cola llena y otra de alta prioridad intenta xQueueSend(..., portMAX_DELAY), la de alta prioridad queda bloqueada hasta que la de baja haga Receive y libere espacio.




### Paso 03: Comunicacion `task_btn` ↔ `task_led` mediante Cola


// TODO: ANGEL COMPLETAR; 
Antes de la modificacion:
En el sistema anterior task_btn se comunica con un archivo/módulo de "interfaz" para impactar en un led seteando evento y flag con void put_event_task_led(task_led_ev_t event); 

Utilizamos la queue h_btn_led_q para comunicar las tareas.
Con xQueueSend(h_btn_led_q, &event, portMAX_DELAY) publicamos los eventos de boton presionado y release.
Del lado de la tarea del led con if(xQueueReceive(h_btn_led_q, &task_led_dta.event, 10) == pdTRUE)
leemos si tenemos un evento en la queue para modificar el event del handler del led y aplicamos el flag en True para que el cambio sea tomado por la FSM.

