# Módulo 4 — Artillery (piezas de artillería, inventario e historial de disparos)

**Estado**: completado, compilando (`simuladorArtilleria1Editor`, Development, `Result: Succeeded`) y con **32/32 tests en verde**.

## Propósito

"Administración de piezas de artillería": el modelo de datos de una pieza (ID, nombre, modelo, tipo, calibre, peso, alcance mínimo/máximo, límites y velocidades de azimut y elevación), el Actor que la representa colocada en el mundo con su posición geográfica y estado operativo, su inventario de municiones, su historial de disparos, y los dos puntos de acceso que la UI necesita: el **catálogo** de modelos y el **roster** de piezas colocadas con selección.

## Componentes

### Domain (`Public/ArtilleryTypes.h`)

- `EArtilleryPieceType` (UENUM): `Howitzer`, `Gun`, `Mortar`, `RocketLauncher`, `SelfPropelled`.
- `EArtilleryOperationalStatus` (UENUM): `Operational`, `Degraded`, `Maintenance`, `Displacing`, `OutOfAction`.
- `FArtilleryPieceDefinition` (`USTRUCT BlueprintType`, hereda `FTableRowBase`): todos los campos estáticos del spec. Métodos:
  - `Validate(TArray<FSisaError>&)`: id no vacío; calibre/peso > 0; `MaxRange > MinRange`; límites de elevación coherentes y dentro de [-90, 90]; arco de travesía en [0, 180]; velocidades de giro/elevación > 0.
  - `IsRangeEngageable(double)`: alcance dentro del sobre [min, max].
  - `IsMunitionCompatible(FName, double Calibre)`: el calibre es la restricción física y siempre aplica; `CompatibleMunitionIds`, si está poblada, la estrecha aún más (lista de municiones autorizadas).
- `FArtilleryAmmoStock`: una línea del cargador (munición + número de proyectiles).
- `FArtilleryFireRecord`: una entrada del historial — número de disparo, timestamp UTC, munición, posición de lanzamiento, laying (`FBallisticLaunchParams`) y, **rellenados después**, punto de impacto/alcance/tiempo de vuelo con su bandera `bImpactResolved` (el solve balístico es asíncrono, así que el registro nace incompleto por diseño).

### Laying — matemática pura (`Public/ArtilleryLaying.h`)

- `FArtilleryLayingState` (USTRUCT): azimut de montaje (centro del arco de travesía), azimut/elevación actuales y ordenados.
- `FArtilleryLayingSolver` (clase C++ pura, sin Actors ni `UWorld`, directamente testeable): `NormalizeAzimuth`, `ShortestAzimuthDelta`, `ClampAzimuthToArc`, `IsAzimuthWithinArc`, `ClampElevation`, `OrderLaying`, `Step` (avance limitado por las velocidades de giro y elevación, ambos ejes a la vez) e `IsOnTarget`.

El modelo de travesía es un **arco centrado en el azimut de montaje** con semiancho `TraverseHalfArcDegrees` — así se representan tanto una pieza remolcada (arco limitado) como una torreta/autopropulsada (semiancho 180 = travesía total). Los cálculos manejan correctamente el cruce por el Norte.

### Almacenamiento (dos vías, como en Ammunition)

- `UArtilleryPieceDataAsset` (`UPrimaryDataAsset`): un asset por modelo, tipo primario `"ArtilleryPiece"`.
- Data Table: `FArtilleryPieceDefinition` hereda `FTableRowBase`, así que una tabla (o CSV/JSON importado) funciona sin tipo extra.
- Config del Asset Manager en `Config/DefaultGame.ini`: tipo primario `ArtilleryPiece`, AssetBaseClass `/Script/Artillery.ArtilleryPieceDataAsset`, directorio `/Game/ArtilleryPieces`. Si el directorio no existe, el escaneo sale vacío sin error.

### Componentes de la pieza

- `UArtilleryInventoryComponent`: el cargador ("cantidad de municiones"). `InitialStock` editable en el editor y aplicado en `BeginPlay`; `AddRounds`, `ConsumeRounds` (**todo o nada**), `GetRounds`, `HasRounds`, `GetTotalRounds`, `GetStock`, `Clear`. Solo conoce ids de munición: la compatibilidad la decide el Actor y los datos de la munición viven en el registro de Ammunition.
- `UArtilleryFireHistoryComponent`: el historial. `AddRecord` estampa número de disparo y timestamp; `ResolveImpact(ShotNumber, ...)` completa el impacto cuando llega el resultado balístico; `MaxRecords` recorta el log conservando el **contador de disparos de por vida**, de modo que los números de disparo nunca se repiten.
- `UArtilleryLayingComponent`: conduce el tubo hacia el laying ordenado a las velocidades de la definición. Toda la matemática está en `FArtilleryLayingSolver`; el componente solo aporta delta time y **desactiva su propio tick al llegar al objetivo**, así que una pieza en reposo no cuesta nada. `ToBallisticLaunchParams()` es el puente hacia Ballistics.

### Infrastructure — `AArtilleryPieceActor`

La pieza colocada. Resuelve su definición en este orden: `PieceDataAsset` → `CatalogPieceId` (vía el catálogo) → la definición inline del Actor. Mantiene `GeoPosition` (WGS84) como posición autoritativa y la traduce a espacio Unreal con `UCesiumGeoreferenceAdapter` (`SetGeoPosition`, que devuelve `false` sin georeferencia — el caso de los tests headless — pero conserva la coordenada). Se registra en el roster en `BeginPlay` y se da de baja en `EndPlay`.

`CanFire(MunitionId)` devuelve `TSisaResult<bool>` con la **primera** causa de rechazo: estado no operativo, munición no seleccionada, munición desconocida en el registro, munición incompatible, sin proyectiles, o tubo aún no asentado en su laying. `Fire(MunitionId)` consume un proyectil, añade el registro al historial y publica `FArtilleryPieceFiredEvent` en el event bus. Ambas tienen envoltorio `BlueprintCallable` (`CanFireWithReason`/`FireRound`) porque `TSisaResult<T>` es un template y UHT no lo refleja.

**El Actor no resuelve la trayectoria**: publica el evento y Simulation, que sí posee el solve asíncrono, devuelve el impacto llamando a `ResolveImpact`. Esto mantiene Artillery libre de la orquestación.

### Application — catálogo y roster

- `UArtilleryCatalogSubsystem` (`UGameInstanceSubsystem`): catálogo de **modelos**, contraparte exacta de `UMunitionRegistrySubsystem` (`RegisterDefinition` con validación, `LoadFromDataTable`, `LoadFromAssetManager`, `FindById`, `GetAll`, `GetByType`, `Num`, `Clear`).
- `UArtilleryRosterSubsystem` (`UWorldSubsystem`): las piezas **colocadas** y la selección. Es un World subsystem, no un GameInstance subsystem, porque el roster es propiedad del nivel cargado y no debe sobrevivir a un cambio de mapa con Actors muertos. Guarda `TWeakObjectPtr` para no mantener vivo un Actor destruido, preserva el orden de registro para que el panel izquierdo liste de forma predecible, y expone `SelectPiece`/`ClearSelection`/`GetSelectedPiece`.

### Eventos (`Public/ArtilleryEvents.h`)

Canales del event bus de SISACore, con un tipo de payload por canal: `SISA.Artillery.PieceFired` (`FArtilleryPieceFiredEvent`), `SISA.Artillery.PieceRosterChanged` (`FArtilleryPieceRosterEvent`), `SISA.Artillery.PieceSelectionChanged` (`FArtilleryPieceSelectionEvent`). Con esto, Simulation y la UI reaccionan a los disparos y a la selección **sin incluir ningún header de Artillery**.

## Decisiones de diseño y limitaciones honestas

- **`SISAGIS` es dependencia privada** del módulo: solo la implementación del Actor toca la conversión geográfica, así que los consumidores de los headers públicos de Artillery nunca ven la costura GIS.
- El disparo modela **un proyectil por orden**; no hay cadencia de tiro, tiempo de recarga ni ráfagas. Es una extensión natural sobre `UArtilleryFireHistoryComponent`/`OperationalStatus` cuando se necesite.
- No se modela la **zona de carga propulsora** (una velocidad inicial por zona): la velocidad inicial viene entera de la munición, como en el Módulo 3.
- `CanFire` exige que el tubo esté asentado (`IsOnTarget`). Es el comportamiento realista, pero implica que un escenario que coloca piezas y dispara en el mismo frame debe usar `SnapToOrderedLaying()`.
- Los valores de pieza usados en tests (155 mm tipo M114A1) son datos de prueba plausibles, **no una tabla de tiro certificada**.

## Plan de pruebas — resultado real (32/32 en verde)

Ejecutado headless con `Automation RunTests SISA.Artillery -testexit="Automation Test Queue Empty"` (reporte JSON: `succeeded=32, failed=0, notRun=0`).

- `SISA.Artillery.Definition` (`ArtilleryPieceDefinitionSpec.cpp`, 8): validación (pieza correcta, id vacío, alcance máximo por debajo del mínimo, límites de elevación invertidos, velocidades no positivas), sobre de alcance, y compatibilidad de munición con y sin lista de autorización.
- `SISA.Artillery.Laying` (`ArtilleryLayingSpec.cpp`, 12): normalización de azimut, camino más corto por la brújula, recorte al arco de travesía (incluido un arco que cruza el Norte y un montaje de travesía total), `OrderLaying` recortando sin mover el tubo, y `Step` (respeta las velocidades, no sobrepasa, cruza el Norte por el lado corto, converge en el número de pasos esperado, ignora delta time no positivo).
- `SISA.Artillery.Inventory` (`ArtilleryInventorySpec.cpp`, 8): cargador (suma, consumo todo-o-nada, adiciones no positivas ignoradas, disponibilidad) e historial (números de disparo secuenciales y timestamp, resolución de impacto incluido un número de disparo inexistente, recorte a `MaxRecords` conservando el contador de por vida, log vacío).
- `SISA.Artillery.Catalog` (`ArtilleryCatalogSpec.cpp`, 6): registro/consulta, rechazo de definición inválida, filtro por tipo, carga desde Data Table y rechazo de tabla con row struct incorrecto o nula.

### Qué **no** cubren los tests

`AArtilleryPieceActor` y `UArtilleryRosterSubsystem` necesitan un `UWorld` con Actors (y, para el posicionamiento, una `ACesiumGeoreference` en el nivel), así que no se cubren con Automation headless. Verificación manual en el editor: colocar dos `AArtilleryPieceActor` en `mapa.umap` con `GeoPosition` distintas, comprobar que aterrizan en el terreno Cesium correcto, que aparecen en el roster (`GetAllPieces`), que `SelectPiece` publica el evento de selección, y que `FireRound` con munición cargada descuenta el proyectil y añade la entrada al historial.
