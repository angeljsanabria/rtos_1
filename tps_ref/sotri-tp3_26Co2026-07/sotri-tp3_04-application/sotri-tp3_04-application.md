## Paso 03: Consultar a Gemini Pro

El código fuente de este Trabajo Práctico establece una arquitectura basada en **FreeRTOS** diseñada para controlar y simular un sistema de múltiples compuertas o puertas (Gates). Mantiene el paradigma orientado a estímulos o eventos mediante interrupciones de hardware y, al mismo tiempo, incorpora una máquina de pruebas (Test Task) para inyectar secuencias lógicas de funcionamiento.

A continuación, se detalla el funcionamiento de cada archivo:

### 1. `app.c` (Configuración e Inicialización Principal)
Es el archivo orquestador encargado de inicializar el sistema operativo en tiempo real y crear los diferentes hilos (threads).
* **Creación de múltiples tareas:** A través de la API `xTaskCreate`, el sistema instancia un total de cinco tareas:
    * **Task Gate A**, **Task Gate B**, **Task Gate C**, **Task Gate D**: Representan a cuatro compuertas individuales en el sistema.
    * **Task Test**: Tarea destinada a probar el funcionamiento del sistema.
* **Parámetros:** Todas las tareas arrancan inicialmente con la misma prioridad base (`tskIDLE_PRIORITY + 1ul`) y el tamaño de pila mínimo (`configMINIMAL_STACK_SIZE`).

---

### 2. `app_it.c` (Manejo de Interrupciones)
Gestiona las interrupciones externas provenientes del hardware (botones).
* **Callback de EXTI (Interrupción Externa):** Contiene la función `HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)` que es disparada automáticamente por la capa de abstracción de hardware (HAL) de STM32 cuando ocurre un cambio de flanco en un pin.
* **Captura de eventos físicos:** En su interior, posee un bloque de código para detectar si el pin presionado corresponde a un pulsador (`if (GPIO_Pin == BTN_...)`). Esto se utiliza para generar estímulos físicos reales al sistema desde el exterior.

---

### 3. `freertos.c` (Retrollamadas o Hooks del Sistema)
Proporciona rutinas de diagnóstico nativas de FreeRTOS.
* **Monitoreo Continuo:** Implementa los *hooks* del sistema operativo:
    * `vApplicationIdleHook`: Monitorea el tiempo inactivo e incrementa `g_task_idle_cnt`.
    * `vApplicationTickHook`: Cuenta los "latidos" del sistema en `g_app_tick_cnt`.
    * `vApplicationStackOverflowHook`: Medida de seguridad vital que detiene el sistema (`configASSERT(0)`) si la memoria RAM asignada a alguna de las tareas (puertas o test) se desborda, evitando así funcionamientos erráticos.

---

### 4. `task_gate_a.c`, `task_gate_b.c`, `task_gate_c.c`, `task_gate_d.c` (Lógica de las Compuertas)
Cada uno de estos archivos modela el comportamiento independiente de una compuerta. Tienen una estructura arquitectónica idéntica entre sí:
* **Inicialización:** Todas declaran e inicializan un contador de ciclos individual (`g_task_gate_a_cnt`, `g_task_gate_b_cnt`, etc.). Al iniciar, emiten un mensaje por consola (`LOGGER_INFO`) con su nombre y el número de Tick actual para registrar su arranque.
* **Bucle Infinito:** Poseen demoras máximas definidas (por ejemplo, `TASK_GATE_A_DEL_MAX` fijado en 2500 milisegundos), que marcan los tiempos en los que se quedarán a la espera de transiciones de estados ante aperturas o cierres de compuertas.

---

### 5. `task_test.c` (Simulador o Máquina de Pruebas Automatizadas)
Es el núcleo de validación lógica del programa. Su objetivo es inyectar solicitudes programadas para observar cómo reaccionan y sincronizan las compuertas.
* **Elevación de Prioridad:** Su primera instrucción importante es usar `vTaskPrioritySet` para subir su propia jerarquía a 2 puntos por encima del resto (`uxTaskPriorityGet(NULL) + 2ul`). Esto asegura que el test se adueñe de la CPU inmediatamente y despache sus estímulos sin ser interrumpido.
* **Matriz de Eventos Complejos:** Recorre un vector de pruebas secuenciales (`e_task_test_array`) mediante un bucle infinito y un bloque `switch-case`.
* **Escenarios simulados:** Los eventos que inyecta están vinculados a las puertas. Algunos de los eventos listados son `OPEN_REQUEST_A` (solicitud de apertura de la Puerta A), `DOOR_CLOSED_A` (señal de puerta A cerrada), y de forma análoga para la Puerta C (`OPEN_REQUEST_C`, `DOOR_CLOSED_C`). Esto permite depurar secuencias cruzadas para verificar el enclavamiento y la correcta sincronización entre diferentes compuertas.
