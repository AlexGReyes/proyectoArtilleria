# Documentación de SISA

- [`fases-desarrollo.md`](fases-desarrollo.md) — las 3 fases del proyecto (digital, hardware real, VR) y qué incluye cada una.
- [`arquitectura.md`](arquitectura.md) — descomposición en plugins, grafo de dependencias, convenciones de carpetas y patrones transversales.
- [`modulo-01-core-gis.md`](modulo-01-core-gis.md) — módulo 1 (`SISACore` + `SISAGIS`): propósito, clases, decisiones de diseño, limitaciones conocidas y plan de pruebas.
- [`modulo-02-ballistics.md`](modulo-02-ballistics.md) — módulo 2 (`Ballistics`): motor RK4 de trayectoria con viento/temperatura/presión/densidad del aire reales, solve asíncrono, y plan de pruebas (13/13 en verde).
- [`modulo-03-ammunition.md`](modulo-03-ammunition.md) — módulo 3 (`Ammunition`): catálogo data-driven de municiones (Primary Data Assets / Data Tables), registro, validación y puente a Ballistics/área afectada (11/11 en verde).
- [`modulo-04-artillery.md`](modulo-04-artillery.md) — módulo 4 (`Artillery`): definición data-driven de piezas, Actor de pieza con posición geográfica y estado operativo, laying (azimut/elevación con límites y velocidades), inventario, historial de disparos, catálogo y roster/selección (34/34 en verde).

- [`modulo-05-hardware.md`](modulo-05-hardware.md) — módulo 5 (`Hardware`): la costura `IHardwareController` con el contrato completo de transportes de Fase 2, el controlador simulado de Fase 1, el subsystem con factorías por tipo de dispositivo, y `SisaAngle` en SISACore (34/34 en verde).

Los próximos módulos (Simulation, Networking, VR) añaden un archivo `modulo-NN-<nombre>.md` cada uno a medida que se completan, siguiendo el orden descrito en `arquitectura.md`.
