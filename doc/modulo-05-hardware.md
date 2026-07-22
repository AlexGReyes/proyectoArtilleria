# Módulo 5 — Hardware (abstracción de hardware + controlador simulado)

**Estado**: completado, compilando (`simuladorArtilleria1Editor`, Development, `Result: Succeeded`) y con **34/34 tests en verde** (suite SISA completa: 112/112).

## Propósito

Definir la costura `IHardwareController` **desde la Fase 1**, para que el salto a la Fase 2 (hardware real: Arduino, ESP32, PLC, STM32 sobre RS-232/RS-485/CAN/TCP/UDP/Serial/Ethernet) sea cambiar una implementación y no reescribir la lógica de simulación. En Fase 1 se entrega además el controlador simulado que permite escribir, ejecutar y probar Simulation contra el contrato real antes de que exista ningún dispositivo físico — incluidas las rutas de fallo.

## Componentes

### Domain (`Public/HardwareTypes.h`)

- `EHardwareControllerKind`: `Simulated`, `Arduino`, `Esp32`, `Plc`, `Stm32`.
- `EHardwareTransport`: `None` (simulado), `Serial`, `Rs232`, `Rs485`, `CanBus`, `TcpIp`, `Udp`, `Ethernet` — la lista completa que pide la Fase 2.
- `EHardwareConnectionState`: `Disconnected`, `Connecting`, `Connected`, `Error`.
- `FHardwareCapabilities`: qué sabe hacer realmente un controlador (control de puntería, encoders, IMU, GNSS, brújula, disparador). Se modela como booleanos explícitos, no como máscara de bits, para que el panel pueda enlazar cada capacidad y para que un dispositivo de Fase 2 que solo lea sensores sea expresable.
- `FHardwareConnectionConfig`: una sola struct cubre todos los transportes (la familia serie usa `PortName`/`BaudRate`, la de red `Host`/`NetworkPort`, y CAN/RS-485 añaden `DeviceAddress`), con `Validate()` que comprueba lo que cada familia exige.
- `FHardwareTelemetry`: una muestra completa — ángulos de encoder, movimiento, posición GNSS, rumbo de brújula, actitud IMU y contador de disparos. **Cada grupo lleva su bandera de validez**, para que nadie confunda "no instalado" con "lee cero".

### La costura (`Public/IHardwareController.h`)

Interfaz C++ pura (no `UINTERFACE`, por el mismo motivo que `IGeoCoordinateService`: devuelve `TSisaResult<T>`, que UHT no reflexiona): `Connect`/`Disconnect`/`GetConnectionState`, `GetCapabilities`, `CommandLaying`, `StopMotion`, `TriggerFire`, `ReadTelemetry` y `Poll(DeltaSeconds)`. El contrato documenta explícitamente que **todo se llama desde el game thread** y que una implementación de Fase 2 con I/O real debe hacer ese I/O en su propio hilo y entregar muestras ya terminadas, de modo que `Poll()` y `ReadTelemetry()` nunca bloqueen.

### `FSimulatedHardwareController`

El defecto de Fase 1: sin I/O, modela un montaje motorizado en proceso. Gira hacia la puntería ordenada a velocidades configurables **usando la misma matemática (`SisaAngle`) con la que Artillery apunta una pieza real**, rellena telemetría de encoder/GNSS/brújula/IMU y cuenta los disparos. Detalles deliberados:

- Un `CommandLaying` con elevación fuera de topes **no se rechaza**: se recorta al tope, como un montaje real, pero devuelve `Hardware.ElevationClamped` para que el operador no crea que llegará donde pidió.
- `Disconnect()` deja el tubo donde está (un enlace que se cae no recentra un tubo) y marca la telemetría como inválida.
- `SimulateLinkFailure(Reason)` fuerza el estado `Error` para poder ejercitar el manejo de fallos de Simulation sin desenchufar nada.

### Application — `UHardwareSubsystem`

`UGameInstanceSubsystem` que posee el controlador activo, ejecuta su `Poll()` una vez por frame (vía `FTSTicker`) y republica en el event bus de SISACore.

Los controladores se crean por **factorías registradas por `EHardwareControllerKind`**. La Fase 1 registra una (`Simulated`) en `Initialize`; la Fase 2 registrará Arduino/ESP32/PLC/STM32 desde el arranque de su propio módulo — y **ningún punto de llamada en Simulation cambia**, que es exactamente el objetivo de la costura.

También expone envoltorios `BlueprintCallable` (`ConnectToHardware`, `CommandLayingWithReason`, `StopMotion`, `TriggerFire`) porque `TSisaResult<T>` no es reflexionable, y comprueba capacidades antes de comandar (mandar puntería a un controlador que no la tiene falla con `Hardware.NoLayingControl` en vez de silenciarse).

### Eventos (`Public/HardwareEvents.h`)

`SISA.Hardware.Telemetry` (`FHardwareTelemetryEvent`) y `SISA.Hardware.ConnectionChanged` (`FHardwareConnectionEvent`). El sondeo ocurre cada frame pero la **republicación de telemetría se limita** por `TelemetryPublishIntervalSeconds` (0.1 s por defecto), para que 120 fps no inunden la UI. Los cambios de estado que el controlador hace por su cuenta (un enlace que se cae) se detectan en el sondeo y se publican sin esperar al siguiente comando.

## Refinación de SISACore: `SisaAngle.h`

Al implementar el montaje simulado apareció matemática de brújula idéntica a la del Módulo 4. En vez de duplicarla, se movió al shared kernel como `SisaAngle` (`Normalize360`, `ShortestDelta`, `StepTowards`, `StepTowardsLinear`) — funciones libres header-only, sin `UObject` ni reflexión. `FArtilleryLayingSolver` ahora delega en ellas conservando su API pública, y el controlador simulado las usa directamente. Es un añadido al Módulo 1 hecho en el 5, cubierto por su propio spec.

## Decisiones de diseño y limitaciones honestas

- **`Hardware` depende solo de `SISACore`**, como marca el grafo: no conoce Artillery ni Simulation. Los transportes de Fase 2 añadirán sus dependencias *dentro* de este módulo, nunca al revés.
- **No hay ni una línea de I/O real todavía**: los enums de transporte y los campos de `FHardwareConnectionConfig` son el contrato preparado; `Validate()` comprueba que la configuración es coherente, pero nadie abre un puerto. Eso es Fase 2.
- El montaje simulado **no modela** inercia, backlash, holgura de engranajes, latencia del enlace ni ruido de sensor (la telemetría es exacta y determinista, a propósito, para que los tests sean estables). Si se quiere ejercitar filtrado de sensores, habrá que añadir ruido configurable.
- `EHardwareConnectionState::Connecting` está en el contrato pero el controlador simulado nunca lo usa: conecta de forma instantánea. Un dispositivo real sí pasará por ese estado.
- `UHardwareSubsystem::Initialize` no se ejercita en los tests (requiere una colección de subsystems viva), así que el registro de la factoría por defecto y el ticker se verifican manualmente; ver abajo.

## Plan de pruebas — resultado real (34/34 en verde)

Ejecutado headless con `Automation RunTests SISA.Hardware -testexit="Automation Test Queue Empty"` (`failed=0, notRun=0`; un caso cuenta como "succeeded with warnings" porque provoca a propósito un fallo de enlace que registra un `UE_LOG(Warning)`).

- `SISA.Hardware.Config` (`HardwareConfigSpec.cpp`, 8): simulado sin transporte se acepta; simulado con transporte real y dispositivo físico sin transporte se rechazan; familia serie (Serial/RS-232/RS-485/CAN) exige puerto y baudios; familia de red (TCP/UDP/Ethernet) exige host y puerto en rango; dirección de dispositivo negativa y timeout no positivo se rechazan.
- `SISA.Hardware.SimulatedController` (`SimulatedHardwareControllerSpec.cpp`, 15): ciclo de vida del enlace (arranca desconectado, conecta, rechaza configuración de dispositivo físico y configuración inválida, mantiene el tubo al desconectar, entra en estado de error a petición); todos los comandos fallan sin enlace y el sondeo no mueve nada; movimiento (gira a las velocidades configuradas, llega sin sobrepasar y deja de reportar movimiento, recorta elevación al tope avisando, `StopMotion` detiene y no reanuda); telemetría (posición/rumbo/actitud válidos y coherentes, todas las capacidades anunciadas, contador de disparos que se reinicia al reconectar).
- `SISA.Hardware.Subsystem` (`HardwareSubsystemSpec.cpp`, 11): factorías (informa qué tipos puede construir, rechaza un tipo sin factoría con `Hardware.NoFactoryRegistered`, una factoría posterior reemplaza a la anterior); conexión (conecta el simulado y expone capacidades, estado vacío sin controlador, libera el controlador al desconectar, una reconexión reemplaza y cierra el anterior); comandos (todos fallan sin controlador, la puntería llega al montaje a través del subsystem, el disparador se propaga y se cuenta, sondear sin controlador no hace nada).
- `SISA.Core.Angle` (`SisaAngleSpec.cpp`, 5, en SISACore): normalización, delta más corto, paso limitado sin sobrepasar y con cruce por el Norte, y variante lineal que no envuelve (para elevación negativa).

### Qué **no** cubren los tests

`Initialize`/`Deinitialize` del subsystem (registro de la factoría por defecto, alta y baja del ticker) y la publicación real en el event bus: los specs construyen el subsystem con un `UGameInstance` desnudo, cuya colección de subsystems no está inicializada, así que el bus no existe y la publicación se omite silenciosamente. Verificación manual en el editor: entrar en PIE, comprobar en el log que el controlador simulado se conecta, suscribirse a `SISA.Hardware.Telemetry` desde Simulation (cuando exista) y confirmar que llegan muestras al ritmo de `TelemetryPublishIntervalSeconds`.
