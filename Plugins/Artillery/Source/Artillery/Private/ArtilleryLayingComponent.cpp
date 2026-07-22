#include "ArtilleryLayingComponent.h"

UArtilleryLayingComponent::UArtilleryLayingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Nothing to slew until a laying is ordered; OrderLaying re-enables the tick.
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UArtilleryLayingComponent::SetDefinition(const FArtilleryPieceDefinition& InDefinition)
{
	Definition = InDefinition;

	// Re-apply the current order so it satisfies the new limits.
	State = FArtilleryLayingSolver::OrderLaying(State, Definition, State.TargetAzimuthDegrees, State.TargetElevationDegrees);
}

void UArtilleryLayingComponent::SetMountAzimuth(double MountAzimuthDegrees)
{
	State.MountAzimuthDegrees = FArtilleryLayingSolver::NormalizeAzimuth(MountAzimuthDegrees);
	State = FArtilleryLayingSolver::OrderLaying(State, Definition, State.TargetAzimuthDegrees, State.TargetElevationDegrees);
}

bool UArtilleryLayingComponent::OrderLaying(double AzimuthDegrees, double ElevationDegrees)
{
	State = FArtilleryLayingSolver::OrderLaying(State, Definition, AzimuthDegrees, ElevationDegrees);

	SetComponentTickEnabled(true);

	const bool bAzimuthHonored = FMath::IsNearlyEqual(
		FArtilleryLayingSolver::ShortestAzimuthDelta(AzimuthDegrees, State.TargetAzimuthDegrees), 0.0, UE_KINDA_SMALL_NUMBER);
	const bool bElevationHonored = FMath::IsNearlyEqual(ElevationDegrees, State.TargetElevationDegrees, UE_KINDA_SMALL_NUMBER);
	return bAzimuthHonored && bElevationHonored;
}

void UArtilleryLayingComponent::SnapToOrderedLaying()
{
	State.CurrentAzimuthDegrees = State.TargetAzimuthDegrees;
	State.CurrentElevationDegrees = State.TargetElevationDegrees;
	SetComponentTickEnabled(false);
}

void UArtilleryLayingComponent::StepLaying(double DeltaSeconds)
{
	State = FArtilleryLayingSolver::Step(State, Definition, DeltaSeconds);
}

void UArtilleryLayingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	StepLaying(DeltaTime);

	if (IsOnTarget())
	{
		// Idle pieces must not cost a tick; the next order turns this back on.
		SetComponentTickEnabled(false);
	}
}

FBallisticLaunchParams UArtilleryLayingComponent::ToBallisticLaunchParams() const
{
	FBallisticLaunchParams Params;
	Params.AzimuthDegrees = State.CurrentAzimuthDegrees;
	Params.ElevationDegrees = State.CurrentElevationDegrees;
	return Params;
}
