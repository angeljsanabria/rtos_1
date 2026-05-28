# CESE - Sistemas Operativos de Tiempo Real

## Trabajo Practico N°: 2 - Comunicacion de Tareas de FreeRTOS

### Paso 01
Fue completado. Revisar el repo.

// TODO: ANGEL; COMPLETAR 
### Paso 02: Colas de FreeRTOS (respuestas en base a la depuracion del proyecto STM32)

> Proyecto: `sotri-tp2_02_26Co2026-07`  
> Completar cada respuesta segun la experiencia depurando en STM32CubeIDE.

**1. ¿Como crear una Cola?**

---

**2. ¿Como eliminar una Cola?**

---

**3. ¿Como gestiona una Cola los datos que contiene?**

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
Es que task_btn se comunica con un archivo de "interfaz" para impactar en led seteando evento y flag con void put_event_task_led(task_led_ev_t event); 

Utilizamos la queue h_btn_led_q para comunicar las tareas.
Con xQueueSend(h_btn_led_q, &event, portMAX_DELAY) publicamos los eventos de boton presionado y release.
Del lado de la tarea del led con if(xQueueReceive(h_btn_led_q, &task_led_dta.event, 10) == pdTRUE)
leemos si tenemos un evento en la queue para modificar el event del handler del led y aplicamos el flag en True para que el cambio sea tomado por la FSM.

