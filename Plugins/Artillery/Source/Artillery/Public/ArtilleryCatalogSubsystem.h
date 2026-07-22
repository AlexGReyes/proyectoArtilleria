#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArtilleryTypes.h"
#include "ArtilleryCatalogSubsystem.generated.h"

class UDataTable;

/**
 * The catalog of artillery piece *models* ("administración de piezas", static
 * half): definitions keyed by PieceId, populated from direct registration, a
 * Data Table, or the Asset Manager's "ArtilleryPiece" primary assets. The exact
 * counterpart of UMunitionRegistrySubsystem on the Ammunition side; live placed
 * pieces are a different concern and live in UArtilleryRosterSubsystem.
 */
UCLASS()
class ARTILLERY_API UArtilleryCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Adds or replaces a definition in the catalog. Invalid definitions are
	 * rejected (logged) and not added; returns true on success.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	bool RegisterDefinition(const FArtilleryPieceDefinition& Definition);

	/** Loads every valid row of a piece Data Table (RowStruct = FArtilleryPieceDefinition). Returns the number added. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	int32 LoadFromDataTable(const UDataTable* DataTable);

	/**
	 * Loads all "ArtilleryPiece" primary assets known to the Asset Manager.
	 * Requires the AssetManagerSettings entry and assets under
	 * /Game/ArtilleryPieces; returns 0 (no error) if none are configured/present.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	int32 LoadFromAssetManager();

	/** Finds a piece definition by id. Sets bFound and returns a default definition if not present. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	FArtilleryPieceDefinition FindById(FName PieceId, bool& bFound) const;

	/** Returns every definition in the catalog. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	TArray<FArtilleryPieceDefinition> GetAll() const;

	/** Returns every definition of the given type. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	TArray<FArtilleryPieceDefinition> GetByType(EArtilleryPieceType Type) const;

	/** Number of definitions currently in the catalog. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	int32 Num() const { return Pieces.Num(); }

	/** Removes all definitions from the catalog. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	void Clear() { Pieces.Empty(); }

private:
	/** Piece definitions keyed by PieceId. */
	TMap<FName, FArtilleryPieceDefinition> Pieces;
};
