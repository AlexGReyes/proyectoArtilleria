# Arquitectura de SISA

## Principios

El proyecto se construye sobre la base Unreal Engine 5.8 + Cesium for Unreal ya existente (visualización de globo, navegación, terreno 3D y coordenadas geográficas reales, funcionando antes de empezar SISA). Toda funcionalidad nueva se organiza en **plugins de Unreal Engine**, uno por responsabilidad, siguiendo SOLID, Clean Architecture y Domain-Driven Design, con Blueprints reservados solo para UI, animaciones e integración visual — el resto es C++.

Unreal Engine no tiene "capas" nativas de Clean Architecture, así que la adaptación pragmática usada en SISA es:

| Capa | Qué contiene | Regla |
|---|---|---|
| **Domain** | `USTRUCT`s (value objects) e interfaces C++ puras (`IBallisticsEngine`, `IHardwareController`, `IGeoCoordinateService`) | Cero dependencia de Actors, Slate o Cesium |
| **Application** | `UObject`s de servicio que orquestan casos de uso | No conocen Actors concretos ni Cesium directamente |
| **Infrastructure** | Actors/Components/Subsystems que sí tocan Cesium, Niagara, Chaos, hardware, red | Implementan las interfaces de Domain |

## Descomposición en plugins

| Plugin | Propósito | Depende de |
|---|---|---|
| **SISACore** | Shared kernel: value objects comunes (coordenadas, `TSisaResult<T>`/`FSisaError`), event bus (`USisaEventBusSubsystem`) | — |
| **SISAGIS** | Única capa de contacto con Cesium: `IGeoCoordinateService` (WGS84/ECEF/UTM/MGRS/ENU), `IGeoDataLayerProvider` para capas futuras (GeoJSON/KML/Shapefile/DEM/3D Tiles/imágenes/tácticas) | SISACore, CesiumForUnreal |
| **Ballistics** | Motor matemático de trayectoria con viento, temperatura, presión y densidad del aire como variables reales del cálculo (no solo un contrato para el futuro), cálculo asíncrono. Efecto Coriolis/rotación terrestre quedan como extensión futura | SISACore |
| **Ammunition** | Catálogo de municiones vía `PrimaryDataAsset`/`DataTable`; aporta los parámetros balísticos del proyectil y los radios efectivo/letal del área afectada | SISACore, Ballistics |
| **Artillery** | Entidades/Actors de piezas de artillería, inventario, historial de disparos | SISACore, SISAGIS, Ammunition, Ballistics |
| **Hardware** | `IHardwareController` + implementación simulada por defecto (Fase 1); implementaciones reales en Fase 2 sin tocar Simulation | SISACore |
| **Networking** | Interfaces/DTOs cliente-servidor-instructor-observador + implementación local no-op (Fase 1) | SISACore |
| **Simulation** | Orquesta escenarios, aloja la UI estilo C4ISR (CommonUI): colocación/selección de piezas sobre el mapa, círculos de área afectada (radio efectivo/letal) en el punto de impacto, y estela Niagara del proyectil en vuelo | SISACore, SISAGIS, Artillery, Ammunition, Ballistics, Hardware\*, Networking\* |
| **VR** | Reutiliza el 100% de Simulation; solo cambia la interacción (Fase 3) | SISACore, Simulation |

\* Simulation depende de las **interfaces** de Hardware/Networking, nunca de una implementación física — así el cambio Fase 1→2 no toca lógica de simulación.

Los nombres de plugin llevan el prefijo `SISA` (`SISACore`, `SISAGIS`, ...) cuando hay riesgo real de colisión con módulos del motor o del marketplace — por ejemplo, un plugin llamado `Core` a secas colisiona con el módulo `Core` del propio Unreal Engine y no compila.

### Grafo de dependencias (Fase 1)

```
SISACore
 ├─ SISAGIS
 ├─ Ballistics
 ├─ Ammunition   (Ballistics)
 ├─ Hardware
 ├─ Networking
 ├─ Artillery    (SISAGIS + Ammunition + Ballistics)
 ├─ Simulation   (Artillery + Ammunition + Ballistics + SISAGIS + Hardware* + Networking*)
 └─ VR (fase 3, depende solo de Simulation)
```

### Convención de carpetas

```
simuladorArtilleria1/
├─ Source/simuladorArtilleria1/     (módulo de lanzamiento, se mantiene delgado)
├─ Plugins/
│   ├─ SISACore/    SISACore.uplugin, Source/SISACore/{Public,Private}
│   ├─ SISAGIS/     SISAGIS.uplugin,  Source/SISAGIS/{Public,Private}
│   └─ ...          (Ballistics, Ammunition, Artillery, Hardware, Networking, Simulation, VR)
├─ Content/          (assets compartidos: mapa.umap, escenarios, UI)
└─ doc/              (esta documentación)
```

## Patrones transversales

- **Data-driven**: `PrimaryDataAsset`/`DataTable` para piezas y municiones, sin valores hardcodeados.
- **Result/error explícito** (`TSisaResult<T>`/`FSisaError`) en servicios de Application, en vez de excepciones.
- **Async**: cálculo balístico en tareas asíncronas, nunca bloqueando el game thread.
- **Event bus** (`USisaEventBusSubsystem`, en SISACore): desacopla, por ejemplo, Simulation de Artillery/UI (patrón Observer) sin includes cruzados entre plugins.
- **Pruebas**: UE Automation Spec (`DEFINE_SPEC`/`FAutomationTestBase`), ejecutables desde Session Frontend o `UnrealEditor-Cmd.exe ... -ExecCmds="Automation RunTests SISA.; Quit"`. La lógica pura (matemática balística, conversión GIS) se diseña para ser testeable sin `UWorld`.

## Orden de implementación

Por dependencias, no por el orden en que se listaron las features originalmente:

1. **SISACore + SISAGIS** — fundacional (✅ completado, ver `doc/modulo-01-core-gis.md`)
2. **Ballistics** — motor puro, independiente de Artillery
3. **Ammunition**
4. **Artillery**
5. **Hardware** (interfaz + implementación simulada)
6. **Simulation** (orquestación, escenarios, UI C4ISR) + **Networking** (interfaces/stub)
7. **VR** — no antes de Fase 3
