# SISA — Sistema Integral de Simulación de Artillería

SISA es un simulador profesional de artillería construido sobre **Unreal Engine 5.8** (C++) y **Cesium for Unreal**, pensado para entrenamiento, análisis táctico y simulación balística sobre cartografía mundial real (terreno 3D, coordenadas geográficas), con una arquitectura preparada para evolucionar hacia integración con hardware físico y realidad virtual sin reescribir la lógica de simulación.

El proyecto parte de una base ya funcional (Unreal Engine 5.8 + Cesium for Unreal integrados, con visualización de globo, navegación y terreno 3D operativos) y se desarrolla de forma incremental, módulo por módulo, sobre una arquitectura de plugins desacoplados siguiendo SOLID, Clean Architecture y Domain-Driven Design.

## Estado actual

**Módulo 1 completado**: plugins `SISACore` (shared kernel: value objects de coordenadas, resultado/error, event bus) y `SISAGIS` (única capa de contacto con Cesium: conversión WGS84/ECEF/UTM/MGRS, contrato para capas geoespaciales futuras). Ver [`doc/modulo-01-core-gis.md`](doc/modulo-01-core-gis.md).

**Módulo 2 completado**: plugin `Ballistics` (motor RK4 de trayectoria puro, con viento/temperatura/presión/densidad del aire reales y solve asíncrono; 13/13 tests en verde). Ver [`doc/modulo-02-ballistics.md`](doc/modulo-02-ballistics.md).

**Módulo 3 completado**: plugin `Ammunition` (catálogo data-driven de municiones vía Primary Data Assets / Data Tables, con registro, validación y puente a Ballistics/área afectada; 11/11 tests en verde). Ver [`doc/modulo-03-ammunition.md`](doc/modulo-03-ammunition.md).

**Módulo 4 completado**: plugin `Artillery` (definición data-driven de piezas, Actor de pieza con posición geográfica/estado operativo, laying con límites y velocidades reales, inventario, historial de disparos, catálogo y roster con selección; 34/34 tests en verde). Ver [`doc/modulo-04-artillery.md`](doc/modulo-04-artillery.md).

**Módulo 5 completado**: plugin `Hardware` (costura `IHardwareController` con el contrato completo de transportes de Fase 2, controlador simulado de Fase 1, subsystem con factorías por tipo de dispositivo y telemetría en el event bus; 34/34 tests en verde). Ver [`doc/modulo-05-hardware.md`](doc/modulo-05-hardware.md).

Suite completa: **112/112 tests en verde**.

## Stack técnico

- Unreal Engine 5.8, todo en C++ (Blueprints reservados para UI/animaciones/integración visual)
- Cesium for Unreal (terreno 3D, coordenadas geográficas reales)
- Enhanced Input
- Arquitectura basada en plugins de Unreal Engine, uno por responsabilidad (GIS, Artillery, Ballistics, Ammunition, Simulation, Hardware, Networking, VR)

## Documentación

Toda la documentación del proyecto vive en [`doc/`](doc/):

- [`doc/fases-desarrollo.md`](doc/fases-desarrollo.md) — las 3 fases del proyecto y qué incluye cada una.
- [`doc/arquitectura.md`](doc/arquitectura.md) — descomposición en plugins, grafo de dependencias, convenciones y patrones transversales.
- [`doc/modulo-01-core-gis.md`](doc/modulo-01-core-gis.md) — diseño, clases, decisiones y plan de pruebas del Módulo 1 (y análogamente `modulo-02-ballistics.md`, `modulo-03-ammunition.md`, `modulo-04-artillery.md`).

Para instrucciones de compilación, estructura del código fuente y guía de trabajo en el repositorio, ver [`CLAUDE.md`](CLAUDE.md).
