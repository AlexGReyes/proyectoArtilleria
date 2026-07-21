# Módulo 2 — Ballistics (con clima real)

**Estado**: completado, compilando (`simuladorArtilleria1Editor`, Development, `Result: Succeeded`) y con **13/13 tests en verde**.

## Propósito

Motor matemático de trayectoria del que dependerán Artillery y Simulation. Calcula, para un proyectil disparado con una velocidad inicial, azimut y elevación dadas, y bajo unas condiciones atmosféricas dadas: la trayectoria completa (polilínea para visualizar), el tiempo de vuelo, la altura máxima (apogeo), el alcance, el punto de impacto, la velocidad residual y la energía de impacto.

El usuario amplió el alcance respecto al plan macro: **viento, temperatura, presión y densidad del aire son entradas reales del cálculo**, no una interfaz vacía para el futuro. Coriolis/rotación terrestre quedan como extensión futura (impacto insignificante a alcances de artillería de campaña) — el código deja aislado el punto exacto donde se sumarían.

Requisito arquitectónico clave: Ballistics es **puro** — depende solo de `SISACore` + módulos de motor, **sin Cesium ni UWorld** en la matemática. Trabaja en un marco local East-North-Up (ENU) en metros con origen en la pieza; Artillery/Simulation mapean ese ENU al mundo Cesium/Unreal con `IGeoCoordinateService` (SISAGIS). Esto lo hace testeable headless, como `FUtmMgrsConverter` en el Módulo 1.

## Componentes

### Domain — contratos e I/O puros (`Public/`)

- **`BallisticTypes.h`** (todos `USTRUCT BlueprintType`, para que la UI de Simulation los lea/edite):
  - `FBallisticProjectileParams`: `InitialSpeedMps`, `MassKg`, `BallisticCoefficient` (kg/m², BC = m/(Cd·A)).
  - `FBallisticLaunchParams`: `AzimuthDegrees` (brújula, horario desde el Norte), `ElevationDegrees` (sobre el horizonte).
  - `FEnvironmentConditions`: `TemperatureCelsius`, `PressurePascals`, `RelativeHumidity01`, viento como `WindSpeedMps` + `WindFromDirectionDegrees` (convención meteorológica), con helper `GetWindVelocityEnu()`. Estos son los campos que el panel de clima de Simulation editará.
  - `FBallisticsSolveConfig`: `IntegrationTimeStepSeconds` (0.01), `MaxFlightTimeSeconds` (300), `OutputSampleIntervalSeconds` (0.25, densidad de la polilínea de salida, independiente del dt), `ImpactPlaneHeightMeters` (0), `GravityMps2` (9.80665).
  - `FTrajectoryPoint`, `FBallisticTrajectory` (salida con la polilínea + métricas resumen + `bImpacted`).
- **`IEnvironmentModel.h`**: interfaz C++ pura — `GetAirDensityKgM3(alt)`, `GetWindVelocityEnu(alt)`. Seam de extensión (viento por capas, sondeos reales) sin tocar el solver.
- **`IBallisticsEngine.h`**: interfaz C++ pura — `Solve(projectile, launch, environment, config) → TSisaResult<FBallisticTrajectory>`. No es `UINTERFACE` porque devuelve una plantilla (`TSisaResult`) no reflexionable por UHT.

### Implementaciones

- **`FStandardAtmosphereModel`** (`Public/StandardAtmosphereModel.h` + `.cpp`): modelo ISA de troposfera sembrado con las condiciones a nivel del suelo. Densidad del aire por fórmula barométrica + corrección de aire húmedo (aire húmedo es menos denso). Viento constante con la altitud en este módulo, detrás de la interfaz. Tipo valor copiable (no UObject) para construirse en el game thread y pasarse seguro a un solve en segundo plano.
- **`FNumericalBallisticsEngine`** (`Private/NumericalBallisticsEngine.h`/`.cpp`): integrador **RK4** de paso fijo. Aceleración = gravedad + arrastre (`a_drag = 0.5·ρ(alt)·|v_rel|·v_rel / BC`, opuesta a la velocidad relativa al aire). El cálculo de aceleración está aislado en `ComputeAcceleration()` → único punto donde se sumaría Coriolis. Detección de impacto por cruce del plano de impacto con interpolación lineal al punto exacto. Valida entradas (BC>0, masa>0, velocidad>0, elevación∈[0,90]) devolviendo `TSisaResult::Err`. Privado; los tests lo incluyen directamente.
- **`UBallisticsSubsystem`** (`UGameInstanceSubsystem`, `Public/BallisticsSubsystem.h`/`.cpp`): punto de entrada Application para otros plugins.
  - `SolveSync(...)`: solve inmediato en el hilo llamante.
  - `SolveAsync(..., TSharedRef<const IEnvironmentModel>, ..., OnComplete)`: cumple el requisito de cálculo en hilos independientes — lanza el solve en un hilo de fondo (`AsyncTask(AnyBackgroundThreadNormalTask)`) y marshala `OnComplete` de vuelta al game thread. El modelo de entorno va como `TSharedRef<const>` para sobrevivir al trabajo async. El engine es sin estado, así que se construye local dentro del `.cpp` (esto además evitó un problema de tipo incompleto con `TUniquePtr` en el código generado por UHT).

## Decisiones y limitaciones

- **Marco ENU local, no geográfico**: mantiene el solver puro y testeable; la georreferenciación es responsabilidad de Artillery/Simulation vía SISAGIS.
- **Modelo de arrastre simple**: un coeficiente balístico (Cd·A efectivo) constante, no curvas estándar G1/G7 ni Mach-dependencia del Cd — mejora futura. Los tests validan **consistencia física y concordancia con la solución analítica en vacío**, no precisión balística certificada contra tablas de tiro reales.
- **Área afectada y estela Niagara** NO son de este módulo: el área (radios efectivo/letal) depende de Ammunition (Módulo 3) y la estela depende de UWorld/Niagara (Simulation). Aquí se produce solo su base física (punto de impacto, velocidad/energía de impacto y la polilínea de trayectoria).
- **Coriolis/rotación terrestre**: no implementados; punto de extensión marcado en `ComputeAcceleration()`.

## Plan de pruebas — resultado real (13/13 en verde)

Ejecutado headless con `Automation RunTests SISA.Ballistics -testexit="Automation Test Queue Empty"` (reporte JSON: `succeeded=13, failed=0`).

`SISA.Ballistics.Engine` (`BallisticsEngineSpec.cpp`):
- **Vacío vs analítico**: con arrastre desactivado, alcance/apogeo/tiempo de vuelo coinciden con las fórmulas cerradas (v²·sin2θ/g, v²·sin²θ/2g, 2v·sinθ/g) al 0.5%.
- **Arrastre**: con aire estándar, alcance < vacío.
- **Viento**: cola aumenta alcance, frente lo reduce, lateral deflecta el impacto en el sentido correcto.
- **Densidad**: aire más frío (más denso) → menor alcance.
- **Energía de impacto** = 0.5·m·v_residual².
- **Validación**: velocidad/masa/BC ≤ 0 y elevación fuera de [0,90] → error.

`SISA.Ballistics.StandardAtmosphere` (`StandardAtmosphereSpec.cpp`):
- Densidad a nivel del mar ≈ 1.225 kg/m³ (15 °C, 101325 Pa, seco).
- La densidad decrece con la altitud.
- El aire húmedo es menos denso que el seco a igual T y P.

`SolveAsync` no se cubre con Automation headless (hilos + game thread); reutiliza el mismo `SolveSync` ya testeado. Verificación manual opcional.

## Nota de proceso

Durante la compilación surgió un problema de tipo incompleto (`TUniquePtr<IBallisticsEngine>` como miembro del subsystem, que el código generado por UHT no podía destruir sin el tipo completo, no incluible en un header público). Se resolvió no almacenando el engine como miembro (es sin estado) y construyéndolo bajo demanda en el `.cpp`. Además, el enlace de los DLLs requiere el editor cerrado (bloquea los binarios); y la invocación headless correcta de los tests usa `-testexit="Automation Test Queue Empty"` (no `; Quit`, que cierra el editor antes de que corran los tests).
