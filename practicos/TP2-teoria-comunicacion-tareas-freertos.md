# TP2 – Teoría y conceptos: Comunicación de Tareas de FreeRTOS

**Curso:** CESE – Sistemas Operativos de Tiempo Real  
**Fuentes:** Guía TP2 – Comunicación de Tareas de FreeRTOS (Rev. 01, 2026); *Hands-On RTOS with Microcontrollers* (Brian Amos, Packt 2020) — Capítulos 3, 7, 8, 9 y 10  
**Proyecto base:** `sotri-tp2_01-application` (demo botón → LED con FreeRTOS)

---

## Índice

1. [Panorama del TP2](#1-panorama-del-tp2)
2. [Conceptos previos indispensables](#2-conceptos-previos-indispensables)
3. [Problema que resuelve la comunicación entre tareas](#3-problema-que-resuelve-la-comunicación-entre-tareas)
4. [Colas (Queues)](#4-colas-queues)
5. [Semáforos binarios y contadores](#5-semáforos-binarios-y-contadores)
6. [Interrupciones y FreeRTOS](#6-interrupciones-y-freertos)
7. [Patrones de diseño del TP2](#7-patrones-de-diseño-del-tp2)
8. [Mapa actividad → conceptos → qué implementar](#8-mapa-actividad--conceptos--qué-implementar)
9. [Comparación con baremetal](#9-comparación-con-baremetal)
10. [Referencias y checklist de estudio](#10-referencias-y-checklist-de-estudio)
11. [Reforzamiento: Brian Amos (desde *The FreeRTOS Scheduler*)](#11-reforzamiento-brian-amos-desde-the-freertos-scheduler)

---

## 1. Panorama del TP2

### 1.1 Objetivos generales de la guía

La guía pide aprender a:

- **Comunicar tareas** de FreeRTOS usando colas y semáforos.
- **Integrar interrupciones** con el RTOS (ISR → tarea).
- Usar **STM32CubeIDE**, depuración y metodología portable.
- Identificar **patrones de diseño** (Event-Triggered Systems, delegación ISR→tarea).
- Documentar con **Markdown** y versionar con **Git/GitHub**.

### 1.2 Progresión de las cuatro actividades

| Actividad | Proyecto | Mecanismo de comunicación | Foco teórico |
|-----------|----------|---------------------------|--------------|
| **TP2-01** | `sotri-tp2_01-application` | Variable compartida + flag (`put_event_task_led`) | Arranque, SysTick, TIM, hooks, statecharts, tareas periódicas |
| **TP2-02** | `sotri-tp2_02-application` | **Cola** entre `task_btn` y `task_led` | Crear, enviar, recibir, bloqueo, prioridades, overwrite, vaciar |
| **TP2-03** | `sotri-tp2_03-application` | **Semáforo binario** entre tareas | Give/Take, diferencias con contador |
| **TP2-04** | `sotri-tp2_04-application` | **ISR del botón** + semáforo binario hacia `task_btn` | API FromISR, delegación, anidamiento de IRQ |

El proyecto base (`tp2_01`) **ya crea** la cola `h_btn_led_q` y el semáforo `h_btn_led_bin_sem` en `app.c`, pero **aún no los usa** en las tareas. Eso es intencional: cada actividad reemplaza el mecanismo de comunicación.

### 1.3 Comportamiento funcional del demo

Independientemente del mecanismo de comunicación, la lógica de aplicación es:

- Botón **libre** → LED **apagado**.
- Botón **presionado** (debounce 50 ms) → LED **parpadea** (toggle cada 500 ms).
- Botón **suelto** (debounce 50 ms) → LED **apagado**.

Secuencia típica en UART:

```
Task BTN - BTN PRESSED  →  Task LED - LED BLINK
Task BTN - BTN HOVER    →  Task LED - LED OFF
```

---

## 2. Conceptos previos indispensables

### 2.1 Tareas, scheduler y un solo núcleo

En un MCU como el STM32 (Cortex-M4, un núcleo):

- Solo **una tarea ejecuta a la vez** (no hay paralelismo real).
- El **scheduler** elige qué tarea corre según **prioridad** y **estado**.
- Una tarea puede estar: **Running**, **Ready**, **Blocked** o **Suspended**.

**Regla clave:** la tarea de **mayor prioridad** que esté Ready ejecuta primero. Si se bloquea (`vTaskDelay`, cola vacía, semáforo sin token), el scheduler elige otra.

### 2.2 Bloqueo vs busy-wait

| Enfoque | Qué hace la CPU | Uso en TP2 |
|---------|-----------------|------------|
| **Busy-wait** (`while (!flag);`) | Consume CPU esperando | No recomendado con RTOS |
| **Bloqueo RTOS** (`vTaskDelay`, `xQueueReceive` con timeout) | La tarea cede el CPU al scheduler | Correcto: otras tareas pueden ejecutar |

### 2.3 Delays periódicos

- **`vTaskDelay(ticks)`** – delay **relativo**: duerme al menos N ticks desde el momento de la llamada. Puede acumular **jitter** si el procesamiento previo varía.
- **`vTaskDelayUntil(&last_wake, ticks)`** – delay **absoluto**: intenta ejecutar cada N ticks desde un instante de referencia. Usado en `task_led` para periodo estable de 50 ms.

En baremetal equivalente: un `delay_ms()` bloqueante en el `main()` que impide hacer otra cosa. Con RTOS, la tarea duerme y **otras tareas siguen ejecutando**.

### 2.4 Statecharts (máquinas de estados)

Tanto `task_btn` como `task_led` usan **Run-to-Completion statecharts**:

- **Eventos** de entrada: lectura GPIO (BTN) o flag/event (LED).
- **Estados**: UP, FALLING, DOWN, RISING (botón); OFF, BLINK (LED).
- **Transiciones** con condiciones de tiempo (`DEL_BTN_MAX = 50 ms`, `DEL_LED_MAX = 500 ms`).

Esto modela un sistema **orientado a eventos (ETS)**: las transiciones ocurren por eventos, no solo por tiempo fijo.

### 2.5 Infraestructura de tiempo e interrupciones (Actividad 01)

Conceptos que la guía pide analizar en el arranque del sistema:

| Recurso | Rol típico en el proyecto |
|---------|---------------------------|
| **SysTick** | Tick del kernel FreeRTOS (1 ms): `vTaskDelay`, `xTaskGetTickCount` |
| **TIM1 (HAL timebase)** | `HAL_IncTick()` → variable `uwTick` para delays HAL |
| **TIM2** | Contador de alta frecuencia para **runtime stats** de FreeRTOS (`configGENERATE_RUN_TIME_STATS`) |
| **EXTI (botón B1)** | Cableada en CubeMX; en `tp2_01` el callback está vacío (polling en tarea) |
| **PendSV / SVC** | Cambio de contexto del scheduler (infraestructura, no lógica de app) |

**Hooks** (en `freertos.c` de la app):

- `vApplicationIdleHook` – se ejecuta en la tarea Idle (prioridad mínima).
- `vApplicationTickHook` – se ejecuta en cada tick (desde ISR de SysTick; debe ser muy corta).
- `vApplicationStackOverflowHook` – detecta desbordamiento de pila.

---

## 3. Problema que resuelve la comunicación entre tareas

### 3.1 Variable compartida (mecanismo actual en tp2_01)

```c
void put_event_task_led(task_led_ev_t event)
{
    task_led_dta.event = event;
    task_led_dta.flag = true;
}
```

**Ventajas:** simple, rápido, fácil de entender.

**Riesgos:**

- **Condición de carrera**: si dos escritores modifican `event`/`flag` sin protección, puede perderse un evento.
- **Sin buffer**: si llegan varios eventos antes de que `task_led` los procese, solo queda el último (si no hay cola).
- **No bloquea** al receptor: `task_led` debe **consultar** el flag periódicamente (polling de eventos).

En el demo actual funciona porque hay **un solo productor** (`task_btn`), **un consumidor** (`task_led`), misma prioridad y eventos espaciados por debounce. En sistemas reales, esto suele ser insuficiente.

### 3.2 Por qué colas y semáforos

| Necesidad | Primitiva adecuada |
|-----------|-------------------|
| Enviar **datos** (eventos, muestras, comandos) de A a B | **Cola** |
| **Sincronizar** (“algo ocurrió, despertame”) sin transferir dato grande | **Semáforo binario** |
| Contar **N recursos** o **N eventos pendientes** | **Semáforo contador** |
| Proteger **sección crítica** compartida | **Mutex** (no es foco principal del TP2) |

---

## 4. Colas (Queues)

> **Actividad TP2-02:** reemplazar `put_event_task_led()` por comunicación con cola.

### 4.1 Qué es una cola en FreeRTOS

Una cola es un **buffer FIFO** administrado por el kernel:

- Tiene **capacidad fija** (N elementos).
- Cada elemento tiene **tamaño fijo** (`sizeof(tipo)`).
- Las operaciones **copian** el dato (no se guarda un puntero salvo que el elemento sea un puntero).
- Es **thread-safe**: el kernel protege acceso concurrente entre tareas.

En el proyecto:

```c
h_btn_led_q = xQueueCreate(5, sizeof(task_led_ev_t));
```

Crea una cola de **5 eventos** del tipo `task_led_ev_t` (`EV_LED_OFF`, `EV_LED_BLINK`).

### 4.2 Cómo crear una cola

```c
QueueHandle_t cola;

cola = xQueueCreate(
    5,                      /* uxQueueLength: cantidad maxima de items */
    sizeof(task_led_ev_t)   /* uxItemSize: tamano de cada item en bytes */
);

if (cola == NULL) {
    /* Fallo: sin memoria en el heap de FreeRTOS */
}
```

**Requisitos:**

- Crear **antes** de usar (`app_init`, antes de `xTaskCreate`).
- Verificar retorno (`configASSERT` en el curso).
- Opcional: `vQueueAddToRegistry(cola, "nombre")` para depuración/trace.

### 4.3 Cómo gestiona una cola los datos

Conceptualmente la cola tiene:

```
[ item0 | item1 | item2 | ... | ___ | ___ ]  ← capacidad = 5
  ↑                           ↑
  frente (Receive)            espacio libre
```

- **Send** copia un item al **final** (FIFO) si hay espacio.
- **Receive** copia un item desde el **frente** si hay datos.
- Internamente FreeRTOS usa estructuras de lista y puede bloquear tareas en colas de espera (blocked on send / blocked on receive).

Funciones útiles de inspección:

```c
UBaseType_t libres   = uxQueueSpacesAvailable(cola);
UBaseType_t pendientes = uxQueueMessagesWaiting(cola);
```

### 4.4 Cómo enviar datos a una cola

API principal desde **tareas** (no ISR):

```c
task_led_ev_t evento = EV_LED_BLINK;

/* Al final de la cola (FIFO tipico) */
xQueueSendToBack(cola, &evento, timeout_ticks);

/* Al frente (prioridad, menos usado en TP2) */
xQueueSendToFront(cola, &evento, timeout_ticks);

/* Alias generico */
xQueueSend(cola, &evento, timeout_ticks);
```

**Parámetro `timeout`:**

| Valor | Comportamiento |
|-------|----------------|
| `0` | **No bloqueante**: si la cola está llena, retorna de inmediato con error |
| `portMAX_DELAY` | **Bloqueante indefinido**: espera hasta que haya espacio |
| `N` ticks | Espera como máximo N ticks; retorna si expira el timeout |

Retorno: `pdPASS` si tuvo éxito, `errQUEUE_FULL` si no pudo enviar (cola llena y timeout 0 o expirado).

**Ejemplo de aplicación TP2** – en `task_btn`, al confirmar presión:

```c
task_led_ev_t ev = EV_LED_BLINK;
if (xQueueSendToBack(h_btn_led_q, &ev, pdMS_TO_TICKS(0)) != pdPASS) {
    /* Cola llena: decidir overwrite, descartar o loguear */
}
```

### 4.5 Cómo recibir datos de una cola

```c
task_led_ev_t recibido;

if (xQueueReceive(h_btn_led_q, &recibido, timeout_ticks) == pdPASS) {
    task_led_dta.event = recibido;
    task_led_dta.flag = true;
}
```

| Timeout | Comportamiento |
|---------|----------------|
| `0` | No bloquea si la cola está vacía |
| `portMAX_DELAY` | Espera hasta que llegue un item |
| `N` ticks | Espera limitada |

### 4.6 Qué significa bloquearse en una cola

**Bloquearse** = la tarea pasa a estado **Blocked** y **no consume CPU** hasta que:

- En **Receive**: llegue un item a la cola, o expire el timeout.
- En **Send**: haya espacio libre, o expire el timeout.

Mientras está bloqueada, el scheduler ejecuta **otras tareas** (Idle, otras Ready de igual o mayor prioridad cuando corresponda).

**Ejemplo conceptual:**

```
task_led hace xQueueReceive(cola, ..., portMAX_DELAY)
  → cola vacía → task_led BLOCKED
  → task_btn envía EV_LED_BLINK
  → task_led pasa a READY → scheduler la ejecuta
```

En baremetal no existe esto: tendrías un `while (cola_vacia)` consumiendo CPU.

### 4.7 Cómo bloquearse en varias colas

FreeRTOS ofrece **`xQueueSelectFromSet()`** con un **Queue Set** para esperar en **varias colas a la vez** (o colas + semáforos en el set).

Flujo:

1. Crear set: `xQueueCreateSet(total_items)`.
2. Agregar colas: `xQueueAddToSet(cola_a, set)`, etc.
3. Esperar: `handle = xQueueSelectFromSet(set, timeout)`.
4. Recibir del handle que se activó.

**Relevancia TP2:** no es obligatorio en el demo de dos tareas, pero la guía lo menciona como concepto para sistemas con múltiples fuentes de eventos.

### 4.8 Cómo sobrescribir datos en una cola

Cuando la cola tiene **longitud 1** (o se quiere conservar solo el último valor):

```c
xQueueOverwrite(cola, &evento);
```

- Si la cola está **vacía**, se comporta como un send normal.
- Si está **llena**, **reemplaza** el item existente sin bloquear.

**Caso de uso TP2:** si el botón genera eventos más rápido de lo que `task_led` consume, y solo importa el **estado más reciente** (BLINK u OFF), una cola de tamaño 1 + `xQueueOverwrite` evita bloqueos y descartes silenciosos.

> **Nota:** `xQueueOverwrite` solo es válido para colas de longitud **1**.

### 4.9 Cómo vaciar una cola

Opciones:

```c
/* Vaciar todos los items */
xQueueReset(cola);

/* Recibir y descartar en loop */
task_led_ev_t dummy;
while (xQueueReceive(cola, &dummy, 0) == pdPASS) {
    /* descartar */
}
```

`xQueueReset` deja la cola vacía. Las tareas bloqueadas en receive **no** se desbloquean automáticamente con reset (comportamiento a tener en cuenta al depurar).

### 4.10 Cómo eliminar una cola

```c
vQueueDelete(cola);
cola = NULL;
```

Libera la memoria del kernel asociada. **Ninguna tarea debe usar la cola después.** No se usa en el flujo normal del TP2, pero la Actividad 02 lo pregunta teóricamente.

### 4.11 Efecto de las prioridades al escribir y leer en una cola

Escenario típico del demo: `task_btn` y `task_led` con **prioridad 1** (iguales).

**Misma prioridad:**

- Se alternan por **time-slicing** (si está habilitado) o por quien queda Ready primero.
- Send/Receive no “priorizan” una tarea por sí solos; importa quién está Ready y el orden del scheduler.

**Prioridades distintas – ejemplo:**

- `task_btn` prioridad **2**, `task_led` prioridad **1**.
- Si ambas compiten, **`task_btn` ejecuta primero** hasta bloquearse.
- Si `task_btn` hace `xQueueSend` y `task_led` estaba bloqueada en `xQueueReceive`, al enviar **`task_led` pasa a Ready**. Si su prioridad es **menor**, **no ejecuta aún** hasta que `task_btn` se bloquee (p. ej. `vTaskDelay`).
- Si `task_led` tuviera prioridad **mayor** que `task_btn`, al desbloquearse por Receive podría **preemptar** inmediatamente a `task_btn` (según configuración de preemptividad).

**Conclusión para el informe:** las colas **desbloquean** tareas, pero **quién corre después** depende de **prioridades** y del scheduler preemptivo.

### 4.12 Diseño recomendado para TP2-02

Patrón productor-consumidor:

```
task_btn (productor)                    task_led (consumidor)
     |                                        |
     |  xQueueSendToBack(EV_LED_*)            |  xQueueReceive (0 o timeout corto)
     v                                        v
              h_btn_led_q [ FIFO de eventos ]
```

- **Send** desde `task_btn`: suele ser **no bloqueante** (`timeout = 0`) o `Overwrite` si cola tamaño 1.
- **Receive** en `task_led`: puede ser **no bloqueante** dentro del loop de 50 ms (equivalente al flag actual) o **bloqueante** si se quiere que `task_led` duerma hasta evento.

---

## 5. Semáforos binarios y contadores

> **Actividad TP2-03:** comunicación `task_btn` ↔ `task_led` con semáforo binario.  
> **Actividad TP2-04:** ISR → `task_btn` con semáforo binario.

### 5.1 Qué es un semáforo

Un semáforo es un **contador de permisos** administrado por el kernel:

- **`Take`** (wait): si hay permiso, lo consume; si no, la tarea puede **bloquearse**.
- **`Give`** (signal): devuelve/incrementa un permiso; puede **desbloquear** una tarea en espera.

En FreeRTOS, semáforos binarios y contadores se implementan sobre la misma estructura interna de cola.

### 5.2 Semáforo binario

```c
SemaphoreHandle_t sem;

sem = xSemaphoreCreateBinary();
/* Estado inicial: VACIO (no se puede Take sin un Give previo) */
```

| Operación (tarea) | Efecto |
|-------------------|--------|
| `xSemaphoreGive(sem)` | Si hay tarea bloqueada en Take → la desbloquea. Si no, deja el semáforo “disponible” (máx. 1). |
| `xSemaphoreTake(sem, timeout)` | Si disponible → lo toma. Si no → bloquea o timeout. |

**Limitación importante:** un semáforo binario **no acumula** múltiples Give si nadie hace Take. Sirve para **sincronización** (“ocurrió un evento”), no para contar 10 pulsaciones pendientes.

### 5.3 Semáforo contador

```c
SemaphoreHandle_t sem_cont;

sem_cont = xSemaphoreCreateCounting(
    max_count,    /* valor maximo */
    initial_count /* valor inicial */
);
```

| Aspecto | Binario | Contador |
|---------|---------|----------|
| Valores posibles | 0 o 1 | 0 … max_count |
| Give con nadie esperando | Queda “pendiente” (1) | Incrementa contador hasta max |
| Usos típicos | Evento, handshake, ISR→tarea | Pool de N buffers, N interrupciones pendientes |
| En TP2 | Sincronizar btn↔led o ISR↔tarea | Si hubiera ráfaga de pulsaciones a procesar |

### 5.4 Cómo crear y usar (desde tareas)

Creación (ya en `app.c` del proyecto base):

```c
h_btn_led_bin_sem = xSemaphoreCreateBinary();
configASSERT(h_btn_led_bin_sem != NULL);
```

Uso típico – **señalización**:

```c
/* Productor / ISR delegada */
xSemaphoreGive(h_btn_led_bin_sem);

/* Consumidor */
if (xSemaphoreTake(h_btn_led_bin_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
    /* Ocurrio evento: procesar */
}
```

Con timeout `portMAX_DELAY`, la tarea **duerme** hasta que alguien haga Give.

### 5.5 Diferencias prácticas binario vs contador

**Ejemplo contador:** buffer pool de 4 slots UART RX.

```c
sem_buf = xSemaphoreCreateCounting(4, 4);  /* 4 buffers libres */

/* Al tomar buffer */
xSemaphoreTake(sem_buf, portMAX_DELAY);

/* Al liberar buffer */
xSemaphoreGive(sem_buf);
```

**Ejemplo binario:** “llegó interrupción del botón, despertá a `task_btn`”.

```c
/* En HAL_GPIO_EXTI_Callback (via GiveFromISR) */
xSemaphoreGiveFromISR(h_btn_sem, &xHigherPriorityTaskWoken);

/* En task_btn */
xSemaphoreTake(h_btn_sem, portMAX_DELAY);
/* Procesar flanco, debounce en tarea, no en ISR */
```

### 5.6 Limitación del semáforo para el demo TP2

Un semáforo binario **no transporta** `EV_LED_BLINK` vs `EV_LED_OFF`. Solo dice “pasó algo”.

Por eso en TP2-03 la guía combina semáforo con **otra variable** (p. ej. actualizar `task_led_dta.event` antes del Give, o alternar estado en `task_led` al recibir Take). En `semaphore_bin.txt` del ref se ve un patrón de **toggle** del evento en `task_led` al hacer Take.

**Comparación:**

| Mecanismo | ¿Lleva el tipo de evento? | ¿Desbloquea receptor? |
|-----------|---------------------------|------------------------|
| Cola | Sí (`task_led_ev_t`) | Sí |
| Semáforo binario | No (solo señal) | Sí |
| Variable + flag | Sí (pero sin protección kernel) | No (polling) |

---

## 6. Interrupciones y FreeRTOS

> **Actividad TP2-04:** botón por interrupción + semáforo hacia `task_btn`.

### 6.1 Regla de oro: dos mundos

| Contexto | Qué API usar |
|----------|--------------|
| **Tarea** | `xQueueSend`, `xSemaphoreGive`, `vTaskDelay`, … |
| **ISR** | Solo funciones **`...FromISR`** y macros asociadas |

Usar API normal dentro de una ISR **corrompe el kernel** (no está diseñada para reentrancia desde IRQ).

### 6.2 Funciones API usables en ISR (conceptual)

Familias permitidas (sufijo **`FromISR`**):

| Categoría | Ejemplos |
|-----------|----------|
| Colas | `xQueueSendToBackFromISR`, `xQueueSendFromISR`, `xQueueOverwriteFromISR`, `xQueueReceiveFromISR` |
| Semáforos | `xSemaphoreGiveFromISR`, `xSemaphoreTakeFromISR` (TakeFromISR poco usado) |
| Notificaciones | `xTaskNotifyFromISR`, `vTaskNotifyGiveFromISR` |
| Scheduler | `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` |

También suelen permitirse (según port/doc):

- `xTaskGetSchedulerStateFromISR()`
- Operaciones de **Event Groups** FromISR (si están habilitadas)

**Prohibido en ISR:** `vTaskDelay`, `xQueueSend` (sin FromISR), `printf` largo, malloc, mutex take con bloqueo, etc.

### 6.3 Patrón obligatorio en ISR

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == B1_Pin) {
        xSemaphoreGiveFromISR(h_btn_led_bin_sem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

**`xHigherPriorityTaskWoken`:**

- Entrada: **siempre** `pdFALSE`.
- Salida: el kernel la pone en `pdTRUE` si un Give/QueueFromISR despertó una tarea de **prioridad mayor** que la interrumpida.
- **`portYIELD_FROM_ISR`:** solicita cambio de contexto **al salir de la ISR** (no en medio de la ISR).

### 6.4 Métodos para delegar procesamiento de ISR a una tarea

La ISR debe ser **corta**. El procesamiento pesado va en tarea.

| Método | Idea | TP2 |
|--------|------|-----|
| **Semáforo binario** | ISR Give → tarea Take → debounce/statechart | Actividad 04 |
| **Cola** | ISR envía dato/evento → tarea Receive | Variante posible |
| **Task Notification** | ISR notifica directamente a un TaskHandle | Alternativa moderna (no exigida en guía) |
| **Deferred interrupt handler** | Cola dedicada a punteros de función | Patrón avanzado FreeRTOS |

**Flujo TP2-04 objetivo:**

```
Flanco boton (hardware)
    → EXTI15_10_IRQHandler
    → HAL_GPIO_EXTI_IRQHandler
    → HAL_GPIO_EXTI_Callback  (minimo: GiveFromISR)
    → task_btn bloqueada en Take se despierta
    → debounce + statechart en task_btn (como ahora)
    → comunicacion hacia task_led (cola/sem segun actividad)
```

### 6.5 Cola para transferir datos dentro y fuera de una ISR

**ISR → tarea:**

```c
task_led_ev_t ev = EV_LED_BLINK;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

xQueueSendToBackFromISR(h_btn_led_q, &ev, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

**Tarea → ISR:** poco frecuente; normalmente la tarea escribe hardware o flags volátiles; la ISR lee. Si hace falta, la tarea envía a cola y una tarea de baja prioridad o la misma ISR de otro evento consume.

**Restricción:** en Cortex-M, la **prioridad numérica de la IRQ** debe ser **compatible** con `configMAX_SYSCALL_INTERRUPT_PRIORITY` (interrupciones más prioritarias que ese umbral **no pueden** llamar API FreeRTOS).

### 6.6 Modelo de anidamiento de interrupciones

En portaciones Cortex-M con FreeRTOS:

- IRQ de **prioridad numérica baja** = prioridad lógica **alta** (atención: convención ARM invertida).
- FreeRTOS enmascara interrupciones hasta `configMAX_SYSCALL_INTERRUPT_PRIORITY` durante critical sections del kernel.
- IRQ **por encima** de ese umbral: **no** deben usar FromISR; solo flags/hardware.
- IRQ **por debajo** (menos urgentes numéricamente): pueden usar FromISR.

**Anidamiento:** una ISR puede ser interrumpida por otra de mayor urgencia si las prioridades NVIC lo permiten. El kernel asume que las API FromISR se usan desde IRQ de prioridad **permitida**.

Para el informe TP2-04: explicar que en STM32 + FreeRTOS hay un **rango de prioridades NVIC “RTOS-safe”** y otro **“no RTOS-safe”**.

---

## 7. Patrones de diseño del TP2

### 7.1 Event-Triggered Systems (ETS)

El demo es **orientado a eventos**:

- Eventos de botón: UP, DOWN (derivados de GPIO).
- Eventos de LED: BLINK, OFF.
- Statecharts procesan eventos en **ráfagas Run-to-Completion**.

RTOS agrega: eventos entre tareas vía cola/semáforo en lugar de flags globales.

### 7.2 Productor – consumidor

```
[Sensor / BTN / ISR]  --evento-->  [Cola o Sem]  -->  [Actuador / LED task]
```

### 7.3 ISR mínima + tarea de servicio

Separación de responsabilidades:

| Capa | Responsabilidad | Tiempo máximo |
|------|-----------------|---------------|
| ISR | Detectar hardware, señalizar | Microsegundos |
| Tarea | Debounce, lógica, comunicación | Milisegundos |

En baremetal suele mezclarse debounce en ISR o flags volátiles sin scheduler; con RTOS la tarea bloqueada **no consume CPU** mientras espera el Give.

### 7.4 Evolución por actividad (mismo comportamiento, distinto acoplamiento)

```
tp2_01:  task_btn --put_event--> task_led_dta (memoria compartida)
tp2_02:  task_btn --QueueSend--> cola --QueueReceive--> task_led
tp2_03:  task_btn --Give/Take--> sem binario + event en struct
tp2_04:  ISR --GiveFromISR--> sem --> task_btn --...--> task_led
```

---

## 8. Mapa actividad → conceptos → qué implementar

### Actividad 01 – Análisis (`sotri-tp2_01-application`)

**Aprender:**

- Secuencia de arranque: Reset → `main()` → HAL init → `app_init()` → `osKernelStart()`.
- Rol de SysTick vs TIM1 vs TIM2.
- Estructura de tareas, statecharts, `vTaskDelay` vs `vTaskDelayUntil`.
- Hooks y contadores globales de depuración.

**No implementar aún:** uso real de cola/semáforo/ISR del botón.

---

### Actividad 02 – Cola (`sotri-tp2_02-application`)

**Preguntas teóricas de la guía → respuesta resumida:**

| Pregunta | Concepto clave |
|----------|----------------|
| ¿Cómo crear? | `xQueueCreate(n, sizeof(tipo))` |
| ¿Cómo gestiona datos? | FIFO, copia por valor, thread-safe |
| ¿Cómo enviar? | `xQueueSendToBack/Front`, timeout |
| ¿Cómo recibir? | `xQueueReceive`, timeout |
| ¿Bloquearse? | Tarea Blocked hasta dato o timeout |
| ¿Varias colas? | `xQueueCreateSet` + `xQueueSelectFromSet` |
| ¿Sobrescribir? | `xQueueOverwrite` (cola length = 1) |
| ¿Vaciar? | `xQueueReset` o receive en loop |
| ¿Eliminar? | `vQueueDelete` |
| ¿Prioridades? | Desbloqueo ≠ ejecución inmediata; gana el scheduler según prio |

**Implementar:**

- En `task_btn`: reemplazar `put_event_task_led()` por `xQueueSendToBack`.
- En `task_led`: reemplazar lectura de flag por `xQueueReceive` (ajustar statechart).
- Documentar en `.md` comportamiento observado al depurar.

---

### Actividad 03 – Semáforo binario (`sotri-tp2_03-application`)

**Preguntas teóricas:**

| Pregunta | Concepto clave |
|----------|----------------|
| ¿Crear y usar? | `xSemaphoreCreateBinary`, Give/Take con timeout |
| ¿Diferencias binario vs contador? | Máx. 1 vs N; sincronización vs conteo de recursos |

**Implementar:**

- Sincronizar `task_btn` y `task_led` con Give/Take.
- Definir cómo se transmite BLINK vs OFF (variable compartida protegida por diseño, o toggle según ref `semaphore_bin.txt`).

---

### Actividad 04 – ISR + semáforo (`sotri-tp2_04-application`)

**Preguntas teóricas:**

| Pregunta | Concepto clave |
|----------|----------------|
| ¿API en ISR? | Solo `...FromISR` + `portYIELD_FROM_ISR` |
| ¿Delegar a tarea? | ISR corta + GiveFromISR + Take en tarea |
| ¿Cola desde ISR? | `xQueueSendToBackFromISR` |
| ¿Anidamiento? | Prioridades NVIC vs `configMAX_SYSCALL_INTERRUPT_PRIORITY` |

**Implementar:**

- Lógica en `HAL_GPIO_EXTI_Callback` (`app_it.c`).
- `task_btn` espera evento por semáforo (parte del debounce puede moverse desde polling puro a “despertar por IRQ”).
- Verificar prioridad EXTI en `.ioc` compatible con FreeRTOS.

---

## 9. Comparación con baremetal

| Tema | Baremetal | FreeRTOS TP2 |
|------|-----------|--------------|
| Loop principal | Un `while(1)` con todo mezclado | Varias tareas con responsabilidades separadas |
| Esperar evento | `while (!flag);` o polling en loop | Block en cola/semáforo o `vTaskDelay` |
| Botón | ISR setea flag volátil | ISR GiveFromISR → tarea procesa |
| Comunicación | Variables globales + `volatile` | Cola (dato + sync) o semáforo (sync) |
| Tiempo | SysTick único | SysTick kernel + TIM HAL + TIM stats |
| Depuración | Contadores manuales | Hooks, registry de colas, runtime stats |

**Ventaja RTOS:** modularidad, bloqueo sin quemar CPU, herramientas de sync del kernel.  
**Costo:** heap, context switches, reglas estrictas en ISR, curva de aprendizaje.

---

## 10. Referencias y checklist de estudio

### 10.1 Documentación oficial FreeRTOS y libro del curso

**FreeRTOS (oficial):**

- [Queues](https://www.freertos.org/Embedded-RTOS-Queues.html)
- [Binary Semaphores](https://www.freertos.org/Embedded-RTOS-Binary-Semaphores.html)
- [Counting Semaphores](https://www.freertos.org/Embedded-RTOS-Counting-Semaphores.html)
- [Interrupt Management](https://www.freertos.org/a00122.html)
- [RTOS API in ISR](https://www.freertos.org/a00122.html)

**Brian Amos – *Hands-On RTOS with Microcontrollers* (reforzamiento teórico, Sección 11):**

- Cap. 7 – Scheduler, estados, `xTaskCreate`, anti-polling
- Cap. 8 – Semáforos, polling vs bloqueo, timeouts
- Cap. 9 – Colas pass-by-value, prioridades + cola llena
- Cap. 10 – ISRs, `FromISR`, prioridades NVIC

### 10.2 Archivos del proyecto a estudiar

| Archivo | Contenido |
|---------|-----------|
| `APP/src/app.c` | Creación cola, semáforo, tareas |
| `APP/src/task_btn.c` | Productor de eventos, debounce |
| `APP/src/task_led.c` | Consumidor, blink periódico |
| `APP/src/task_led_interface.c` | Interfaz actual (memoria compartida) |
| `APP/src/app_it.c` | Callbacks de interrupción (TP2-04) |
| `APP/src/freertos.c` | Hooks de aplicación |
| `Core/Inc/FreeRTOSConfig.h` | Config kernel, stats, hooks |
| `app/queue.txt` (ref) | Snippets de cola para TP2-02 |
| `app/semaphore_bin.txt` (ref) | Snippets de semáforo para TP2-03/04 |

### 10.3 Checklist antes de entregar cada actividad

- [ ] ¿Compila sin warnings críticos?
- [ ] ¿Comportamiento LED/botón coincide con la especificación?
- [ ] ¿Se usó la API correcta (tarea vs FromISR)?
- [ ] ¿Se verificó timeout bloqueante vs no bloqueante?
- [ ] ¿El informe `.md` responde **todas** las preguntas de la guía?
- [ ] ¿Se documentó qué se observó en depuración (no solo teoría)?
- [ ] ¿README y repositorio GitHub actualizados?

---

## 11. Reforzamiento: Brian Amos (desde *The FreeRTOS Scheduler*)

Esta sección complementa las secciones 2–6 con extractos y conceptos del libro *Hands-On RTOS with Microcontrollers* (Brian Amos). El autor introduce colas y semáforos en el **Capítulo 3** (*Task Signaling and Communication Mechanisms*) de forma conceptual; el **Capítulo 7** (*The FreeRTOS Scheduler*) profundiza tareas y estados; el **Capítulo 8** (*Protecting Data and Synchronizing Tasks*) cubre semáforos en código real; el **Capítulo 9** (*Intertask Communication*) detalla colas; y el **Capítulo 10** (*Drivers and ISRs*) une interrupciones con la API FromISR.

**Mapa libro → TP2:**

| Capítulo Amos | Tema central | Actividad TP2 |
|---------------|--------------|---------------|
| 7 – Scheduler | Crear tareas, estados, bloqueo, polling vs ISR | TP2-01 |
| 3 – Primitivas (intro) | Colas FIFO, semáforos binarios/contadores | Base teórica TP2-02/03 |
| 8 – Sincronización | Give/Take, polling ineficiente, timeouts | TP2-03 |
| 9 – Comunicación | Pass-by-value, prioridades + colas | TP2-02 |
| 10 – Drivers e ISRs | ISR corta, `FromISR`, cola desde IRQ | TP2-04 |

---

### 11.1 Capítulo 7 – The FreeRTOS Scheduler

#### Por qué importa para el TP2

Antes de usar colas o semáforos entre `task_btn` y `task_led`, hay que dominar **cómo el kernel elige qué tarea corre** y **cuándo una tarea deja de consumir CPU**. Sin eso, no se entiende por qué `xQueueReceive` con timeout libera el procesador o por qué una ISR con `GiveFromISR` despierta una tarea.

#### Arranque de una aplicación RTOS (traducción y síntesis)

Amos resume cuatro pasos obligatorios antes de que el sistema sea “RTOS” de verdad:

> *In order to get an RTOS application up and running, a few things need to happen:*
> 1. *The MCU hardware needs to be initialized.*
> 2. *Task functions need to be defined.*
> 3. *RTOS tasks need to be created and mapped to the functions that were defined in step 2.*
> 4. *The RTOS scheduler must be started.*

**Traducción:** para poner en marcha una aplicación RTOS hay que: (1) inicializar el hardware del MCU; (2) definir las funciones de tarea; (3) crear las tareas RTOS y asociarlas a esas funciones; (4) arrancar el scheduler.

En tu proyecto TP2 esto ocurre así:

| Paso Amos | Equivalente en `sotri-tp2_01-application` |
|-----------|---------------------------------------------|
| HW init | `main()` → HAL init → `app_init()` |
| Definir funciones | `task_btn()`, `task_led()` |
| Crear tareas | `xTaskCreate()` en `app.c` |
| Arrancar scheduler | `osKernelStart()` / `vTaskStartScheduler()` desde `main()` |

Amos enfatiza un detalle crítico:

> *Because the call to start the scheduler doesn't return, it won't be possible to start a task from main after making a call to start the scheduler. Once the scheduler is started, tasks can create new tasks as necessary.*

**Traducción:** como `vTaskStartScheduler()` no retorna, **no podés crear tareas desde `main()` después de arrancar el scheduler**. Toda creación previa debe hacerse en `app_init()` (como ya hace tu `app.c`).

#### Qué es una tarea (modelo mental del autor)

> *Remember – a task is really just an infinite while loop with its own stack and a priority.*

**Traducción:** una tarea es, en esencia, un **`while(1)` infinito con pila propia y prioridad**.

Comparación baremetal: en un super-loop hay **un solo** `while(1)` en `main()` que llama funciones en secuencia. Con RTOS hay **varios** loops infinitos “aislados”, intercalados por el scheduler. Eso explica por qué `task_btn` puede hacer `vTaskDelay(50)` sin frenar permanentemente a `task_led`.

#### `xTaskCreate` – parámetros que Amos desglosa

El prototipo que cita el libro coincide con el de FreeRTOS:

```c
BaseType_t xTaskCreate( TaskFunction_t pvTaskCode,
                        const char * const pcName,
                        configSTACK_DEPTH_TYPE usStackDepth,
                        void *pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t *pxCreatedTask );
```

| Parámetro | Rol | En tu TP2 |
|-----------|-----|-----------|
| `pvTaskCode` | Función con el loop de la tarea | `task_btn`, `task_led` |
| `pcName` | Nombre para depuración | `"Task BTN"`, `"Task LED"` |
| `usStackDepth` | Profundidad de pila en **palabras** | `2 * configMINIMAL_STACK_SIZE` |
| `pvParameters` | Argumento al crear (puede ser `NULL`) | `NULL` |
| `uxPriority` | Prioridad numérica | `tskIDLE_PRIORITY + 1` (ambas = 1) |
| `pxCreatedTask` | Handle para referenciar/borrar tarea | `&h_task_btn`, `&h_task_led` |

Amos insiste:

> *You must check this return value!*  
> *Return value: either pdPASS or errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY*

En tu proyecto se usa `configASSERT(pdPASS == ret)`, alineado con la recomendación del libro.

#### Estados de tarea – diagrama conceptual de Amos

El Capítulo 7 formaliza lo que la guía del TP2 da por sentado al hablar de “bloquearse en una cola”:

| Estado | Definición según Amos | Ejemplo TP2 |
|--------|----------------------|-------------|
| **Running** | Única tarea en contexto; corre hasta bloquearse o ser preemptada | `task_btn` ejecutando statechart |
| **Ready** | Esperando que el scheduler le dé CPU | Ambas prio 1 compitiendo tras un tick |
| **Blocked** | Esperando evento o timeout; **no consume CPU** | `vTaskDelay(50)`; futuro `xQueueReceive(..., portMAX_DELAY)` |
| **Suspended** | Ignorada por scheduler hasta `vTaskResume()` | No usado en TP2 base |

Extracto clave sobre **Blocked**:

> *A Blocked task is a task that is waiting for something. [...] While a task is in the Blocked state, it doesn't consume any processor time.*

> *This is a very important feature of an RTOS: each blocking call is time-bound. That is, a task will only block while waiting for an event for as long as the programmer specifies it can be blocked.*

**Traducción:** una tarea bloqueada **no gasta CPU**. Cada llamada bloqueante tiene **límite de tiempo** definido por el programador (`timeout`). Eso diferencia firmware RTOS de programación de propósito general donde un `wait` puede ser indefinido sin modelo formal.

**Aplicación TP2:** cuando migres a cola en TP2-02, `task_led` bloqueada en `xQueueReceive` dejará de “consultar el flag cada 50 ms” y pasará a dormir hasta evento — exactamente el patrón que Amos describe como eficiente frente al polling.

#### Polling vs bloqueo – advertencia del Cap. 7

Amos muestra polling de ADC como anti-patrón:

> *While this code will detect when a new ADC reading has occurred, it will also cause the task to continually be in the Running state. If this happens to be the highest priority task in the system, this will starve the other tasks of CPU time.*

**Traducción:** un `while(!condicion)` mantiene la tarea en **Running** y puede **starvear** al resto.

Solución que propone (directamente aplicable a TP2-04):

> *An ISR can be interfaced with RTOS primitives to notify a task when there is valuable work to be done, thereby eliminating the need for CPU-intensive polling.*

En TP2-01, `task_btn` hace **polling del GPIO cada 50 ms** (`HAL_GPIO_ReadPin` + `vTaskDelay`). Funciona para el demo, pero Amos clasificaría eso como “polling periódico tolerable”, no como ISR-driven. TP2-04 mueve el **evento de flanco** a EXTI + semáforo y deja el debounce en tarea.

#### Scheduling preemptivo – recordatorio del Cap. 2 citado en Cap. 7

Amos distingue **round-robin** (rebanadas iguales entre tareas de igual prioridad) de **preemptive scheduling** (la de mayor prioridad lista corre primero). Extracto:

> *A preemptive scheduling algorithm will give priority to the most important task, regardless of what else in the system is happening – except for interrupts, since they occur underneath the scheduler and always have a higher priority.*

**Traducción:** el scheduler preemptivo prioriza la tarea más importante, **excepto las interrupciones**, que están “debajo” del scheduler y tienen prioridad hardware superior.

En tu TP2, `task_btn` y `task_led` comparten prioridad 1 → se reparten tiempo (round-robin si está habilitado time-slicing). Si en un experimento subís prioridad de `task_btn`, Amos advierte el escenario de **task starvation**: una tarea de alta prioridad que nunca se bloquea impide que las demás avancen.

---

### 11.2 Capítulo 3 – Colas y semáforos (fundamentos previos al scheduler)

Amos presenta las primitivas **antes** de la API detallada. Estos conceptos responden directamente las preguntas teóricas de TP2-02 y TP2-03.

#### Colas – definición

> *At its heart, a queue is simply a circular buffer. However, this buffer contains some very special properties, such as native multi-thread safety, the flexibility for each queue to hold any type of data, and waking up other tasks that are waiting on an item to appear in the queue. By default, data is stored in queues using First In First Out (FIFO) ordering.*

**Traducción:** una cola es un **buffer circular** con seguridad multi-hilo, tipos de dato flexibles y capacidad de **despertar tareas** en espera. Orden por defecto: **FIFO**.

#### Cuatro comportamientos esenciales (Cap. 3)

| Situación | Comportamiento (Amos) | API TP2 |
|-----------|----------------------|---------|
| Send con espacio libre | Inmediato; la tarea sigue corriendo | `xQueueSendToBack(..., 0)` → `pdPASS` |
| Receive con datos | Inmediato; recibe el item más antiguo | `xQueueReceive` |
| Send con cola llena | Bloquea hasta timeout o espacio | `xQueueSend(..., portMAX_DELAY)` |
| Receive con cola vacía | Bloquea hasta timeout o dato | `xQueueReceive(..., portMAX_DELAY)` |

Sobre cola llena:

> *When a queue is full, no information is thrown away. Instead, the task attempting to send the item to the queue will wait for up to a predetermined amount of time for available space in the queue.*

**Excepción TP2:** `xQueueOverwrite` (cola length = 1) sí reemplaza el único elemento; Amos lo cubre en ejemplos avanzados del Cap. 9.

#### Efecto de prioridades con colas (Cap. 3 – clave para TP2-02)

Amos describe el escenario clásico **productor alta prioridad / consumidor baja prioridad**:

1. Task2 intenta Receive de cola vacía → **Blocked**.
2. Task1 (mayor prioridad) Send hasta llenar la cola.
3. Task1 queda **Blocked** esperando espacio.
4. Task2 corre, Receive un item.
5. Task1 se despierta, Send uno, cola llena otra vez → ciclo.

> *Since a preemptive scheduler always runs the task with the highest priority, if that task always has data to write to the queue, the queue will fill before another task is given a chance to read from the queue.*

**Aplicación TP2:** si `task_btn` (productor) tuviera prioridad mayor que `task_led` y enviara ráfagas, la cola se llenaría antes de que el consumidor drene. En el demo ambas tienen **prioridad 1**, mitigando ese patrón. Documentá esto en `sotri-tp2_02-application.md` al responder la pregunta de la guía sobre prioridades.

#### Semáforos – origen y usos (Cap. 3)

> *The word semaphore has a Greek origin – the approximate English translation is sign-bearer, which is a wonderfully intuitive way to think about them. Semaphores are used to indicate that something has happened; they signal events.*

Casos que lista Amos (relevantes para TP2-04):

- ISR terminó de atender un periférico → **Give** para avisar a una tarea.
- Sincronizar tareas en un punto del algoritmo.
- Limitar usuarios simultáneos de un recurso (semáforo contador).

#### Semáforo binario vs contador (Cap. 3)

> *Binary semaphores are really just counting semaphores with a maximum count of 1. They are most often used for synchronization.*

> *Counting semaphores are most often used to manage a shared resource that has limitations on the number of simultaneous users.*

**Aplicación TP2:**

| Primitiva | Transporta `EV_LED_BLINK` / `EV_LED_OFF`? | Uso en TP2 |
|-----------|------------------------------------------|------------|
| Cola | **Sí** (copia `task_led_ev_t`) | TP2-02 |
| Semáforo binario | **No** (solo señal) | TP2-03, TP2-04 |
| Semáforo contador | No el tipo; cuenta N eventos | No requerido en demo |

Sobre binarios, Amos aclara:

> *Notice, however, that TaskB doesn't need to give back the binary semaphore. Instead, it simply waits for it again.*

Por eso en TP2-03 suele combinarse Give/Take con una variable de evento o lógica de toggle (como sugiere `semaphore_bin.txt` del ref).

---

### 11.3 Capítulo 8 – Protecting Data and Synchronizing Tasks

#### Sincronización eficiente con semáforo (vs polling)

Amos demuestra **TaskA** que hace `xSemaphoreGive` y **TaskB** que hace `xSemaphoreTake(..., portMAX_DELAY)`:

> *Blocking with semaphores is efficient as each task is only using 0.01% of the CPU time.*  
> *A task that is blocked because it is waiting on a semaphore won't run until it is available. This is true even if it is the highest-priority task in the system and no other tasks are currently READY.*

**Traducción:** bloquearse en semáforo es eficiente (~0,01 % CPU en su trace). Una tarea bloqueada en semáforo **no corre aunque sea la de mayor prioridad** hasta que haya Give.

#### “Wasting cycles” – polling con flag (Cap. 8)

El libro replica el mismo comportamiento observable con LEDs pero cambiando Give/Take por un `flag`:

```c
while(!flag);  /* BlueTaskB esperando indefinidamente */
```

Resultado medido con SystemView:

> *BlueTaskB is now using 100% of the CPU time while polling the value of flag.*  
> *Even though BlueTaskB is hogging the CPU, GreenTaskA still runs consistently since it has a higher priority.*

**Traducción:** polling al 100 % CPU; solo sobrevive la otra tarea si tiene **mayor prioridad**.

Paralelo con TP2-01: `put_event_task_led()` + flag evita el busy-wait puro porque `task_led` no hace `while(!flag)`, pero **sí hace polling periódico cada 50 ms** del flag en el statechart. Es mejor que `while(!flag)`, pero inferior a bloqueo en cola/semáforo.

Amos propone mitigación intermedia:

```c
while(!flag) { vTaskDelay(1); }  /* ~5% CPU pero peor latencia (≥ 1 tick) */
```

#### Semáforos acotados en tiempo (time-bound)

> *An RTOS does not guarantee the successful timeliness of an operation. It only promises that the call will be returned in an amount of time.*

**Traducción:** el RTOS **no garantiza éxito** de la operación; garantiza que la llamada **retorna** dentro del timeout.

```c
if (xSemaphoreTake(semPtr, 500/portTICK_PERIOD_MS) == pdPASS) { /* OK */ }
else { /* timeout – acción alternativa, p. ej. LED rojo de error */ }
```

En TP2 podrías usar timeout finito en Take para detectar fallos de comunicación (no exigido por la guía, pero buena práctica industrial).

#### Semáforo vs mutex (contexto; no es foco del TP2)

Amos advierte que usar semáforo binario para **proteger datos compartidos** puede causar **priority inversion**. Los **mutex** con priority inheritance corrigen eso. En TP2:

- **Cola** → comunicación thread-safe sin mutex explícito (el kernel copia datos).
- **Variable + flag** → riesgo de carrera si hay múltiples escritores.
- **Semáforo binario** → sincronización (“pasó algo”), no sustituto de mutex para structs complejas.

---

### 11.4 Capítulo 9 – Intertask Communication

Este capítulo responde con profundidad las preguntas de **TP2-02** sobre envío/recepción y el efecto de prioridades.

#### Pass-by-value – patrón productor/consumidor

Amos usa una cola de comandos LED análoga a tu `task_led_ev_t`:

```c
ledCmdQueue = xQueueCreate(2, sizeof(uint8_t));

/* Consumidor */
if (xQueueReceive(ledCmdQueue, &nextCmd, portMAX_DELAY) == pdTRUE) {
    switch(nextCmd) { /* ... */ }
}

/* Productor */
xQueueSend(ledCmdQueue, &ledCmd, portMAX_DELAY);
```

Puntos que enfatiza:

> *xQueueReceive will copy the next [...] value stored in the queue into nextCmd.*  
> *Each time xQueueSend is called, the contents [...] is copied into the queue before moving on. As soon as xQueueSend() returns successfully, the value [...] does not need to be preserved.*

**Traducción:** la cola **copia por valor**. Tras un Send exitoso, la variable local del productor puede modificarse sin afectar lo ya encolado.

**Aplicación TP2-02:**

```c
task_led_ev_t ev = EV_LED_BLINK;
xQueueSendToBack(h_btn_led_q, &ev, pdMS_TO_TICKS(0));
/* ev puede reutilizarse; la cola tiene su propia copia */
```

#### Tipos compuestos – tu caso directo

El ejemplo `LedStates_t` del Cap. 9 es equivalente a pasar un struct por cola:

```c
ledCmdQueue = xQueueCreate(8, sizeof(LedStates_t));
```

En TP2:

```c
h_btn_led_q = xQueueCreate(5, sizeof(task_led_ev_t));
```

Amos advierte:

> *The use of void* for interacting with queues acts as a double-edged sword. It provides the ultimate amount of flexibility, but also provides the very real possibility for you to pass the wrong data type into the queue, potentially without a warning from the compiler.*

Debés mantener coherencia: `sizeof(task_led_ev_t)` al crear y mismos tipos al Send/Receive.

#### Prioridades + cola llena – experimento del Cap. 9

Amos asigna **máxima prioridad** al productor y **baja** al consumidor a propósito:

```c
xTaskCreate(recvTask,  ..., tskIDLE_PRIORITY + 1, NULL);
xTaskCreate(sendingTask, ..., configMAX_PRIORITIES - 1, NULL);
```

> *This prioritization setup allows sendingTask to repeatedly send data to the queue until it is full (because sendingTask has a higher priority). After the queue has filled, sendingTask will block and allow recvTask to remove an item from the queue.*

**Traducción:** el productor llena la cola; al quedar lleno, **se bloquea**; recién entonces el consumidor drena.

**Lección TP2:** diseñar **prioridades + tamaño de cola + timeout** juntos. Una cola de 5 eventos como la de tu `app.c` amortigua ráfagas de pulsaciones; con overwrite (length 1) se conserva solo el último estado.

#### Receive bloqueante vs polling en loop periódico

Amos usa `portMAX_DELAY` en el consumidor LED. En TP2 podés elegir:

| Estrategia | Ventaja | Desventaja |
|------------|---------|------------|
| `xQueueReceive(..., 0)` dentro del loop 50 ms | Cambio mínimo respecto a flag | Sigue despertando cada 50 ms |
| `xQueueReceive(..., portMAX_DELAY)` | Mínimo CPU; reacción inmediata al evento | Hay que reestructurar loop de `task_led` |
| Cola + `vTaskDelayUntil` para blink | Separa “esperar evento” de “toggle 500 ms” | Más complejidad (dos loops o statechart) |

---

### 11.5 Capítulo 10 – Drivers and ISRs

Base teórica de **TP2-04** (botón por interrupción + semáforo hacia `task_btn`).

#### Tareas vs ISRs – similitudes y diferencias (traducción selectiva)

**Similitudes (Amos):**

> *Both provide a way of achieving parallel code execution. Both only run when required.*

**Diferencias críticas:**

| Aspecto | Tarea | ISR |
|---------|-------|-----|
| Quién la activa | Scheduler (kernel) | Hardware (NVIC) |
| Duración | Puede ser larga; usa primitivas RTOS | **Debe ser mínima** |
| Parámetros | `void *argument` | **Ninguno** (lee registros HW) |
| API FreeRTOS | Completa (con bloqueo) | Solo **`...FromISR`** |
| Pila | Pila privada por tarea | **Pila de sistema compartida** por ISRs |

> *ISRs must exit as quickly as possible; tasks are more forgiving.*

> *Calling a non-ISR API function from inside an ISR will cause FreeRTOS to trigger configASSERT.*

#### API FromISR – reglas del libro

> *The FromISR variants won't block.*  
> *The FromISR variants require an extra parameter, BaseType_t *pxHigherPriorityTaskWoken.*  
> *Only interrupts that have a logically lower priority than what is defined by configMAX_SYSCALL_INTERRUPT_PRIORITY [...] are permitted to call FreeRTOS API functions.*

Patrón UART del libro (aplicable a EXTI del botón en TP2-04):

```c
void USART2_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* ... leer hardware, preparar dato ... */
    xQueueSendFromISR(uart2_BytesReceived, &tempVal, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

Equivalente TP2-04 con semáforo (según guía):

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == B1_Pin) {
        xSemaphoreGiveFromISR(h_btn_led_bin_sem, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

Amos explica `xHigherPriorityTaskWoken`:

> *This variable is passed to the xQueueSendFromISR function and is used to determine whether a high-priority task [...] is blocking because it is waiting on an empty queue. In this case, xHigherPriorityTaskWoken will be set to true, indicating a higher-priority task should be woken immediately after the ISR exits.*

#### Modelo de anidamiento / prioridades NVIC

El Cap. 10 diagrama tres bandas de prioridad en Cortex-M:

1. **0–4 (numéricamente altas):** ISR “bare-metal pura” — **sin** API FreeRTOS.
2. **5 – `configMAX_SYSCALL_INTERRUPT_PRIORITY`:** ISR que **pueden** llamar `FromISR`.
3. **Prioridad kernel (SysTick, PendSV):** infraestructura RTOS.

> *If an interrupt with a priority that is logically higher than configMAX_SYSCALL_INTERRUPT_PRIORITY calls a FreeRTOS API function, you'll be greeted with a configASSERT failure.*

**Aplicación TP2:** verificar en CubeMX que `EXTI15_10_IRQn` del botón tenga prioridad **compatible** (en STM32, número **mayor** = prioridad lógica **menor**; la IRQ del botón debe ser **menos urgente** que el umbral syscall del kernel). Si `HAL_GPIO_EXTI_Callback` dispara assert, lo primero a revisar es prioridad NVIC.

#### Driver por cola desde ISR – delegación (Cap. 10)

Flujo que Amos mide con SystemView:

| Enfoque | Uso CPU (ejemplo UART) |
|---------|------------------------|
| Polled driver en tarea | ~96 % |
| ISR + `xQueueSendFromISR` + tarea bloqueada en Receive | ~1,6 % |

> *The ISR responsible for dealing with the incoming data [...] is only consuming around 1.6% of the CPU (much better than the 96% we saw when we were using a polled approach).*

**Paralelo TP2:** TP2-01 lee botón por polling cada 50 ms (aceptable para el curso). TP2-04 mueve la **detección de flanco** a ISR y deja debounce/statechart en `task_btn` — mismo principio de **ISR mínima + tarea con trabajo pesado**.

Comparación baremetal: en baremetal la ISR suele setear `volatile flag` y el `main` o una función polled reacciona. Con RTOS, la ISR hace `GiveFromISR` y la tarea bloqueada en `Take` **no consume CPU** mientras espera — ventaja directa del modelo del Cap. 8 y 10.

---

### 11.6 Síntesis cruzada: guía TP2 + Amos

| Pregunta guía TP2 | Dónde responde Amos | Idea central |
|-------------------|---------------------|--------------|
| ¿Cómo crear/gestionar cola? | Cap. 3, 9 | FIFO, copia por valor, thread-safe |
| ¿Qué es bloquearse en cola? | Cap. 3, 7 | Estado Blocked, sin consumo de CPU |
| ¿Efecto de prioridades? | Cap. 3, 9 | Productor rápido llena cola; bloqueo en cadena |
| ¿Semáforo binario vs contador? | Cap. 3, 8 | Señal vs conteo de recursos |
| ¿API en ISR? | Cap. 10 | Solo FromISR + `portYIELD_FROM_ISR` |
| ¿Delegar ISR a tarea? | Cap. 7, 10 | ISR corta; procesamiento en tarea vía cola/sem |
| ¿Anidamiento IRQ? | Cap. 10 | `configMAX_SYSCALL_INTERRUPT_PRIORITY` |

**Hilo conductor del libro alineado a tu proyecto:**

```
TP2-01  memoria compartida     →  polling periódico (Cap. 7 advierte limitaciones)
TP2-02  cola pass-by-value     →  Cap. 9 (LED command queue ≈ task_led_ev_t)
TP2-03  semáforo Give/Take     →  Cap. 8 (eficiente vs flag polling)
TP2-04  ISR + semáforo         →  Cap. 10 (FromISR, prioridades NVIC)
```

---

### 11.7 Referencias bibliográficas (Amos)

- Brian Amos, *Hands-On RTOS with Microcontrollers*, Packt, 2020, ISBN 978-1-83882-673-4.
- Cap. 3 – Task Signaling and Communication Mechanisms (pp. 50–64).
- Cap. 7 – The FreeRTOS Scheduler (pp. 147–170).
- Cap. 8 – Protecting Data and Synchronizing Tasks (pp. 171–201).
- Cap. 9 – Intertask Communication (pp. 202–223).
- Cap. 10 – Drivers and ISRs (pp. 226–276).
- Código de ejemplo: [PacktPublishing/Hands-On-RTOS-with-Microcontrollers](https://github.com/PacktPublishing/Hands-On-RTOS-with-Microcontrollers) (`Chapter_7`, `Chapter_8`, `Chapter_9`, `Chapter_10`).

---

## Apéndice A – Ejemplo integrado de timeline (cola, prioridades iguales)

**Condiciones:** `task_btn` y `task_led`, prioridad 1. Cola vacía. `task_led` usa `xQueueReceive(..., 0)` cada 50 ms.

```
t=0 ms    task_btn corre, vTaskDelay(50) → BLOCKED
t=0 ms    task_led corre, Receive falla (vacía), vTaskDelayUntil(50) → BLOCKED
t=50 ms   task_btn corre, detecta PRESSED, Send BLINK → pdPASS
t=50 ms   task_btn vTaskDelay(50) → BLOCKED
t=50 ms   task_led despierta (tick), Receive OK → event=BLINK, statechart → ST_LED_BLINK
t=100 ms  task_led toggle si corresponde...
```

Si `Receive` fuera con `portMAX_DELAY`, `task_led` permanecería BLOCKED hasta el primer Send, ahorrando ciclos de polling.

---

## Apéndice B – Errores frecuentes en el TP2

1. **Usar `xQueueSend` en ISR** → usar `xQueueSendToBackFromISR`.
2. **Olvidar `portYIELD_FROM_ISR`** → la tarea despertada espera hasta el próximo tick.
3. **Semáforo binario para contar pulsaciones rápidas** → se pierden eventos; usar cola o semáforo contador.
4. **Cola llena con Send bloqueante desde alta prioridad** → posible bloqueo en cadena; evaluar cola más grande o overwrite.
5. **Mezclar `put_event_task_led` con cola** → doble mecanismo; eliminar el viejo al migrar.
6. **Prioridad IRQ EXTI demasiado alta** → FromISR no permitido en esa IRQ (Cortex-M + FreeRTOS).

---

*Documento para estudio del TP2 – Comunicación de Tareas de FreeRTOS. Complementa la guía oficial del curso y el libro de Brian Amos (Cap. 3, 7–10). Las entregas deben basarse en la experiencia de depuración del proyecto STM32.*
