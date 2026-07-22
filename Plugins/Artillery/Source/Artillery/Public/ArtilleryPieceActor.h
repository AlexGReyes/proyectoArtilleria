#pragma once

#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "SisaGeoCoordinate.h"
#include "SisaResult.h"
#include "ArtilleryTypes.h"
#include "ArtilleryPieceActor.generated.h"

class UArtilleryFireHistoryComponent;
class UArtilleryInventoryComponent;
class UArtilleryLayingComponent;
class UArtilleryPieceDataAsset;

/**
 * A single artillery piece placed in the world: the Infrastructure-layer
 * embodiment of a FArtilleryPieceDefinition. It owns the piece's laying,
 * magazine and fire history as components, keeps its authoritative position in
 * geographic (WGS84) coordinates, and registers itself with the roster so the
 * C4ISR panels can list and select it.
 *
 * It deliberately does NOT run the ballistic solve: firing consumes a round,
 * records the shot and publishes FArtilleryPieceFiredEvent on the SISACore
 * event bus; Simulation subscribes, solves the trajectory asynchronously, and
 * calls back into the fire history with the impact.
 */
UCLASS()
class ARTILLERY_API AArtilleryPieceActor : public AActor
{
	GENERATED_BODY()

public:
	AArtilleryPieceActor();

	/**
	 * Unique runtime id of this placed piece (distinct from PieceId, which names
	 * the model). Left empty, a stable id is derived from the Actor name at
	 * BeginPlay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery")
	FName InstanceId;

	/**
	 * Optional per-asset definition. When set, it wins over CatalogPieceId; when
	 * both are empty the piece keeps the inline Definition below.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery")
	TObjectPtr<UArtilleryPieceDataAsset> PieceDataAsset;

	/** Optional catalog id, resolved through UArtilleryCatalogSubsystem at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery")
	FName CatalogPieceId;

	/** Geographic position the piece is emplaced at ("colocación por coordenada"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery")
	FGeoCoordinate GeoPosition;

	/** Operational state shown in the pieces panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SISA|Artillery")
	EArtilleryOperationalStatus OperationalStatus = EArtilleryOperationalStatus::Operational;

	/** The resolved definition governing this piece. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery")
	const FArtilleryPieceDefinition& GetDefinition() const { return Definition; }

	/** Replaces the definition and pushes its limits/rates into the laying component. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	void SetDefinition(const FArtilleryPieceDefinition& InDefinition);

	/**
	 * Emplaces the piece at a geographic coordinate, moving the Actor to the
	 * matching world position via the GIS coordinate service. Returns false (and
	 * still stores GeoPosition) if no georeference is available in this world,
	 * which is the case in headless tests.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	bool SetGeoPosition(const FGeoCoordinate& InGeoPosition);

	/** Changes the operational state. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery")
	void SetOperationalStatus(EArtilleryOperationalStatus NewStatus);

	/** True when the piece is in a state that permits firing at all. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery")
	bool IsOperational() const;

	/**
	 * Checks every precondition for firing the given munition: operational state,
	 * munition compatibility (caliber and clearance list), rounds available, and
	 * the tube being on its ordered laying. Returns an error describing the first
	 * failure so the UI can show why the fire command is unavailable.
	 */
	TSisaResult<bool> CanFire(FName MunitionId) const;

	/** Blueprint-facing wrapper around CanFire that reports the failure as a plain error struct. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery", meta = (DisplayName = "Can Fire"))
	bool CanFireWithReason(FName MunitionId, FSisaError& OutError) const;

	/**
	 * Fires one round: consumes it from the magazine, appends a fire record, and
	 * publishes FArtilleryPieceFiredEvent on the event bus. Fails (without
	 * consuming anything) if CanFire() fails.
	 */
	TSisaResult<FArtilleryFireRecord> Fire(FName MunitionId);

	/** Blueprint-facing wrapper around Fire. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery", meta = (DisplayName = "Fire"))
	bool FireRound(FName MunitionId, FArtilleryFireRecord& OutRecord, FSisaError& OutError);

	/** Laying (azimuth/elevation) of this piece. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery")
	UArtilleryLayingComponent* GetLaying() const { return LayingComponent; }

	/** Magazine of this piece. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery")
	UArtilleryInventoryComponent* GetInventory() const { return InventoryComponent; }

	/** Fire history of this piece. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery")
	UArtilleryFireHistoryComponent* GetFireHistory() const { return FireHistoryComponent; }

protected:
	//~ Begin AActor
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor

private:
	/** Resolves PieceDataAsset / CatalogPieceId into Definition. */
	void ResolveDefinition();

	/** Looks up the caliber of a munition in the Ammunition registry. Returns false if unknown. */
	bool TryGetMunitionCaliber(FName MunitionId, double& OutCaliberMillimeters) const;

	/** The piece definition in force, resolved at BeginPlay or set explicitly. */
	UPROPERTY(EditAnywhere, Category = "SISA|Artillery", meta = (AllowPrivateAccess = "true", ShowOnlyInnerProperties))
	FArtilleryPieceDefinition Definition;

	UPROPERTY(VisibleAnywhere, Category = "SISA|Artillery", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArtilleryLayingComponent> LayingComponent;

	UPROPERTY(VisibleAnywhere, Category = "SISA|Artillery", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArtilleryInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, Category = "SISA|Artillery", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArtilleryFireHistoryComponent> FireHistoryComponent;
};
