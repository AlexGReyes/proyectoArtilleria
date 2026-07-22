#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArtilleryLaying.h"
#include "ArtilleryTypes.h"
#include "BallisticTypes.h"
#include "ArtilleryLayingComponent.generated.h"

/**
 * Drives a piece's tube toward an ordered azimuth/elevation at the mechanical
 * rates declared by its definition. All the math lives in FArtilleryLayingSolver;
 * this component only supplies the definition, the state and delta time, and
 * stops ticking once the tube is on target so idle pieces cost nothing.
 */
UCLASS(ClassGroup = (SISA), meta = (BlueprintSpawnableComponent))
class ARTILLERY_API UArtilleryLayingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArtilleryLayingComponent();

	/** Sets the piece definition whose limits and rates govern this component. */
	void SetDefinition(const FArtilleryPieceDefinition& InDefinition);

	/** Reference azimuth the carriage is laid on; the traverse arc is centered here. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Laying")
	void SetMountAzimuth(double MountAzimuthDegrees);

	/**
	 * Orders a new laying. The values are clamped to the traverse arc and the
	 * elevation limits, so a request outside the piece's envelope lays it as
	 * close as the mount allows rather than failing. Returns false when clamping
	 * had to alter the request, so the UI can warn the operator.
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Laying")
	bool OrderLaying(double AzimuthDegrees, double ElevationDegrees);

	/** Snaps the tube to the ordered laying immediately, bypassing the slew rates (scenario setup, tests). */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Laying")
	void SnapToOrderedLaying();

	/** Advances the slew by DeltaSeconds. Called from Tick; exposed so tests can drive it deterministically. */
	void StepLaying(double DeltaSeconds);

	/** Current tube azimuth, degrees clockwise from true North. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Laying")
	double GetCurrentAzimuth() const { return State.CurrentAzimuthDegrees; }

	/** Current tube elevation above the local horizon, in degrees. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Laying")
	double GetCurrentElevation() const { return State.CurrentElevationDegrees; }

	/** True once both axes have reached the ordered laying. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Laying")
	bool IsOnTarget() const { return FArtilleryLayingSolver::IsOnTarget(State); }

	/** The full laying state (current + ordered + mount reference). */
	const FArtilleryLayingState& GetState() const { return State; }

	/** The current laying as ballistic solver input. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Laying")
	FBallisticLaunchParams ToBallisticLaunchParams() const;

protected:
	//~ Begin UActorComponent
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent

private:
	/** Limits and rates of the piece this component lays. Set by the owning Actor. */
	UPROPERTY()
	FArtilleryPieceDefinition Definition;

	/** Current and ordered angles. */
	UPROPERTY()
	FArtilleryLayingState State;
};
