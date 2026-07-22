#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ArtilleryFireHistoryComponent.h"
#include "ArtilleryInventoryComponent.h"

DEFINE_SPEC(
	FArtilleryInventorySpec,
	"SISA.Artillery.Inventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// UActorComponent has no Within class, so a transient-package outer is enough
	// to exercise the magazine/history logic without spawning an Actor or a world.
	UArtilleryInventoryComponent* NewTestInventory()
	{
		return NewObject<UArtilleryInventoryComponent>(GetTransientPackage());
	}

	UArtilleryFireHistoryComponent* NewTestHistory()
	{
		return NewObject<UArtilleryFireHistoryComponent>(GetTransientPackage());
	}
}

void FArtilleryInventorySpec::Define()
{
	Describe("Magazine", [this]()
	{
		It("adds and reports rounds", [this]()
		{
			UArtilleryInventoryComponent* Inventory = NewTestInventory();
			Inventory->AddRounds(TEXT("HE_M107"), 30);
			Inventory->AddRounds(TEXT("HE_M107"), 10);
			Inventory->AddRounds(TEXT("SMK_M116"), 5);

			TestEqual(TEXT("HE count"), Inventory->GetRounds(TEXT("HE_M107")), 40);
			TestEqual(TEXT("Smoke count"), Inventory->GetRounds(TEXT("SMK_M116")), 5);
			TestEqual(TEXT("Unknown munition"), Inventory->GetRounds(TEXT("nope")), 0);
			TestEqual(TEXT("Total"), Inventory->GetTotalRounds(), 45);
			TestEqual(TEXT("Two stock lines"), Inventory->GetStock().Num(), 2);
		});

		It("ignores non-positive additions", [this]()
		{
			UArtilleryInventoryComponent* Inventory = NewTestInventory();
			Inventory->AddRounds(TEXT("HE_M107"), 0);
			Inventory->AddRounds(TEXT("HE_M107"), -5);

			TestEqual(TEXT("Nothing added"), Inventory->GetTotalRounds(), 0);
		});

		It("consumes rounds all-or-nothing", [this]()
		{
			UArtilleryInventoryComponent* Inventory = NewTestInventory();
			Inventory->AddRounds(TEXT("HE_M107"), 3);

			TestTrue(TEXT("Consumes one"), Inventory->ConsumeRounds(TEXT("HE_M107"), 1));
			TestEqual(TEXT("Two left"), Inventory->GetRounds(TEXT("HE_M107")), 2);

			TestFalse(TEXT("Cannot consume five"), Inventory->ConsumeRounds(TEXT("HE_M107"), 5));
			TestEqual(TEXT("Still two left"), Inventory->GetRounds(TEXT("HE_M107")), 2);

			TestFalse(TEXT("Cannot consume from an unknown munition"), Inventory->ConsumeRounds(TEXT("nope"), 1));
		});

		It("reports availability", [this]()
		{
			UArtilleryInventoryComponent* Inventory = NewTestInventory();
			Inventory->AddRounds(TEXT("HE_M107"), 2);

			TestTrue(TEXT("Has one"), Inventory->HasRounds(TEXT("HE_M107"), 1));
			TestTrue(TEXT("Has two"), Inventory->HasRounds(TEXT("HE_M107"), 2));
			TestFalse(TEXT("Does not have three"), Inventory->HasRounds(TEXT("HE_M107"), 3));
		});
	});

	Describe("Fire history", [this]()
	{
		It("stamps records with sequential shot numbers", [this]()
		{
			UArtilleryFireHistoryComponent* History = NewTestHistory();

			FArtilleryFireRecord Record;
			Record.MunitionId = TEXT("HE_M107");

			const FArtilleryFireRecord First = History->AddRecord(Record);
			const FArtilleryFireRecord Second = History->AddRecord(Record);

			TestEqual(TEXT("First shot"), First.ShotNumber, 1);
			TestEqual(TEXT("Second shot"), Second.ShotNumber, 2);
			TestEqual(TEXT("Two records"), History->Num(), 2);
			TestEqual(TEXT("Lifetime count"), History->GetTotalShotsFired(), 2);
			TestTrue(TEXT("Timestamped"), First.TimestampUtc != FDateTime());
		});

		It("resolves the impact of a recorded shot", [this]()
		{
			UArtilleryFireHistoryComponent* History = NewTestHistory();

			FArtilleryFireRecord Record;
			Record.MunitionId = TEXT("HE_M107");
			const FArtilleryFireRecord Stored = History->AddRecord(Record);

			TestFalse(TEXT("Unresolved on entry"), Stored.bImpactResolved);
			TestTrue(TEXT("Resolved"), History->ResolveImpact(Stored.ShotNumber, FGeoCoordinate(4.6, -74.0, 2600.0), 8123.0, 42.5));
			TestFalse(TEXT("Unknown shot number"), History->ResolveImpact(999, FGeoCoordinate(), 0.0, 0.0));

			bool bFound = false;
			const FArtilleryFireRecord Last = History->GetLastRecord(bFound);
			TestTrue(TEXT("Found"), bFound);
			TestTrue(TEXT("Impact flagged"), Last.bImpactResolved);
			TestEqual(TEXT("Range"), Last.RangeMeters, 8123.0, 1e-9);
			TestEqual(TEXT("Time of flight"), Last.TimeOfFlightSeconds, 42.5, 1e-9);
			TestEqual(TEXT("Impact latitude"), Last.ImpactPosition.Latitude, 4.6, 1e-9);
		});

		It("trims the log to MaxRecords while keeping the lifetime count", [this]()
		{
			UArtilleryFireHistoryComponent* History = NewTestHistory();
			History->MaxRecords = 3;

			FArtilleryFireRecord Record;
			Record.MunitionId = TEXT("HE_M107");
			for (int32 Shot = 0; Shot < 5; ++Shot)
			{
				History->AddRecord(Record);
			}

			TestEqual(TEXT("Log trimmed"), History->Num(), 3);
			TestEqual(TEXT("Lifetime count intact"), History->GetTotalShotsFired(), 5);

			const TArray<FArtilleryFireRecord> Records = History->GetRecords();
			TestEqual(TEXT("Oldest kept is shot 3"), Records[0].ShotNumber, 3);
			TestEqual(TEXT("Newest is shot 5"), Records.Last().ShotNumber, 5);
		});

		It("reports an empty log", [this]()
		{
			UArtilleryFireHistoryComponent* History = NewTestHistory();
			bool bFound = true;
			History->GetLastRecord(bFound);
			TestFalse(TEXT("Nothing to report"), bFound);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
