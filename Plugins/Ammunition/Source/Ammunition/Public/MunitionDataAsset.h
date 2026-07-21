#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MunitionTypes.h"
#include "MunitionDataAsset.generated.h"

/**
 * Per-asset (Primary Data Asset) authoring of a munition: one asset = one
 * munition. Discoverable through the Asset Manager as primary asset type
 * "Munition" (see the AssetManagerSettings entry in Config/DefaultGame.ini and
 * the /Game/Munitions content directory). Alternatively, munitions can be
 * authored as rows of a Data Table using FMunitionDefinition directly.
 */
UCLASS(BlueprintType)
class AMMUNITION_API UMunitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The munition this asset defines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Ammunition", meta = (ShowOnlyInnerProperties))
	FMunitionDefinition Definition;

	/** Exposes this asset under the "Munition" primary asset type, keyed by its FName. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Munition"), GetFName());
	}
};
