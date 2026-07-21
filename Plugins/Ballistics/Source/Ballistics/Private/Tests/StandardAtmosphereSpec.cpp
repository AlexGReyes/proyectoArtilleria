#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StandardAtmosphereModel.h"

DEFINE_SPEC(
	FStandardAtmosphereSpec,
	"SISA.Ballistics.StandardAtmosphere",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FStandardAtmosphereSpec::Define()
{
	Describe("Sea-level density", [this]()
	{
		It("is approximately 1.225 kg/m^3 at 15 C and 101325 Pa dry air", [this]()
		{
			FEnvironmentConditions Conditions; // defaults: 15 C, 101325 Pa, 0% humidity
			const FStandardAtmosphereModel Model(Conditions);
			const double Density = Model.GetAirDensityKgM3(0.0);
			TestTrue(TEXT("Density near 1.225"), FMath::IsNearlyEqual(Density, 1.225, 0.005));
		});
	});

	Describe("Altitude dependence", [this]()
	{
		It("decreases with altitude", [this]()
		{
			const FStandardAtmosphereModel Model(FEnvironmentConditions{});
			const double DensitySeaLevel = Model.GetAirDensityKgM3(0.0);
			const double DensityHigh = Model.GetAirDensityKgM3(5000.0);
			TestTrue(TEXT("Density at 5km is lower than at sea level"), DensityHigh < DensitySeaLevel);
			TestTrue(TEXT("Density at 5km is still positive"), DensityHigh > 0.0);
		});
	});

	Describe("Humidity", [this]()
	{
		It("makes air less dense at the same temperature and pressure", [this]()
		{
			FEnvironmentConditions Dry;
			Dry.RelativeHumidity01 = 0.0;
			FEnvironmentConditions Humid;
			Humid.RelativeHumidity01 = 1.0;
			Humid.TemperatureCelsius = 30.0; // warm air holds more vapor, amplifying the effect
			Dry.TemperatureCelsius = 30.0;

			const FStandardAtmosphereModel DryModel(Dry);
			const FStandardAtmosphereModel HumidModel(Humid);
			TestTrue(TEXT("Humid air is less dense than dry air"),
				HumidModel.GetAirDensityKgM3(0.0) < DryModel.GetAirDensityKgM3(0.0));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
