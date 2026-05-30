# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 3 - Comunicacion de Tareas de FreeRTOS

### Paso 01
Fue completado. Revisar el repo.

### Paso 02: Semaforos binarios y contadores (respuestas en base a la depuracion del proyecto STM32)

> Proyecto: `sotri-tp2_03_26Co2026-07`  
> Completar cada respuesta segun la experiencia depurando en STM32CubeIDE.

**1. ¿Como crear y usar semaforos binarios y semaforos contadores?**

Antes de crear el semaforo se necesita una variable del tipo SemaphoreHandle_t.

**Semaforo binario**

Creacion:
```c
SemaphoreHandle_t h_sem = xSemaphoreCreateBinary();
configASSERT(NULL != h_sem);
```
Se crea vacio (no se puede hacer Take hasta un Give previo).

**Semaforo contador**

Creacion:
```c
SemaphoreHandle_t h_sem_cnt = xSemaphoreCreateCounting(max_count, initial_count);
configASSERT(NULL != h_sem_cnt);
```
max_count → valor maximo del contador.
initial_count → valor inicial (ej. cantidad de recursos libres al arrancar).

**Uso desde una task**

Las operaciones basicas son Give (señalizar) y Take (esperar/tomar):

```c
xSemaphoreGive(h_sem);
xSemaphoreTake(h_sem, xTicksToWait);
```

Parametro xTicksToWait: igual que en colas (0 = no bloqueante, portMAX_DELAY = espera indefinida, N ticks = espera acotada).

Retorno de Take:
- pdPASS → tomo el semaforo.
- pdFAIL → no estaba disponible y vencio el timeout (o timeout = 0).

Give retorna pdPASS si pudo señalizar; si hay una tarea bloqueada en Take, la desbloquea.

Patron tipico (Amos, Cap. 3 y 8): una tarea hace Give para avisar que ocurrio un evento; otra hace Take para esperar ese evento sin hacer polling. La tarea que recibe no necesita devolver el semaforo binario con Give; solo vuelve a esperar con otro Take.

Desde ISR no se usa Give/Take normal; se usan las variantes FromISR (TP2-04).

---

**2. ¿Cuales son las diferencias entre semaforos binarios y semaforos contadores?**

Segun Amos (Cap. 3), el semaforo binario es un semaforo contador con maximo 1. La diferencia esta en cuantos permisos puede guardar.

| Aspecto | Binario | Contador |
|---------|---------|----------|
| Valores posibles | 0 o 1 | 0 … max_count |
| Give sin nadie esperando | Queda en 1 (no acumula mas Gives) | Incrementa el contador hasta max_count |
| Uso tipico | Sincronizar tareas o señalizar un evento | Limitar cuantos accesos simultaneos hay a un recurso (ej. pool de buffers) |

En binario: si se hacen varios Give seguidos sin Take, solo cuenta uno. Sirve para decir "paso algo", no para contar cuantos eventos quedaron pendientes.

En contador: cada Give incrementa; cada Take decrementa. Puede haber N eventos o recursos pendientes a la vez (hasta max_count).

Conclusion: binario → sincronizacion (handshake, aviso ISR→tarea). Contador → recursos multiples o eventos acumulables.

---

### Paso 03: Comunicacion `task_btn` ↔ `task_led` mediante Semaforo binario

El reemplazo de la queue por semaforo binario, en este caso que es para comunicar solo dos tareas, no tiene mucho impacto. Solo se pasa el aviso de boton, pero no se tiene la mejora de pasar cual fue el evento (si presionado o release). Tambien se pierde la ventaja de poder encolar hasta 5 avisos de boton con el evento hasta ser atendido. En el caso del semaforo binario, solo puede avisar de un evento hasta que la tarea del led pueda hacer el take de la señal de semaforo.




