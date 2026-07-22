#include "ArtilleryLaying.h"
#include "SisaAngle.h"

// The compass math itself lives in SISACore (SisaAngle.h) so Hardware's
// simulated mount slews with exactly the same rules; these two stay as the
// artillery-facing names of those operations.
double FArtilleryLayingSolver::NormalizeAzimuth(double AzimuthDegrees)
{
	return SisaAngle::Normalize360(AzimuthDegrees);
}

double FArtilleryLayingSolver::ShortestAzimuthDelta(double FromDegrees, double ToDegrees)
{
	return SisaAngle::ShortestDelta(FromDegrees, ToDegrees);
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

	Stepped.CurrentAzimuthDegrees = SisaAngle::StepTowards(
		State.CurrentAzimuthDegrees,
		State.TargetAzimuthDegrees,
		Definition.TraverseRateDegreesPerSecond * DeltaSeconds);

	Stepped.CurrentElevationDegrees = SisaAngle::StepTowardsLinear(
		State.CurrentElevationDegrees,
		State.TargetElevationDegrees,
		Definition.ElevationRateDegreesPerSecond * DeltaSeconds);

	return Stepped;
}

bool FArtilleryLayingSolver::IsOnTarget(const FArtilleryLayingState& State, double ToleranceDegrees)
{
	const double AzimuthError = FMath::Abs(ShortestAzimuthDelta(State.CurrentAzimuthDegrees, State.TargetAzimuthDegrees));
	const double ElevationError = FMath::Abs(State.TargetElevationDegrees - State.CurrentElevationDegrees);
	return AzimuthError <= ToleranceDegrees && ElevationError <= ToleranceDegrees;
}
