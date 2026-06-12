# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 3 - Sincronizacion de Tareas de FreeRTOS (Actividad 04)

> **Proyecto:** `sotri-tp3_04_26Co2026-07_ang`  
> **Aplicacion:** `sotri-tp3_04-application: Security airlock` (puerta esclusa de seguridad)  
> **Placa:** NUCLEO-L4R5ZI (STM32L4R5ZI). La guia original referencia NUCLEO-F103RB; la arquitectura APP es equivalente.

### Nota sobre nombres de archivos (guia vs proyecto)

La guia TP3 Actividad 04, Paso 03, cita `task_entry_a.c` y `task_exit_a.c` (heredado de la Actividad 03 – *Vehicular crossing*).  
En **este** proyecto (Actividad 04 – *Security airlock*) las tareas operativas son:

| Guia (Paso 03, texto generico) | Archivo real en TP3-04 |
|--------------------------------|-------------------------|
| `task_entry_a.c` / `task_exit_a.c` | `task_gate_a.c` … `task_gate_d.c` |

Cada `task_gate_*` modela un punto de ingreso/egreso de la esclusa. El analisis de `task_gate_a.c` aplica a B, C y D (misma estructura en el esqueleto entregado).

---

## Paso 01

Proyecto generado e importado. Compila en STM32CubeIDE.

---

## Paso 02

Archivo de entrega: `sotri-tp3_04-application.md` (este documento).

---

## Paso 03: Analisis del codigo fuente base

### Contexto del problema (Security airlock)

Segun la guia TP3 Actividad 04, Paso 06, el sistema a implementar es una **puerta esclusa**:

- Varias personas pueden solicitar paso por **cuatro puntos** (puertas A, B, C, D).
- Solo **una puerta abierta a la vez**; normalmente todas **cerradas**.
- Estimulos de prueba: `OPEN_REQUEST_*` (pedido de apertura) y `DOOR_CLOSED_*` (puerta cerrada).
- Sincronizacion con **semaforos binarios** y **mutex** para recursos compartidos.

El codigo analizado aqui corresponde al **esqueleto inicial** (antes del Paso 06): crea tareas, hooks y un simulador `task_test`, pero **aun no** implementa mutex ni logica de esclusa.

---

### 1. `app.c` – Orquestacion e inicializacion

**Rol:** punto central de la capa de aplicacion. Se invoca desde `main.c` **antes** de `osKernelStart()`.

**Flujo de `app_init()`:**

1. **Inicializa contadores globales** (`g_app_cnt`, `g_app_task_cnt`, `g_app_tick_cnt`, etc.) en cero.
2. **Log de identificacion** via `LOGGER_INFO`: nombre de la aplicacion (*Security airlock*) y tick actual.
3. **Reserva de primitivas RTOS:** bloques comentados para colas, semaforos y mutex. En el esqueleto **no se crean aun** (`xSemaphoreCreateBinary`, `xSemaphoreCreateMutex`, etc. pendientes para Paso 06).
4. **Creacion de cinco tareas** con `xTaskCreate` y `configASSERT(pdPASS == ret)`:

| Tarea | Funcion | Prioridad | Rol conceptual |
|-------|---------|-----------|----------------|
| Task Gate A | `task_gate_a` | 3 | Puerta A – ingreso/egreso |
| Task Gate B | `task_gate_b` | 2 | Puerta B |
| Task Gate C | `task_gate_c` | 3 | Puerta C |
| Task Gate D | `task_gate_d` | 2 | Puerta D |
| Task Test | `task_test` | 1 | Simulador de eventos |

Las puertas A/C tienen prioridad **3**; B/D prioridad **2**; el test arranca en **1** (la mas baja).

5. **Consulta heap libre** con `xPortGetFreeHeapSize()` (diagnostico).
6. **`app_it_init()`** – inicializacion de interrupciones de aplicacion.
7. **`cycle_counter_init()`** – contador DWT para medicion de tiempos.

**Observacion:** `app.h` aun declara handles de la Actividad 03 (`h_entry_a_bin_sem`, `task_entry_a`, etc.). Conviene alinear header y `app.c` al renombrar primitivas en el Paso 06.

---

### 2. `app_it.c` – Interrupciones de aplicacion

**Rol:** capa entre el hardware (boton EXTI) y las tareas.

- **`app_it_init()`:** plantilla con `CPSID i` / `CPSIE i` para una seccion critica corta. Hoy no configura NVIC ni EXTI adicional.
- **`HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)`:** callback HAL ante flanco en GPIO. Detecta `BTN_A_PIN` pero el cuerpo esta **vacio** (TODO Paso 06: delegar a tarea con `xSemaphoreGiveFromISR` + `portYIELD_FROM_ISR`).

En la esclusa final, el boton fisico puede complementar a `task_test` como fuente de `OPEN_REQUEST_*`.

---

### 3. `task_gate_a.c` (y B, C, D) – Logica de cada puerta

**Nota:** sustituye a `task_entry_a` / `task_exit_a` de la Actividad 03.

**Estado actual (esqueleto):**

```text
task_gate_a()
  ├─ Inicializa g_task_gate_a_cnt
  ├─ LOGGER_INFO: arranque con nombre de tarea y tick
  └─ for (;;)
       ├─ g_task_gate_a_cnt++
       ├─ log "Wait 2500mS"
       └─ vTaskDelay(2500 ms)
```

- **No** espera semaforos ni mutex.
- **No** controla LED/puerta (Open/Close).
- Solo demuestra que la tarea existe, corre y cede CPU cada 2,5 s.

**Comportamiento esperado tras Paso 06 (guia):**

- Bloqueo en semaforo binario al recibir `OPEN_REQUEST_A` (desde `task_test` o ISR).
- Mutex compartido entre puertas que compiten por “abrir una sola a la vez”.
- Accion Open/Close sobre el actuador (LED en la placa).
- Señal `DOOR_CLOSED_A` cuando la puerta vuelve a cerrarse.

Los archivos `task_gate_b.c`, `task_gate_c.c` y `task_gate_d.c` son **identicos en estructura**, cambiando nombre de tarea, contador y mensajes de log.

---

### 4. `task_test.c` – Simulador automatizado de eventos

**Rol:** tarea “generador de escenarios” que inyecta estimulos a las puertas sin depender del boton.

**Enum de eventos:**

```c
typedef enum e_task_test {
    Error,
    OPEN_REQUEST_A, DOOR_CLOSED_A,
    OPEN_REQUEST_B, DOOR_CLOSED_B,
    OPEN_REQUEST_C, DOOR_CLOSED_C,
    OPEN_REQUEST_D, DOOR_CLOSED_D
} e_task_test_t;
```

**Escenarios (`E_TASK_TEST_X`):**

| Valor | Secuencia del array |
|-------|---------------------|
| 0 | Error + valores invalidos (prueba de default) |
| 1 | A: OPEN → CLOSED → OPEN → CLOSED |
| 2 | A completo, luego B |
| 3 | B, luego C |
| 4 | C, luego D |
| 5 | D, luego A (ciclo) |

Con `#define E_TASK_TEST_X (1)` se usa el escenario **solo puerta A**.

**Secuencia de arranque:**

1. `last_wake_time = xTaskGetTickCount()` para `vTaskDelayUntil`.
2. Sube su prioridad con `vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 2ul)` → pasa a prioridad **3**, igual que Gate A/C, para ejecutar el banco de pruebas sin ser preemptada por las puertas de prioridad 2.
3. Bucle externo infinito; bucle interno recorre `e_task_test_array[]`.

**Switch de eventos:** los `case OPEN_REQUEST_*` y `DOOR_CLOSED_*` tienen cuerpo **vacio** (solo `break`). No hay aun `xSemaphoreGive` hacia `task_gate_*`. El `case Error` loguea mensaje de error.

**Temporizacion:** tras cada evento, `vTaskDelayUntil(..., 5000 ms)` → un estimulo cada 5 s, periodico.

**Contraste con Actividad 03:** alli `task_test` enviaba `xSemaphoreGive(h_entry_a_bin_sem)` etc. Aqui los estimulos tienen **dos fases por puerta** (apertura solicitada + puerta cerrada).

---

### 5. `freertos.c` – Hooks del kernel

Implementa callbacks opcionales habilitados en `FreeRTOSConfig.h`:

| Hook | Funcion | Comportamiento |
|------|---------|----------------|
| `vApplicationIdleHook` | Tarea Idle | Incrementa `g_task_idle_cnt` cada vez que no hay trabajo Ready de mayor prioridad. |
| `vApplicationTickHook` | Cada tick (1 ms) | Incrementa `g_app_tick_cnt`. Ejecuta en contexto ISR: debe ser muy breve. |
| `vApplicationStackOverflowHook` | Desbordamiento de pila | `taskENTER_CRITICAL()`, `configASSERT(0)` para detener en depuracion, incrementa `g_app_stack_overflow_cnt`. |

Estos hooks **no participan** en la logica de la esclusa; sirven para diagnostico y estadisticas del curso.

---

### Diagrama de arquitectura (estado actual – esqueleto)

```text
main()
  └─ app_init()
       ├─ xTaskCreate(task_gate_a..d, task_test)
       ├─ app_it_init()
       └─ cycle_counter_init()
  └─ osKernelStart()
       │
       ├─ task_gate_a..d  ──► vTaskDelay(2500 ms)  [sin sync aun]
       │
       ├─ task_test       ──► recorre array eventos ──► vTaskDelayUntil(5000 ms)
       │                      [switch vacio, sin Give/Take]
       │
       ├─ Idle Hook       ──► g_task_idle_cnt++
       └─ Tick Hook       ──► g_app_tick_cnt++
```

---

### Comportamiento esperado al depurar (Paso 04 – esqueleto)

1. Logs de `app_init` con titulo *Security airlock*.
2. Arranque secuencial de **Task Gate A/B/C/D** y **Task Test** (orden depende del scheduler; `task_test` eleva prioridad y loguea primero su escenario).
3. **Task Test:** cada 5 s imprime indice del array y `Wait 5000mS`; con `E_TASK_TEST_X==1` alterna indices 0..3 (OPEN/CLOSED de A).
4. **Task Gate A..D:** cada ~2,5 s imprimen `Wait 2500mS` de forma independiente.
5. **Sin** sincronizacion cruzada visible hasta completar Paso 06.

---

## Paso 04: Confirmacion mediante depuracion

// TODO: ANGEL – Completar tras depurar en semihosting:

- [ ] Verificar secuencia de logs descrita en Paso 03.
- [ ] Confirmar que `g_task_test_cnt` incrementa cada evento del array.
- [ ] Confirmar que `g_task_gate_a_cnt` … `g_task_gate_d_cnt` incrementan con el delay de 2,5 s.
- [ ] Anotar discrepancias respecto al analisis (prioridades, orden de arranque, heap libre).

---

## Paso 06 (referencia guia): implementacion Security airlock

Objetivo: mutex + semaforos para que **solo una puerta este abierta**, con estimulos `OPEN_REQUEST_*` / `DOOR_CLOSED_*`.

Pasos recomendados por la guia:

1. **`task_test` + `task_gate_a`:** sincronizar pedido de apertura y puerta cerrada (semaforos binarios).
2. Repetir para `task_gate_b`, `task_gate_c`, `task_gate_d`.
3. **Mutex** entre tareas que comparten recurso “esclusa” (solo una abierta).
4. **Control Open/Close** (LED / logica de puerta).
5. Probar escenarios cambiando `E_TASK_TEST_X` (0..5).

// TODO: ANGEL – Asentar comportamiento observado tras implementar Paso 06.

---

## Referencias

- Guia: `practicos/SOTR_I - Guia TP3 - Sincronizacion de Tareas de FreeRTOS.pdf` – Actividad 04.
- Problema: *Security airlock* (Other problems, TP3).
- Actividad 03 previa: *Vehicular crossing* (`task_entry_*` / `task_exit_*`) – patron similar con cruce de capacidad `G_TASKS_CNT_MAX`.
