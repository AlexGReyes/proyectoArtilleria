# Módulo 3 — Ammunition (catálogo data-driven de municiones)

**Estado**: completado, compilando (`simuladorArtilleria1Editor`, Development, `Result: Succeeded`) y con **11/11 tests en verde**.

## Propósito

Catálogo de municiones ("administración de municiones"). Cada munición define nombre, calibre, velocidad inicial, peso, coeficiente balístico, tipo, carga propulsora, alcance máximo, radio efectivo, radio letal y tipo de explosión, **almacenado en Primary Data Assets o Data Tables — sin valores codificados en C++**. Es lo que después alimenta a Ballistics con los datos reales del proyectil y aporta los radios que Simulation dibujará como área afectada.

## Refinación del grafo de dependencias

Este módulo **depende de Ballistics** además de SISACore — desviación deliberada del grafo macro original (que ponía Ammunition solo sobre SISACore). Justificación: la identidad balística de una munición *son* exactamente `InitialSpeedMps`/`MassKg`/`BallisticCoefficient`; reutilizar `FBallisticProjectileParams` de Ballistics vía `ToBallisticProjectileParams()` evita duplicar el tipo y que ambos deriven. No crea ciclos (Ballistics es puro sobre SISACore). Registrado en `doc/arquitectura.md`.

## Componentes

### Domain (`Public/MunitionTypes.h`)

- `EMunitionType` (UENUM): `HighExplosive`, `Smoke`, `Illumination`, `Cluster`, `Guided`, `Practice`.
- `EMunitionBurstType` (UENUM, "tipo de explosión"): `Impact`, `Airburst`, `Proximity`, `Timed`, `None`.
- `FMunitionDefinition` (`USTRUCT BlueprintType`, hereda `FTableRowBase`): todos los campos del spec, en forma plana. Métodos:
  - `Validate(TArray<FSisaError>&)`: id no vacío; calibre/velocidad/masa/BC > 0; radios no negativos; y `LethalRadius <= EffectiveRadius` (el radio letal, más intenso, es el interior).
  - `ToBallisticProjectileParams()`: puente a Ballistics.
  - `MakeAffectedArea(FGeoCoordinate ImpactCenter)`: produce `FMunitionAffectedArea` (centro + radios).
- `FMunitionAffectedArea` (`USTRUCT BlueprintType`): base geométrica del área afectada; Simulation la compone con el punto de impacto de Ballistics (mapeado a geo por SISAGIS) para dibujar los círculos. Fase 1: solo geometría, sin modelo de daño cuantitativo.

### Almacenamiento (dos vías, según el spec)

- `UMunitionDataAsset` (`UPrimaryDataAsset`): un asset por munición, expuesto como tipo primario `"Munition"` (override `GetPrimaryAssetId()`). Puede contener contenido (`CanContainContent: true`).
- Data Table: como `FMunitionDefinition` hereda `FTableRowBase`, una tabla (o CSV/JSON importado) funciona sin tipo extra. Una sola struct sirve a ambos mecanismos (DRY).
- Config del Asset Manager en `Config/DefaultGame.ini` (`[/Script/Engine.AssetManagerSettings]`): registra el tipo primario `Munition` con AssetBaseClass `/Script/Ammunition.MunitionDataAsset` y directorio `/Game/Munitions`. Si el directorio no existe, el escaneo sale vacío sin error.

### Application (`Public/MunitionRegistrySubsystem.h`)

`UMunitionRegistrySubsystem` (`UGameInstanceSubsystem`): catálogo en memoria (`TMap<FName, FMunitionDefinition>`), punto de acceso para Artillery/Simulation. `RegisterDefinition` (valida y rechaza inválidas), `LoadFromDataTable`, `LoadFromAssetManager`, `FindById`, `GetAll`, `GetByType`, `Num`, `Clear` — todas `BlueprintCallable`. Sin lógica balística; solo catálogo.

## Limitaciones y notas honestas

- Los tests validan **estructura, almacenamiento y validación**, no el realismo de los valores de munición — eso lo aporta el diseñador en los assets/tables. Los valores usados en los tests (p. ej. 155 mm M107) son datos de prueba plausibles, no una tabla de tiro certificada.
- `LoadFromAssetManager()` depende de assets en disco y de la config del Asset Manager; no se cubre con Automation headless. Verificación manual: crear un `UMunitionDataAsset` en `/Game/Munitions` en el editor y confirmar que `LoadFromAssetManager()` lo incorpora al registro.
- "Carga propulsora" se modela como una masa (`PropellantChargeKilograms`); no se modelan zonas de carga múltiples con velocidad inicial por zona (extensión futura si se requiere).

## Plan de pruebas — resultado real (11/11 en verde)

Ejecutado headless con `Automation RunTests SISA.Ammunition -testexit="Automation Test Queue Empty"` (reporte JSON: `failed=0, notRun=0`).

`SISA.Ammunition.Definition` (`MunitionDefinitionSpec.cpp`):
- `Validate`: munición bien formada pasa; radio letal > efectivo falla (con `Ammunition.RadiiInconsistent`); calibre/velocidad/masa/BC ≤ 0 fallan; id vacío falla.
- `ToBallisticProjectileParams`: mapea velocidad/masa/BC.
- `MakeAffectedArea`: devuelve el centro dado y los radios de la munición.

`SISA.Ammunition.Registry` (`MunitionRegistrySpec.cpp`) — subsystem con `UGameInstance` de prueba como Outer (patrón del Módulo 1):
- `RegisterDefinition` + `FindById`; id inexistente → `bFound=false`; munición inválida rechazada (registro vacío).
- `GetByType` filtra; `GetAll`/`Num` cuentan.
- `LoadFromDataTable`: `UDataTable` en memoria (RowStruct = `FMunitionDefinition`) con filas, cargado y verificado.
