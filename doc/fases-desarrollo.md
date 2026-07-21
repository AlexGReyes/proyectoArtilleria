# Fases de desarrollo de SISA

El desarrollo de SISA está dividido en tres fases. Cada fase se apoya en la anterior sin reescribir la lógica de simulación ya construida — lo único que cambia entre fases es la capa de interacción/infraestructura (hardware real, realidad virtual), gracias a las interfaces definidas desde la Fase 1 (`IHardwareController`, etc.).

## Fase 1 — Simulación completamente digital

Toda la interacción ocurre dentro de Unreal Engine, sin hardware físico. Es la fase en desarrollo actualmente.

Incluye:

- **Administración de piezas de artillería**: ID, nombre, modelo, tipo, calibre, peso, alcance mínimo/máximo, azimut, ángulo de elevación, velocidad de giro horizontal, velocidad de elevación, posición geográfica (latitud/longitud/altitud), estado operativo, cantidad de municiones, historial de disparos.
- **Administración de municiones**: nombre, calibre, velocidad inicial, peso, coeficiente balístico, tipo (Alto Explosivo, Humo, Iluminación, Racimo, Guiada, Práctica), carga propulsora, alcance máximo, radio efectivo, radio letal, tipo de explosión. Todo modelado con Primary Data Assets / Data Tables, sin valores codificados.
- **Sistema balístico**: motor independiente que calcula trayectoria, tiempo de vuelo, altura máxima, alcance, punto de impacto, velocidad residual y energía de impacto, ejecutado en tareas asíncronas. Arquitectura preparada para incorporar después viento, temperatura, humedad, presión atmosférica, efecto Coriolis, rotación terrestre y densidad del aire.
- **Integración GIS**: Cesium como base geoespacial — WGS84, MGRS, UTM, ENU y conversión entre sistemas, terreno real, elevaciones, visualización mundial. Contrato preparado (no necesariamente implementado por completo en esta fase) para GeoJSON, KML, Shapefile, DEM, 3D Tiles, imágenes satelitales y capas tácticas.
- **Interfaz gráfica** estilo C4ISR: panel izquierdo (lista de piezas y estado operativo), panel derecho (propiedades, munición, orientación, elevación), panel inferior (consola de eventos, historial de disparos), panel superior (herramientas, simulación, configuración, clima, escenarios), mapa principal vía Cesium.
- **Escenarios configurables**: terreno, clima, objetivos, observadores, obstáculos, posiciones amigas y enemigas.

## Fase 2 — Integración con hardware real

Se integran controles físicos para operar una pieza de artillería real o una maqueta instrumentada, sin modificar la lógica del simulador — solo cambia la implementación de `IHardwareController`, la capa de abstracción definida desde la Fase 1.

Incluye:

- Implementaciones de `IHardwareController` para Arduino, ESP32, PLC, STM32, sobre RS232, RS485, CAN Bus, TCP/IP, UDP, Serial o Ethernet.
- Soporte de sensores: encoders absolutos e incrementales, IMU, GPS, brújula electrónica, sensores de posición, motores eléctricos, actuadores.

## Fase 3 — Realidad Virtual

Se añade soporte de VR reutilizando el 100% de la lógica de simulación ya construida — la VR únicamente cambia la forma de interacción, no la simulación en sí.

Incluye:

- Soporte OpenXR, Meta Quest, HTC Vive, Pico, SteamVR.
- Modo Escritorio, modo VR y modo Mixto.
- Interacciones en VR: caminar alrededor de la pieza, manipular controles, apuntar, cargar munición, disparar, observar la trayectoria y observar el impacto.

## Cómo se rastrea el avance

El progreso dentro de cada fase se documenta módulo a módulo en `doc/` (ver `doc/modulo-01-core-gis.md` como ejemplo) y en los mensajes de commit del repositorio.
