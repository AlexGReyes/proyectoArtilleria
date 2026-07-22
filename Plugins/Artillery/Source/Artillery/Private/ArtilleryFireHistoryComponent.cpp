#include "ArtilleryFireHistoryComponent.h"

UArtilleryFireHistoryComponent::UArtilleryFireHistoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FArtilleryFireRecord UArtilleryFireHistoryComponent::AddRecord(const FArtilleryFireRecord& Record)
{
	FArtilleryFireRecord Stamped = Record;
	Stamped.ShotNumber = ++LastShotNumber;
	Stamped.TimestampUtc = FDateTime::UtcNow();

	Records.Add(Stamped);

	if (MaxRecords > 0 && Records.Num() > MaxRecords)
	{
		Records.RemoveAt(0, Records.Num() - MaxRecords, EAllowShrinking::No);
	}

	return Stamped;
}

bool UArtilleryFireHistoryComponent::ResolveImpact(int32 ShotNumber, const FGeoCoordinate& ImpactPosition, double RangeMeters, double TimeOfFlightSeconds)
{
	FArtilleryFireRecord* Record = Records.FindByPredicate(
		[ShotNumber](const FArtilleryFireRecord& Candidate) { return Candidate.ShotNumber == ShotNumber; });

	if (!Record)
	{
		return false;
	}

	Record->ImpactPosition = ImpactPosition;
	Record->RangeMeters = RangeMeters;
	Record->TimeOfFlightSeconds = TimeOfFlightSeconds;
	Record->bImpactResolved = true;
	return true;
}

FArtilleryFireRecord UArtilleryFireHistoryComponent::GetLastRecord(bool& bFound) const
{
	if (Records.IsEmpty())
	{
		bFound = false;
		return FArtilleryFireRecord();
	}

	bFound = true;
	return Records.Last();
}
