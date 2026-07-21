#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UtmMgrsConverter.h"

DEFINE_SPEC(
	FUtmMgrsConverterSpec,
	"SISA.GIS.UtmMgrsConverter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// Round-trip reference points only (no external cross-validation was available while
	// writing this test — see the "known limitation" comment in UtmMgrsConverter.h).
	// Chosen to cover both hemispheres and several UTM zones.
	struct FNamedPoint
	{
		const TCHAR* Name;
		FGeoCoordinate Geo;
	};

	const TArray<FNamedPoint>& GetReferencePoints()
	{
		static const TArray<FNamedPoint> Points = {
			{ TEXT("Madrid"), FGeoCoordinate(40.4168, -3.7038, 650.0) },
			{ TEXT("Bogota"), FGeoCoordinate(4.7110, -74.0721, 2640.0) },
			{ TEXT("BuenosAires"), FGeoCoordinate(-34.6037, -58.3816, 25.0) },
			{ TEXT("Sydney"), FGeoCoordinate(-33.8688, 151.2093, 58.0) },
			{ TEXT("NearZoneBoundary"), FGeoCoordinate(45.0, 5.99, 200.0) },
		};
		return Points;
	}

	// Generous on purpose: this checks the math is self-consistent (no sign/offset/zone
	// bugs), not that it matches a certified reference within survey-grade tolerance.
	constexpr double DegreesTolerance = 0.001; // roughly 100m at the equator
}

void FUtmMgrsConverterSpec::Define()
{
	Describe("UTM round-trip", [this]()
	{
		for (const FNamedPoint& Point : GetReferencePoints())
		{
			It(FString::Printf(TEXT("recovers %s within tolerance"), Point.Name), [this, Point]()
			{
				const TSisaResult<FUtmCoordinate> UtmResult = FUtmMgrsConverter::ToUtm(Point.Geo);
				if (!TestTrue(TEXT("ToUtm succeeded"), UtmResult.IsOk()))
				{
					return;
				}

				const TSisaResult<FGeoCoordinate> GeoResult = FUtmMgrsConverter::FromUtm(UtmResult.GetValue());
				if (!TestTrue(TEXT("FromUtm succeeded"), GeoResult.IsOk()))
				{
					return;
				}

				TestTrue(TEXT("Latitude round-trips"), FMath::IsNearlyEqual(GeoResult.GetValue().Latitude, Point.Geo.Latitude, DegreesTolerance));
				TestTrue(TEXT("Longitude round-trips"), FMath::IsNearlyEqual(GeoResult.GetValue().Longitude, Point.Geo.Longitude, DegreesTolerance));
			});
		}

		It("rejects latitudes outside [-80, 84]", [this]()
		{
			const TSisaResult<FUtmCoordinate> Result = FUtmMgrsConverter::ToUtm(FGeoCoordinate(85.0, 0.0, 0.0));
			TestTrue(TEXT("Out-of-range latitude is rejected"), Result.IsError());
		});
	});

	Describe("MGRS round-trip", [this]()
	{
		for (const FNamedPoint& Point : GetReferencePoints())
		{
			It(FString::Printf(TEXT("recovers %s within tolerance at 1m precision"), Point.Name), [this, Point]()
			{
				const TSisaResult<FString> MgrsResult = FUtmMgrsConverter::ToMgrs(Point.Geo, 5);
				if (!TestTrue(TEXT("ToMgrs succeeded"), MgrsResult.IsOk()))
				{
					return;
				}

				const TSisaResult<FGeoCoordinate> GeoResult = FUtmMgrsConverter::FromMgrs(MgrsResult.GetValue());
				if (!TestTrue(FString::Printf(TEXT("FromMgrs succeeded for '%s'"), *MgrsResult.GetValue()), GeoResult.IsOk()))
				{
					return;
				}

				TestTrue(TEXT("Latitude round-trips"), FMath::IsNearlyEqual(GeoResult.GetValue().Latitude, Point.Geo.Latitude, DegreesTolerance));
				TestTrue(TEXT("Longitude round-trips"), FMath::IsNearlyEqual(GeoResult.GetValue().Longitude, Point.Geo.Longitude, DegreesTolerance));
			});
		}
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
