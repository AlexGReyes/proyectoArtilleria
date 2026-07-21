#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MunitionTypes.h"
#include "SisaResult.h"

DEFINE_SPEC(
	FMunitionDefinitionSpec,
	"SISA.Ammunition.Definition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	FMunitionDefinition MakeValidHE()
	{
		FMunitionDefinition M;
		M.MunitionId = TEXT("M107_HE");
		M.DisplayName = FText::FromString(TEXT("155mm M107 HE"));
		M.CaliberMillimeters = 155.0;
		M.Type = EMunitionType::HighExplosive;
		M.BurstType = EMunitionBurstType::Impact;
		M.PropellantChargeKilograms = 7.0;
		M.MaxRangeMeters = 18100.0;
		M.EffectiveRadiusMeters = 50.0;
		M.LethalRadiusMeters = 25.0;
		M.InitialSpeedMps = 684.0;
		M.MassKg = 43.2;
		M.BallisticCoefficient = 3500.0;
		return M;
	}
}

void FMunitionDefinitionSpec::Define()
{
	Describe("Validate", [this]()
	{
		It("accepts a well-formed munition", [this]()
		{
			TArray<FSisaError> Errors;
			TestTrue(TEXT("Valid"), MakeValidHE().Validate(Errors));
			TestEqual(TEXT("No errors"), Errors.Num(), 0);
		});

		It("rejects lethal radius larger than effective radius", [this]()
		{
			FMunitionDefinition M = MakeValidHE();
			M.LethalRadiusMeters = 80.0; // > EffectiveRadiusMeters (50)
			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), M.Validate(Errors));
			TestTrue(TEXT("Reported a radii error"),
				Errors.ContainsByPredicate([](const FSisaError& E) { return E.Code == FName(TEXT("Ammunition.RadiiInconsistent")); }));
		});

		It("rejects non-positive caliber, speed, mass and ballistic coefficient", [this]()
		{
			FMunitionDefinition M = MakeValidHE();
			M.CaliberMillimeters = 0.0;
			M.InitialSpeedMps = 0.0;
			M.MassKg = 0.0;
			M.BallisticCoefficient = 0.0;
			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), M.Validate(Errors));
			TestTrue(TEXT("At least four errors"), Errors.Num() >= 4);
		});

		It("rejects a missing id", [this]()
		{
			FMunitionDefinition M = MakeValidHE();
			M.MunitionId = NAME_None;
			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), M.Validate(Errors));
		});
	});

	Describe("ToBallisticProjectileParams", [this]()
	{
		It("maps speed, mass and ballistic coefficient", [this]()
		{
			const FMunitionDefinition M = MakeValidHE();
			const FBallisticProjectileParams P = M.ToBallisticProjectileParams();
			TestEqual(TEXT("Speed"), P.InitialSpeedMps, M.InitialSpeedMps);
			TestEqual(TEXT("Mass"), P.MassKg, M.MassKg);
			TestEqual(TEXT("BC"), P.BallisticCoefficient, M.BallisticCoefficient);
		});
	});

	Describe("MakeAffectedArea", [this]()
	{
		It("returns the given center and the munition radii", [this]()
		{
			const FMunitionDefinition M = MakeValidHE();
			const FGeoCoordinate Impact(40.4168, -3.7038, 650.0);
			const FMunitionAffectedArea Area = M.MakeAffectedArea(Impact);
			TestEqual(TEXT("Center latitude"), Area.Center.Latitude, Impact.Latitude);
			TestEqual(TEXT("Center longitude"), Area.Center.Longitude, Impact.Longitude);
			TestEqual(TEXT("Effective radius"), Area.EffectiveRadiusMeters, M.EffectiveRadiusMeters);
			TestEqual(TEXT("Lethal radius"), Area.LethalRadiusMeters, M.LethalRadiusMeters);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
