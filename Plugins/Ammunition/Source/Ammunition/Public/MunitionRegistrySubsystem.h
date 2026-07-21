#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MunitionTypes.h"
#include "MunitionRegistrySubsystem.generated.h"

class UDataTable;

/**
 * The munition catalog access point ("administración de municiones") for
 * Artillery and Simulation. Holds munition definitions in memory keyed by
 * MunitionId, populated from any combination of: direct registration, a Data
 * Table, or the Asset Manager's "Munition" primary assets. Pure catalog — it
 * carries no ballistic/business logic.
 */
UCLASS()
class AMMUNITION_API UMunitionRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Adds or replaces a definition in the registry. Invalid definitions are
	 * rejected (logged) and not added; returns true on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	bool RegisterDefinition(const FMunitionDefinition& Definition);

	/** Loads every row of a munition Data Table (RowStruct = FMunitionDefinition). Returns the number added. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	int32 LoadFromDataTable(const UDataTable* DataTable);

	/**
	 * Loads all "Munition" primary assets known to the Asset Manager. Requires
	 * the AssetManagerSettings entry and munition assets under /Game/Munitions;
	 * returns 0 (no error) if none are configured/present. Returns the number added.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	int32 LoadFromAssetManager();

	/** Finds a munition by id. Sets bFound and returns a default definition if not present. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	FMunitionDefinition FindById(FName MunitionId, bool& bFound) const;

	/** Returns every munition in the registry. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	TArray<FMunitionDefinition> GetAll() const;

	/** Returns every munition of the given type. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	TArray<FMunitionDefinition> GetByType(EMunitionType Type) const;

	/** Number of munitions currently in the registry. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	int32 Num() const { return Munitions.Num(); }

	/** Removes all munitions from the registry. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Ammunition")
	void Clear() { Munitions.Empty(); }

private:
	/** Munition definitions keyed by MunitionId. */
	TMap<FName, FMunitionDefinition> Munitions;
};
