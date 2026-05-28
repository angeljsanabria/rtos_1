# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 3 - Comunicacion de Tareas de FreeRTOS

### Paso 01
Fue completado. Revisar el repo.

### Paso 02: Semaforos binarios y contadores (respuestas en base a la depuracion del proyecto STM32)

> Proyecto: `sotri-tp2_03_26Co2026-07`  
> Completar cada respuesta segun la experiencia depurando en STM32CubeIDE.

// TODO ANGEL; COMPLETAR
**1. ¿Como crear y usar semaforos binarios y semaforos contadores?**

---

**2. ¿Cuales son las diferencias entre semaforos binarios y semaforos contadores?**

---

### Paso 03: Comunicacion `task_btn` ↔ `task_led` mediante Semaforo binario

El reemplazo de la queue por semaforo binario, en este caso que es para comunicar solo dos tareas, no tiene mucho impacto. Solo se pasa el aviso de boton, pero no se tiene la mejora de pasar cual fue el evento (si presionado o release). Tambien se pierde la ventaja de poder encolar hasta 5 avisos de boton con el evento hasta ser atendido. En el caso del semaforo binario, solo puede avisar de un evento hasta que la tarea del led pueda hacer el take de la señal de semaforo.




