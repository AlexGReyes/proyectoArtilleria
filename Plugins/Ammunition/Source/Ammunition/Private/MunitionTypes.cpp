#include "MunitionTypes.h"
#include "SisaResult.h"

bool FMunitionDefinition::Validate(TArray<FSisaError>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	if (MunitionId.IsNone())
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.MissingId"), TEXT("Munition must have a non-empty MunitionId.")));
	}
	if (CaliberMillimeters <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidCaliber"), TEXT("Caliber must be greater than zero.")));
	}
	if (InitialSpeedMps <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidSpeed"), TEXT("Initial speed must be greater than zero.")));
	}
	if (MassKg <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidMass"), TEXT("Mass must be greater than zero.")));
	}
	if (BallisticCoefficient <= 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidBallisticCoefficient"), TEXT("Ballistic coefficient must be greater than zero.")));
	}
	if (EffectiveRadiusMeters < 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidEffectiveRadius"), TEXT("Effective radius cannot be negative.")));
	}
	if (LethalRadiusMeters < 0.0)
	{
		OutErrors.Add(FSisaError(TEXT("Ammunition.InvalidLethalRadius"), TEXT("Lethal radius cannot be negative.")));
	}
	if (LethalRadiusMeters > EffectiveRadiusMeters)
	{
		OutErrors.Add(FSisaError(
			TEXT("Ammunition.RadiiInconsistent"),
			TEXT("Lethal radius must not exceed the effective radius (the lethal zone is the inner area).")));
	}

	return OutErrors.Num() == InitialErrorCount;
}

FBallisticProjectileParams FMunitionDefinition::ToBallisticProjectileParams() const
{
	FBallisticProjectileParams Params;
	Params.InitialSpeedMps = InitialSpeedMps;
	Params.MassKg = MassKg;
	Params.BallisticCoefficient = BallisticCoefficient;
	return Params;
}

FMunitionAffectedArea FMunitionDefinition::MakeAffectedArea(const FGeoCoordinate& ImpactCenter) const
{
	FMunitionAffectedArea Area;
	Area.Center = ImpactCenter;
	Area.EffectiveRadiusMeters = EffectiveRadiusMeters;
	Area.LethalRadiusMeters = LethalRadiusMeters;
	return Area;
}
