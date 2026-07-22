#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ArtilleryCatalogSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"

DEFINE_SPEC(
	FArtilleryCatalogSpec,
	"SISA.Artillery.Catalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// A UGameInstanceSubsystem (Within=GameInstance) needs a GameInstance outer;
	// same pattern as the SISACore event-bus and Ammunition registry specs.
	UArtilleryCatalogSubsystem* NewTestCatalog()
	{
		UGameInstance* OwningGameInstance = NewObject<UGameInstance>();
		return NewObject<UArtilleryCatalogSubsystem>(OwningGameInstance);
	}

	FArtilleryPieceDefinition MakePiece(FName Id, EArtilleryPieceType Type)
	{
		FArtilleryPieceDefinition Piece;
		Piece.PieceId = Id;
		Piece.Type = Type;
		Piece.CaliberMillimeters = 155.0;
		Piece.MassKilograms = 5800.0;
		Piece.MinRangeMeters = 500.0;
		Piece.MaxRangeMeters = 14600.0;
		Piece.TraverseHalfArcDegrees = 25.0;
		Piece.MinElevationDegrees = 0.0;
		Piece.MaxElevationDegrees = 63.0;
		Piece.TraverseRateDegreesPerSecond = 5.0;
		Piece.ElevationRateDegreesPerSecond = 3.0;
		return Piece;
	}
}

void FArtilleryCatalogSpec::Define()
{
	Describe("Register and query", [this]()
	{
		It("registers a piece and finds it by id", [this]()
		{
			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			TestTrue(TEXT("Registered"), Catalog->RegisterDefinition(MakePiece(TEXT("M114A1"), EArtilleryPieceType::Howitzer)));

			bool bFound = false;
			const FArtilleryPieceDefinition Found = Catalog->FindById(TEXT("M114A1"), bFound);
			TestTrue(TEXT("Found"), bFound);
			TestEqual(TEXT("Id matches"), Found.PieceId, FName(TEXT("M114A1")));
		});

		It("reports not found for an unknown id", [this]()
		{
			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			bool bFound = true;
			Catalog->FindById(TEXT("does_not_exist"), bFound);
			TestFalse(TEXT("Not found"), bFound);
		});

		It("rejects an invalid piece", [this]()
		{
			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			FArtilleryPieceDefinition Bad = MakePiece(TEXT("Bad"), EArtilleryPieceType::Howitzer);
			Bad.MaxRangeMeters = 10.0;

			TestFalse(TEXT("Rejected"), Catalog->RegisterDefinition(Bad));
			TestEqual(TEXT("Catalog empty"), Catalog->Num(), 0);
		});

		It("filters by type and counts all", [this]()
		{
			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			Catalog->RegisterDefinition(MakePiece(TEXT("H1"), EArtilleryPieceType::Howitzer));
			Catalog->RegisterDefinition(MakePiece(TEXT("H2"), EArtilleryPieceType::Howitzer));
			Catalog->RegisterDefinition(MakePiece(TEXT("M1"), EArtilleryPieceType::Mortar));

			TestEqual(TEXT("Total"), Catalog->Num(), 3);
			TestEqual(TEXT("Howitzers"), Catalog->GetByType(EArtilleryPieceType::Howitzer).Num(), 2);
			TestEqual(TEXT("Mortars"), Catalog->GetByType(EArtilleryPieceType::Mortar).Num(), 1);
			TestEqual(TEXT("Rocket launchers"), Catalog->GetByType(EArtilleryPieceType::RocketLauncher).Num(), 0);
			TestEqual(TEXT("GetAll"), Catalog->GetAll().Num(), 3);
		});
	});

	Describe("LoadFromDataTable", [this]()
	{
		It("loads every valid row of a piece data table", [this]()
		{
			UDataTable* Table = NewObject<UDataTable>();
			Table->RowStruct = FArtilleryPieceDefinition::StaticStruct();
			Table->AddRow(TEXT("H1"), MakePiece(TEXT("H1"), EArtilleryPieceType::Howitzer));
			Table->AddRow(TEXT("M1"), MakePiece(TEXT("M1"), EArtilleryPieceType::Mortar));

			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			TestEqual(TEXT("Two rows added"), Catalog->LoadFromDataTable(Table), 2);
			TestEqual(TEXT("Catalog has two"), Catalog->Num(), 2);

			bool bFound = false;
			Catalog->FindById(TEXT("M1"), bFound);
			TestTrue(TEXT("Mortar row present"), bFound);
		});

		It("ignores a table with the wrong row struct", [this]()
		{
			UDataTable* Table = NewObject<UDataTable>();
			Table->RowStruct = FArtilleryAmmoStock::StaticStruct();

			UArtilleryCatalogSubsystem* Catalog = NewTestCatalog();
			TestEqual(TEXT("Nothing loaded"), Catalog->LoadFromDataTable(Table), 0);
			TestEqual(TEXT("Nothing loaded from null"), Catalog->LoadFromDataTable(nullptr), 0);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
