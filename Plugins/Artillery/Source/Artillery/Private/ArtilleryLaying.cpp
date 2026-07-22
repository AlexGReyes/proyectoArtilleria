#include "ArtilleryLaying.h"

double FArtilleryLayingSolver::NormalizeAzimuth(double AzimuthDegrees)
{
	const double Wrapped = FMath::Fmod(AzimuthDegrees, 360.0);
	return Wrapped < 0.0 ? Wrapped + 360.0 : Wrapped;
}

double FArtilleryLayingSolver::ShortestAzimuthDelta(double FromDegrees, double ToDegrees)
{
	double Delta = NormalizeAzimuth(ToDegrees) - NormalizeAzimuth(FromDegrees);
	if (Delta > 180.0)
	{
		Delta -= 360.0;
	}
	else if (Delta <= -180.0)
	{
		Delta += 360.0;
	}
	return Delta;
}

double FArtilleryLayingSolver::ClampAzimuthToArc(double DesiredAzimuthDegrees, double MountAzimuthDegrees, double TraverseHalfArcDegrees)
{
	const double HalfArc = FMath::Clamp(TraverseHalfArcDegrees, 0.0, 180.0);
	if (HalfArc >= 180.0)
	{
		return NormalizeAzimuth(DesiredAzimuthDegrees);
	}

	const double Offset = ShortestAzimuthDelta(MountAzimuthDegrees, DesiredAzimuthDegrees);
	const double ClampedOffset = FMath::Clamp(Offset, -HalfArc, HalfArc);
	return NormalizeAzimuth(MountAzimuthDegrees + ClampedOffset);
}

bool FArtilleryLayingSolver::IsAzimuthWithinArc(double AzimuthDegrees, double MountAzimuthDegrees, double TraverseHalfArcDegrees)
{
	const double HalfArc = FMath::Clamp(TraverseHalfArcDegrees, 0.0, 180.0);
	if (HalfArc >= 180.0)
	{
		return true;
	}

	const double Offset = ShortestAzimuthDelta(MountAzimuthDegrees, AzimuthDegrees);
	return FMath::Abs(Offset) <= HalfArc + UE_KINDA_SMALL_NUMBER;
}

double FArtilleryLayingSolver::ClampElevation(double DesiredElevationDegrees, const FArtilleryPieceDefinition& Definition)
{
	return FMath::Clamp(DesiredElevationDegrees, Definition.MinElevationDegrees, Definition.MaxElevationDegrees);
}

FArtilleryLayingState FArtilleryLayingSolver::OrderLaying(
	const FArtilleryLayingState& State,
	const FArtilleryPieceDefinition& Definition,
	double DesiredAzimuthDegrees,
	double DesiredElevationDegrees)
{
	FArtilleryLayingState Ordered = State;
	Ordered.TargetAzimuthDegrees = ClampAzimuthToArc(DesiredAzimuthDegrees, State.MountAzimuthDegrees, Definition.TraverseHalfArcDegrees);
	Ordered.TargetElevationDegrees = ClampElevation(DesiredElevationDegrees, Definition);
	return Ordered;
}

FArtilleryLayingState FArtilleryLayingSolver::Step(
	const FArtilleryLayingState& State,
	const FArtilleryPieceDefinition& Definition,
	double DeltaSeconds)
{
	FArtilleryLayingState Stepped = State;
	if (DeltaSeconds <= 0.0)
	{
		return Stepped;
	}

	const double AzimuthDelta = ShortestAzimuthDelta(State.CurrentAzimuthDegrees, State.TargetAzimuthDegrees);
	const double MaxAzimuthStep = Definition.TraverseRateDegreesPerSecond * DeltaSeconds;
	const double AzimuthStep = FMath::Clamp(AzimuthDelta, -MaxAzimuthStep, MaxAzimuthStep);
	Stepped.CurrentAzimuthDegrees = NormalizeAzimuth(State.CurrentAzimuthDegrees + AzimuthStep);

	const double ElevationDelta = State.TargetElevationDegrees - State.CurrentElevationDegrees;
	const double MaxElevationStep = Definition.ElevationRateDegreesPerSecond * DeltaSeconds;
	Stepped.CurrentElevationDegrees = State.CurrentElevationDegrees + FMath::Clamp(ElevationDelta, -MaxElevationStep, MaxElevationStep);

	return Stepped;
}

bool FArtilleryLayingSolver::IsOnTarget(const FArtilleryLayingState& State, double ToleranceDegrees)
{
	const double AzimuthError = FMath::Abs(ShortestAzimuthDelta(State.CurrentAzimuthDegrees, State.TargetAzimuthDegrees));
	const double ElevationError = FMath::Abs(State.TargetElevationDegrees - State.CurrentElevationDegrees);
	return AzimuthError <= ToleranceDegrees && ElevationError <= ToleranceDegrees;
}
