#include "ArtilleryInventoryComponent.h"

UArtilleryInventoryComponent::UArtilleryInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArtilleryInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const FArtilleryAmmoStock& Entry : InitialStock)
	{
		AddRounds(Entry.MunitionId, Entry.Rounds);
	}
}

int32 UArtilleryInventoryComponent::AddRounds(FName MunitionId, int32 Count)
{
	if (MunitionId.IsNone() || Count <= 0)
	{
		return GetRounds(MunitionId);
	}

	int32& Held = Stock.FindOrAdd(MunitionId);
	Held += Count;
	return Held;
}

bool UArtilleryInventoryComponent::ConsumeRounds(FName MunitionId, int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	int32* Held = Stock.Find(MunitionId);
	if (!Held || *Held < Count)
	{
		return false;
	}

	*Held -= Count;
	return true;
}

int32 UArtilleryInventoryComponent::GetRounds(FName MunitionId) const
{
	const int32* Held = Stock.Find(MunitionId);
	return Held ? *Held : 0;
}

bool UArtilleryInventoryComponent::HasRounds(FName MunitionId, int32 Count) const
{
	return Count > 0 && GetRounds(MunitionId) >= Count;
}

int32 UArtilleryInventoryComponent::GetTotalRounds() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Entry : Stock)
	{
		Total += Entry.Value;
	}
	return Total;
}

TArray<FArtilleryAmmoStock> UArtilleryInventoryComponent::GetStock() const
{
	TArray<FArtilleryAmmoStock> Result;
	Result.Reserve(Stock.Num());
	for (const TPair<FName, int32>& Entry : Stock)
	{
		Result.Emplace(Entry.Key, Entry.Value);
	}
	return Result;
}
