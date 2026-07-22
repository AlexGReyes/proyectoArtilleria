#include "ArtilleryTypes.h"
#include "SisaResult.h"

bool FArtilleryPieceDefinition::Validate(TArray<FSisaError>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (PieceId.IsNone())
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.MissingId"), TEXT("Artillery piece must have a non-empty PieceId.")));
	}
	if (CaliberMillimeters <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.InvalidCaliber"), TEXT("Caliber must be greater than zero.")));
	}
	if (MassKilograms <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.InvalidMass"), TEXT("Mass must be greater than zero.")));
	}
	if (MinRangeMeters < 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.InvalidMinRange"), TEXT("Minimum range cannot be negative.")));
	}
	if (MaxRangeMeters <= MinRangeMeters)
	{
		OutErrors.Add(FSisaError(
			TEXT("Artillery.RangesInconsistent"),
			TEXT("Maximum range must be greater than the minimum range.")));
	}
	if (TraverseHalfArcDegrees < 0.0 || TraverseHalfArcDegrees > 180.0)
	{
		OutErrors.Add(FSisaError(
			TEXT("Artillery.InvalidTraverseArc"),
			TEXT("Traverse half-arc must be within [0, 180] degrees (180 = full traverse).")));
	}
	if (MaxElevationDegrees <= MinElevationDegrees)
	{
		OutErrors.Add(FSisaError(
			TEXT("Artillery.ElevationLimitsInconsistent"),
			TEXT("Maximum elevation must be greater than the minimum elevation.")));
	}
	if (MinElevationDegrees < -90.0 || MaxElevationDegrees > 90.0)
	{
		OutErrors.Add(FSisaError(
			TEXT("Artillery.ElevationOutOfRange"),
			TEXT("Elevation limits must stay within [-90, 90] degrees.")));
	}
	if (TraverseRateDegreesPerSecond <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.InvalidTraverseRate"), TEXT("Traverse rate must be greater than zero.")));
	}
	if (ElevationRateDegreesPerSecond <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Artillery.InvalidElevationRate"), TEXT("Elevation rate must be greater than zero.")));
	}

	return OutErrors.Num() == InitialErrorCount;
}

bool FArtilleryPieceDefinition::IsMunitionCompatible(FName MunitionId, double MunitionCaliberMillimeters) const
{
	// Caliber is the physical constraint and always applies; the explicit list,
	// when authored, narrows it further (e.g. a piece cleared only for HE and smoke).
	if (!FMath::IsNearlyEqual(MunitionCaliberMillimeters, CaliberMillimeters, 0.5))
	{
		return false;
	}

	return CompatibleMunitionIds.IsEmpty() || CompatibleMunitionIds.Contains(MunitionId);
}
