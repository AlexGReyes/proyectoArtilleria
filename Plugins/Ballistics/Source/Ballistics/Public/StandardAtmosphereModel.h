#pragma once

#include "CoreMinimal.h"
#include "IEnvironmentModel.h"
#include "BallisticTypes.h"

/**
 * International Standard Atmosphere (ISA) troposphere model, seeded with the
 * ground-level weather in FEnvironmentConditions. Air density falls off with
 * altitude following the standard temperature lapse rate; density is computed
 * from temperature, pressure and relative humidity (humid air is slightly less
 * dense than dry air at the same temperature and pressure). Wind is treated as
 * constant with altitude in this module.
 *
 * A plain copyable value type (not a UObject) so it can be constructed on the
 * game thread from the weather panel and safely handed to a background solve.
 */
class BALLISTICS_API FStandardAtmosphereModel : public IEnvironmentModel
{
public:
	FStandardAtmosphereModel() = default;
	explicit FStandardAtmosphereModel(const FEnvironmentConditions& Conditions);

	//~ Begin IEnvironmentModel
	virtual double GetAirDensityKgM3(double AltitudeMeters) const override;
	virtual FVector GetWindVelocityEnu(double AltitudeMeters) const override;
	//~ End IEnvironmentModel

private:
	/** Ground-level temperature in Kelvin. */
	double GroundTemperatureKelvin = 288.15;

	/** Ground-level pressure in Pascals. */
	double GroundPressurePascals = 101325.0;

	/** Relative humidity, 0..1, assumed constant with altitude. */
	double RelativeHumidity01 = 0.0;

	/** Wind velocity as an ENU vector in m/s, constant with altitude. */
	FVector WindVelocityEnu = FVector::ZeroVector;
};
