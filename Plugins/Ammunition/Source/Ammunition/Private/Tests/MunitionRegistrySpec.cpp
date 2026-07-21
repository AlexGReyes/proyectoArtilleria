#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "MunitionRegistrySubsystem.h"

DEFINE_SPEC(
	FMunitionRegistrySpec,
	"SISA.Ammunition.Registry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// The registry is a UGameInstanceSubsystem (Within=GameInstance), so tests
	// construct it with a UGameInstance outer, matching the SISACore event-bus test.
	UMunitionRegistrySubsystem* NewTestRegistry()
	{
		UGameInstance* OwningGameInstance = NewObject<UGameInstance>();
		return NewObject<UMunitionRegistrySubsystem>(OwningGameInstance);
	}

	FMunitionDefinition MakeMunition(FName Id, EMunitionType Type)
	{
		FMunitionDefinition M;
		M.MunitionId = Id;
		M.Type = Type;
		M.CaliberMillimeters = 155.0;
		M.InitialSpeedMps = 684.0;
		M.MassKg = 43.2;
		M.BallisticCoefficient = 3500.0;
		M.EffectiveRadiusMeters = 50.0;
		M.LethalRadiusMeters = 25.0;
		return M;
	}
}

void FMunitionRegistrySpec::Define()
{
	Describe("Register and query", [this]()
	{
		It("registers a munition and finds it by id", [this]()
		{
			UMunitionRegistrySubsystem* Registry = NewTestRegistry();
			TestTrue(TEXT("Registered"), Registry->RegisterDefinition(MakeMunition(TEXT("A_HE"), EMunitionType::HighExplosive)));

			bool bFound = false;
			const FMunitionDefinition Found = Registry->FindById(TEXT("A_HE"), bFound);
			TestTrue(TEXT("Found"), bFound);
			TestEqual(TEXT("Id matches"), Found.MunitionId, FName(TEXT("A_HE")));
		});

		It("reports not found for an unknown id", [this]()
		{
			UMunitionRegistrySubsystem* Registry = NewTestRegistry();
			bool bFound = true;
			Registry->FindById(TEXT("does_not_exist"), bFound);
			TestFalse(TEXT("Not found"), bFound);
		});

		It("rejects an invalid munition", [this]()
		{
			UMunitionRegistrySubsystem* Registry = NewTestRegistry();
			FMunitionDefinition Bad = MakeMunition(TEXT("Bad"), EMunitionType::HighExplosive);
			Bad.MassKg = -1.0;
			TestFalse(TEXT("Rejected"), Registry->RegisterDefinition(Bad));
			TestEqual(TEXT("Registry empty"), Registry->Num(), 0);
		});

		It("filters by type and counts all", [this]()
		{
			UMunitionRegistrySubsystem* Registry = NewTestRegistry();
			Registry->RegisterDefinition(MakeMunition(TEXT("HE1"), EMunitionType::HighExplosive));
			Registry->RegisterDefinition(MakeMunition(TEXT("HE2"), EMunitionType::HighExplosive));
			Registry->RegisterDefinition(MakeMunition(TEXT("SMK1"), EMunitionType::Smoke));

			TestEqual(TEXT("Total"), Registry->Num(), 3);
			TestEqual(TEXT("HE count"), Registry->GetByType(EMunitionType::HighExplosive).Num(), 2);
			TestEqual(TEXT("Smoke count"), Registry->GetByType(EMunitionType::Smoke).Num(), 1);
			TestEqual(TEXT("Illumination count"), Registry->GetByType(EMunitionType::Illumination).Num(), 0);
		});
	});

	Describe("LoadFromDataTable", [this]()
	{
		It("loads every valid row of a munition data table", [this]()
		{
			UDataTable* Table = NewObject<UDataTable>();
			Table->RowStruct = FMunitionDefinition::StaticStruct();

			const FMunitionDefinition HE = MakeMunition(TEXT("HE1"), EMunitionType::HighExplosive);
			const FMunitionDefinition SMK = MakeMunition(TEXT("SMK1"), EMunitionType::Smoke);
			Table->AddRow(TEXT("HE1"), HE);
			Table->AddRow(TEXT("SMK1"), SMK);

			UMunitionRegistrySubsystem* Registry = NewTestRegistry();
			const int32 Added = Registry->LoadFromDataTable(Table);

			TestEqual(TEXT("Two rows added"), Added, 2);
			TestEqual(TEXT("Registry has two"), Registry->Num(), 2);

			bool bFound = false;
			Registry->FindById(TEXT("SMK1"), bFound);
			TestTrue(TEXT("Smoke row present"), bFound);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
