#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArtilleryTypes.h"
#include "ArtilleryInventoryComponent.generated.h"

/**
 * The magazine of an artillery piece ("cantidad de municiones"): how many
 * rounds of each munition it currently holds. Deliberately knows only munition
 * ids — compatibility with the piece's caliber is the Actor's decision, and the
 * munition's own data lives in the Ammunition registry.
 */
UCLASS(ClassGroup = (SISA), meta = (BlueprintSpawnableComponent))
class ARTILLERY_API UArtilleryInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArtilleryInventoryComponent();

	/**
	 * Initial magazine contents, authored on the placed piece in the editor.
	 * Applied to the live stock in BeginPlay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery|Inventory")
	TArray<FArtilleryAmmoStock> InitialStock;

	/** Adds rounds of a munition. Non-positive amounts are ignored. Returns the new count. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Inventory")
	int32 AddRounds(FName MunitionId, int32 Count);

	/**
	 * Removes rounds of a munition. Consumption is all-or-nothing: if fewer than
	 * Count rounds are held, nothing is removed and the call returns false.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Inventory")
	bool ConsumeRounds(FName MunitionId, int32 Count = 1);

	/** Rounds currently held of a munition (0 if none). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Inventory")
	int32 GetRounds(FName MunitionId) const;

	/** True if at least Count rounds of the munition are available. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Inventory")
	bool HasRounds(FName MunitionId, int32 Count = 1) const;

	/** Total rounds held, across every munition. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Inventory")
	int32 GetTotalRounds() const;

	/** The full magazine as a list, for the right-hand properties panel. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Inventory")
	TArray<FArtilleryAmmoStock> GetStock() const;

	/** Empties the magazine. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Inventory")
	void Clear() { Stock.Empty(); }

protected:
	//~ Begin UActorComponent
	virtual void BeginPlay() override;
	//~ End UActorComponent

private:
	/** Live round counts keyed by munition id. Entries that reach zero are kept, so the UI can show "0 rounds" for a loaded type. */
	TMap<FName, int32> Stock;
};
