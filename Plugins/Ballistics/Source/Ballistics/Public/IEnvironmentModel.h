#pragma once

#include "CoreMinimal.h"

/**
 * An atmosphere queryable by altitude: supplies the air density and wind that
 * the ballistic solver needs at each point along a trajectory. Kept a plain C++
 * interface (no UINTERFACE) because it is consumed only by C++ solver code and
 * has no need for Blueprint reflection.
 *
 * This is the extension seam requested by the project: Phase 1 ships
 * FStandardAtmosphereModel (real ISA density from ground weather, constant
 * wind); altitude-varying wind, non-standard lapse rates, or measured sounding
 * data can be added later as new implementations without touching the solver.
 */
class IEnvironmentModel
{
public:
	virtual ~IEnvironmentModel() = default;

	/** Air density in kg/m^3 at the given altitude above the launch point, in meters. */
	virtual double GetAirDensityKgM3(double AltitudeMeters) const = 0;

	/** Wind velocity as an ENU vector (X=East, Y=North, Z=Up) in m/s at the given altitude. */
	virtual FVector GetWindVelocityEnu(double AltitudeMeters) const = 0;
};
