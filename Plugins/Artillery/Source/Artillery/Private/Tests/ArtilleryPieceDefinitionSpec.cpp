#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ArtilleryTypes.h"
#include "SisaResult.h"

DEFINE_SPEC(
	FArtilleryPieceDefinitionSpec,
	"SISA.Artillery.Definition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// A plausible towed 155 mm howitzer. Test data, not a certified firing table.
	FArtilleryPieceDefinition MakeHowitzer()
	{
		FArtilleryPieceDefinition Piece;
		Piece.PieceId = TEXT("M114A1");
		Piece.Model = TEXT("M114A1");
		Piece.Type = EArtilleryPieceType::Howitzer;
		Piece.CaliberMillimeters = 155.0;
		Piece.MassKilograms = 5800.0;
		Piece.MinRangeMeters = 500.0;
		Piece.MaxRangeMeters = 14600.0;
		Piece.TraverseHalfArcDegrees = 25.0;
		Piece.MinElevationDegrees = -2.0;
		Piece.MaxElevationDegrees = 63.0;
		Piece.TraverseRateDegreesPerSecond = 5.0;
		Piece.ElevationRateDegreesPerSecond = 3.0;
		return Piece;
	}

	bool HasError(const TArray<FSisaError>& Errors, const TCHAR* Code)
	{
		return Errors.ContainsByPredicate(
			[Code](const FSisaError& Error) { return Error.Code == FName(Code); });
	}
}

void FArtilleryPieceDefinitionSpec::Define()
{
	Describe("Validate", [this]()
	{
		It("accepts a well-formed piece", [this]()
		{
			TArray<FSisaError> Errors;
			TestTrue(TEXT("Valid"), MakeHowitzer().Validate(Errors));
			TestEqual(TEXT("No errors"), Errors.Num(), 0);
		});

		It("rejects an empty id", [this]()
		{
			FArtilleryPieceDefinition Piece = MakeHowitzer();
			Piece.PieceId = NAME_None;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Piece.Validate(Errors));
			TestTrue(TEXT("Reports missing id"), HasError(Errors, TEXT("Artillery.MissingId")));
		});

		It("rejects a maximum range below the minimum", [this]()
		{
			FArtilleryPieceDefinition Piece = MakeHowitzer();
			Piece.MaxRangeMeters = 100.0;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Piece.Validate(Errors));
			TestTrue(TEXT("Reports range inconsistency"), HasError(Errors, TEXT("Artillery.RangesInconsistent")));
		});

		It("rejects inverted elevation limits", [this]()
		{
			FArtilleryPieceDefinition Piece = MakeHowitzer();
			Piece.MinElevationDegrees = 60.0;
			Piece.MaxElevationDegrees = 10.0;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Piece.Validate(Errors));
			TestTrue(TEXT("Reports elevation inconsistency"), HasError(Errors, TEXT("Artillery.ElevationLimitsInconsistent")));
		});

		It("rejects non-positive slew rates", [this]()
		{
			FArtilleryPieceDefinition Piece = MakeHowitzer();
			Piece.TraverseRateDegreesPerSecond = 0.0;
			Piece.ElevationRateDegreesPerSecond = -1.0;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Piece.Validate(Errors));
			TestTrue(TEXT("Reports traverse rate"), HasError(Errors, TEXT("Artillery.InvalidTraverseRate")));
			TestTrue(TEXT("Reports elevation rate"), HasError(Errors, TEXT("Artillery.InvalidElevationRate")));
		});
	});

	Describe("IsRangeEngageable", [this]()
	{
		It("accepts ranges inside the envelope and rejects those outside", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeHowitzer();
			TestTrue(TEXT("Mid range"), Piece.IsRangeEngageable(8000.0));
			TestTrue(TEXT("At minimum"), Piece.IsRangeEngageable(500.0));
			TestTrue(TEXT("At maximum"), Piece.IsRangeEngageable(14600.0));
			TestFalse(TEXT("Too close"), Piece.IsRangeEngageable(200.0));
			TestFalse(TEXT("Too far"), Piece.IsRangeEngageable(20000.0));
		});
	});

	Describe("IsMunitionCompatible", [this]()
	{
		It("accepts any munition of matching caliber when no clearance list is authored", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeHowitzer();
			TestTrue(TEXT("Matching caliber"), Piece.IsMunitionCompatible(TEXT("HE_M107"), 155.0));
			TestFalse(TEXT("Wrong caliber"), Piece.IsMunitionCompatible(TEXT("HE_105"), 105.0));
		});

		It("honors the clearance list when one is authored", [this]()
		{
			FArtilleryPieceDefinition Piece = MakeHowitzer();
			Piece.CompatibleMunitionIds = { TEXT("HE_M107"), TEXT("SMK_M116") };

			TestTrue(TEXT("Cleared munition"), Piece.IsMunitionCompatible(TEXT("HE_M107"), 155.0));
			TestFalse(TEXT("Uncleared munition of right caliber"), Piece.IsMunitionCompatible(TEXT("ILL_M485"), 155.0));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
