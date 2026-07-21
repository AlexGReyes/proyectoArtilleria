# Módulo 1 — SISACore + SISAGIS

**Estado**: completado y compilando (`simuladorArtilleria1Editor`, Development, `Result: Succeeded`).

## Propósito

Sentar la base fundacional de la que depende todo el resto de SISA: un value object de coordenadas compartido y un servicio que traduzca esas coordenadas al mundo Cesium ya operativo (`Content/mapa.umap`, con `CesiumGeoreference` funcionando), sin que ningún otro plugin dependa de Cesium directamente.

## SISACore (shared kernel)

Sin dependencias de otros plugins SISA — solo `Core`, `CoreUObject`, `Engine`.

- **`FGeoCoordinate`** (`SisaGeoCoordinate.h`): value object WGS84 puro (latitud/longitud/altura), sin lógica de conversión.
- **`FEnuCoordinate`**: posición local East-North-Up anclada a un `FGeoCoordinate`.
- **`FSisaError` / `TSisaResult<T>`** (`SisaResult.h`): resultado explícito de operaciones que pueden fallar, en vez de excepciones. `TSisaResult<T>` es una plantilla C++ pura (Unreal Header Tool no reflexiona plantillas), pensada para consumo C++-a-C++, no para exponer a Blueprint directamente.
- **`USisaEventBusSubsystem`** (`SisaEventBusSubsystem.h`): pub/sub mínimo por canal `FName` + payload tipado por plantilla (`Publish<T>`/`Subscribe<T>`/`Unsubscribe<T>`). Los canales no se verifican por tipo en tiempo de compilación — publicar un tipo distinto al que se usó al suscribirse en el mismo canal es comportamiento indefinido; se documenta como convención a respetar.

## SISAGIS (única capa de contacto con Cesium)

- **`IGeoCoordinateService`** (`IGeoCoordinateService.h`): interfaz C++ pura (no `UINTERFACE`, por el mismo motivo de las plantillas) con conversión LLH↔Unreal, LLH↔ECEF, LLH↔UTM, LLH↔MGRS y rotación ENU→Unreal.
- **`UCesiumGeoreferenceAdapter`** (`UWorldSubsystem`): implementación real. Delega LLH↔Unreal en `ACesiumGeoreference`, LLH↔ECEF en `UCesiumWgs84Ellipsoid`, UTM/MGRS en `FUtmMgrsConverter`. Es la única clase de todo SISA que incluye headers de Cesium. Se resuelve vía `GetWorld()->GetSubsystem<UCesiumGeoreferenceAdapter>()`.
- **`FUtmMgrsConverter`** (`Private/UtmMgrsConverter.h/.cpp`): matemática UTM/MGRS propia (fórmulas de Snyder, elipsoide WGS84), sin `UObject` ni Cesium — testeable de forma aislada.
- **`IGeoDataLayerProvider`** + **`FNullGeoDataLayerProvider`**: contrato para capas futuras (GeoJSON/KML/Shapefile/DEM/3D Tiles/imágenes/tácticas). La implementación Fase 1 devuelve explícitamente "no implementado" en vez de fallar en silencio; los parsers reales quedan para cuando se pida un formato concreto.

## Decisiones de diseño relevantes

- **Renombre `Core`→`SISACore`, `GIS`→`SISAGIS`**: un plugin llamado `Core` colisiona con el módulo `Core` del propio motor y no compila. Se detectó al implementar, no estaba en el plan original.
- **Interfaces sin `UINTERFACE`**: como varios métodos devuelven `TSisaResult<T>` (plantilla), no son reflexionables por Unreal Header Tool. Se usan interfaces C++ nativas (herencia múltiple sobre el `UWorldSubsystem`), consistente con el requisito de que toda la lógica esté en C++, no en Blueprint.
- **`EnuRotatorToUnreal`**: el yaw de un `FRotator` "ENU" (interpretado como azimut de brújula, en grados horarios desde el norte) se relaciona con el yaw "East-South-Up" de Cesium mediante un offset fijo de -90° sobre el mismo eje Up — ambas convenciones giran en sentido horario visto desde arriba, así que no hace falta corrección de "handedness", solo el cambio de referencia. Queda pendiente confirmarlo visualmente en el editor antes de que Artillery dependa de él (ver limitaciones).

## Limitaciones conocidas

- **MGRS**: no cubre las excepciones de huso de Noruega/Svalbard (32V, 31X-37X) ni las zonas polares UPS (bandas A/B/Y/Z) — fuera de alcance para un simulador de artillería terrestre.
- **Precisión geodésica**: la conversión UTM/MGRS es una implementación propia (Snyder/WGS84), verificada por *round-trip* interno (ver Plan de pruebas), pero **no** cotejada contra una herramienta de geodesia de referencia externa. Recomendado antes de cualquier uso operativo real.
- **`EnuRotatorToUnreal`**: razonado matemáticamente pero sin confirmación visual en editor todavía.
- **`UCesiumGeoreferenceAdapter`**: no tiene test automatizado (depende de `UWorld`/Cesium); requiere la verificación manual descrita abajo.

## Plan de pruebas

- **`FSisaEventBusSubsystemSpec`** (`SISA.Core.EventBus`): Subscribe→Publish entrega el payload correcto; múltiples subscriptores reciben el evento; Unsubscribe detiene la entrega; publicar en un canal sin subscriptores no falla. Construye el subsystem con un `UGameInstance` de prueba como `Outer` (`UGameInstanceSubsystem` exige `ClassWithin=GameInstance`).
- **`FUtmMgrsConverterSpec`** (`SISA.GIS.UtmMgrsConverter`): *round-trip* LLH→UTM→LLH y LLH→MGRS→LLH (precisión 1m) para 5 puntos de referencia en ambos hemisferios y varios husos (Madrid, Bogotá, Buenos Aires, Sydney, un punto cerca de un borde de huso), con tolerancia deliberadamente generosa (~100m) porque valida consistencia interna, no precisión certificada. Además rechaza latitudes fuera de [-80°, 84°].
- **Resultado real de ejecución** (`Automation RunTests SISA.` en editor headless): todos los tests en verde. La primera corrida detectó un `Ensure condition failed` real (construcción incorrecta del subsystem en el test, no en el código de producción), que se corrigió.
- **Pendiente manual**: abrir `mapa.umap`, resolver `UCesiumGeoreferenceAdapter` desde Blueprint o consola con una coordenada conocida, y verificar que `GeoToUnreal`→`UnrealToGeo` vuelve al valor original; y apuntar un actor de prueba con `EnuRotatorToUnreal` hacia un azimut conocido (p. ej. norte/este) para confirmar visualmente la convención de yaw.

## Archivos

```
Plugins/SISACore/
├─ SISACore.uplugin
└─ Source/SISACore/
    ├─ Public/{SISACore.h, SisaGeoCoordinate.h, SisaResult.h, SisaEventBusSubsystem.h}
    └─ Private/{SISACore.cpp, Tests/SisaEventBusSubsystemSpec.cpp}

Plugins/SISAGIS/
├─ SISAGIS.uplugin
└─ Source/SISAGIS/
    ├─ Public/{SISAGIS.h, IGeoCoordinateService.h, IGeoDataLayerProvider.h, SisaUtmMgrsTypes.h, CesiumGeoreferenceAdapter.h}
    └─ Private/{SISAGIS.cpp, UtmMgrsConverter.h/.cpp, CesiumGeoreferenceAdapter.cpp, NullGeoDataLayerProvider.h/.cpp, Tests/UtmMgrsConverterSpec.cpp}
```
