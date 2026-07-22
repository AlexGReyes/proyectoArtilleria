#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArtilleryTypes.h"
#include "ArtilleryPieceDataAsset.generated.h"

/**
 * Per-asset (Primary Data Asset) authoring of an artillery piece model: one
 * asset = one piece definition. Discoverable through the Asset Manager as
 * primary asset type "ArtilleryPiece" (see the AssetManagerSettings entry in
 * Config/DefaultGame.ini and the /Game/ArtilleryPieces content directory).
 * Alternatively, pieces can be authored as rows of a Data Table using
 * FArtilleryPieceDefinition directly.
 */
UCLASS(BlueprintType)
class ARTILLERY_API UArtilleryPieceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The artillery piece this asset defines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery", meta = (ShowOnlyInnerProperties))
	FArtilleryPieceDefinition Definition;

	/** Exposes this asset under the "ArtilleryPiece" primary asset type, keyed by its FName. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ArtilleryPiece"), GetFName());
	}
};
