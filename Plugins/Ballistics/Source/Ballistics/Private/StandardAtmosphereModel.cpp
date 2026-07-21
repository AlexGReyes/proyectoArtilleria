#include "StandardAtmosphereModel.h"

namespace
{
	// ISA / physical constants.
	constexpr double TemperatureLapseRateKPerM = 0.0065;   // K/m, troposphere
	constexpr double GravityMps2 = 9.80665;                // m/s^2
	constexpr double UniversalGasConstant = 8.31446;       // J/(mol*K)
	constexpr double MolarMassDryAir = 0.0289644;          // kg/mol
	constexpr double MolarMassWaterVapor = 0.018016;       // kg/mol

	// Exponent in the barometric formula: g*M/(R*L).
	const double BarometricExponent =
		(GravityMps2 * MolarMassDryAir) / (UniversalGasConstant * TemperatureLapseRateKPerM);

	/** Saturation vapor pressure of water, Tetens formula, in Pascals (Tc in Celsius). */
	double SaturationVaporPressurePa(double TemperatureCelsius)
	{
		return 610.78 * FMath::Exp((17.27 * TemperatureCelsius) / (TemperatureCelsius + 237.3));
	}
}

FStandardAtmosphereModel::FStandardAtmosphereModel(const FEnvironmentConditions& Conditions)
	: GroundTemperatureKelvin(Conditions.TemperatureCelsius + 273.15)
	, GroundPressurePascals(Conditions.PressurePascals)
	, RelativeHumidity01(FMath::Clamp(Conditions.RelativeHumidity01, 0.0, 1.0))
	, WindVelocityEnu(Conditions.GetWindVelocityEnu())
{
}

double FStandardAtmosphereModel::GetAirDensityKgM3(double AltitudeMeters) const
{
	// Keep the model within a sane band; artillery apogees stay well inside the troposphere.
	const double Altitude = FMath::Clamp(AltitudeMeters, -1000.0, 20000.0);

	// Temperature and pressure at altitude (barometric formula).
	double TemperatureKelvin = GroundTemperatureKelvin - TemperatureLapseRateKPerM * Altitude;
	TemperatureKelvin = FMath::Max(TemperatureKelvin, 150.0); // floor to avoid nonsense high up

	const double PressureRatio = FMath::Pow(TemperatureKelvin / GroundTemperatureKelvin, BarometricExponent);
	const double PressurePa = GroundPressurePascals * PressureRatio;

	// Partial pressures of dry air and water vapor (humid air is less dense).
	const double TemperatureCelsius = TemperatureKelvin - 273.15;
	const double VaporPressurePa = RelativeHumidity01 * SaturationVaporPressurePa(TemperatureCelsius);
	const double DryPressurePa = FMath::Max(PressurePa - VaporPressurePa, 0.0);

	const double Density =
		(DryPressurePa * MolarMassDryAir + VaporPressurePa * MolarMassWaterVapor)
		/ (UniversalGasConstant * TemperatureKelvin);

	return Density;
}

FVector FStandardAtmosphereModel::GetWindVelocityEnu(double AltitudeMeters) const
{
	return WindVelocityEnu;
}
