#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArtilleryTypes.h"
#include "ArtilleryFireHistoryComponent.generated.h"

/**
 * The fire log of a piece ("historial de disparos"), feeding the bottom panel.
 * Records are appended at the moment of firing with the laying and munition
 * used; the impact fields are filled in later via ResolveImpact() once the
 * ballistic solve completes, since the solve is asynchronous.
 */
UCLASS(ClassGroup = (SISA), meta = (BlueprintSpawnableComponent))
class ARTILLERY_API UArtilleryFireHistoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArtilleryFireHistoryComponent();

	/**
	 * Maximum number of records kept. Older records are dropped once the log
	 * grows past this. Set to 0 for an unbounded history.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery|History", meta = (ClampMin = "0"))
	int32 MaxRecords = 500;

	/**
	 * Appends a shot to the log, stamping it with the next shot number and the
	 * current UTC time. Returns the stored record (with those fields filled).
	 */
	FArtilleryFireRecord AddRecord(const FArtilleryFireRecord& Record);

	/**
	 * Fills in the impact of a previously recorded shot. Returns false if no
	 * record with that shot number is still in the log (it may have been trimmed).
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|History")
	bool ResolveImpact(int32 ShotNumber, const FGeoCoordinate& ImpactPosition, double RangeMeters, double TimeOfFlightSeconds);

	/** Every record still in the log, oldest first. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|History")
	TArray<FArtilleryFireRecord> GetRecords() const { return Records; }

	/** The most recent record. Sets bFound to false (and returns a default) when the log is empty. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|History")
	FArtilleryFireRecord GetLastRecord(bool& bFound) const;

	/** Number of records currently in the log (not the lifetime shot count — see GetTotalShotsFired). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|History")
	int32 Num() const { return Records.Num(); }

	/** Lifetime rounds fired by this piece, including records already trimmed from the log. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|History")
	int32 GetTotalShotsFired() const { return LastShotNumber; }

	/** Clears the log. The lifetime shot counter is preserved so shot numbers stay unique. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|History")
	void Clear() { Records.Empty(); }

private:
	/** Records in chronological order. */
	UPROPERTY()
	TArray<FArtilleryFireRecord> Records;

	/** Last assigned shot number; also the lifetime shot count. */
	int32 LastShotNumber = 0;
};
