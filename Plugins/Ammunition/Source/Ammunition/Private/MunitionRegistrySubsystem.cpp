#include "MunitionRegistrySubsystem.h"
#include "MunitionDataAsset.h"
#include "Ammunition.h"
#include "SisaResult.h"
#include "Engine/DataTable.h"
#include "Engine/AssetManager.h"

bool UMunitionRegistrySubsystem::RegisterDefinition(const FMunitionDefinition& Definition)
{
	TArray<FSisaError> Errors;
	if (!Definition.Validate(Errors))
	{
		for (const FSisaError& Error : Errors)
		{
			UE_LOG(LogSisaAmmunition, Warning, TEXT("Rejected munition '%s': %s (%s)"),
				*Definition.MunitionId.ToString(), *Error.Message, *Error.Code.ToString());
		}
		return false;
	}

	Munitions.Add(Definition.MunitionId, Definition);
	return true;
}

int32 UMunitionRegistrySubsystem::LoadFromDataTable(const UDataTable* DataTable)
{
	if (!DataTable)
	{
		return 0;
	}

	if (DataTable->GetRowStruct() != FMunitionDefinition::StaticStruct())
	{
		UE_LOG(LogSisaAmmunition, Warning,
			TEXT("LoadFromDataTable: '%s' row struct is not FMunitionDefinition; skipping."), *DataTable->GetName());
		return 0;
	}

	int32 AddedCount = 0;
	for (const TPair<FName, uint8*>& Row : DataTable->GetRowMap())
	{
		const FMunitionDefinition* Definition = reinterpret_cast<const FMunitionDefinition*>(Row.Value);
		if (Definition && RegisterDefinition(*Definition))
		{
			++AddedCount;
		}
	}
	return AddedCount;
}

int32 UMunitionRegistrySubsystem::LoadFromAssetManager()
{
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		return 0;
	}

	const FPrimaryAssetType MunitionType(TEXT("Munition"));
	TArray<FPrimaryAssetId> AssetIds;
	Manager->GetPrimaryAssetIdList(MunitionType, AssetIds);

	int32 AddedCount = 0;
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath AssetPath = Manager->GetPrimaryAssetPath(AssetId);
		if (const UMunitionDataAsset* Asset = Cast<UMunitionDataAsset>(AssetPath.TryLoad()))
		{
			if (RegisterDefinition(Asset->Definition))
			{
				++AddedCount;
			}
		}
	}
	return AddedCount;
}

FMunitionDefinition UMunitionRegistrySubsystem::FindById(FName MunitionId, bool& bFound) const
{
	if (const FMunitionDefinition* Found = Munitions.Find(MunitionId))
	{
		bFound = true;
		return *Found;
	}
	bFound = false;
	return FMunitionDefinition();
}

TArray<FMunitionDefinition> UMunitionRegistrySubsystem::GetAll() const
{
	TArray<FMunitionDefinition> Result;
	Munitions.GenerateValueArray(Result);
	return Result;
}

TArray<FMunitionDefinition> UMunitionRegistrySubsystem::GetByType(EMunitionType Type) const
{
	TArray<FMunitionDefinition> Result;
	for (const TPair<FName, FMunitionDefinition>& Pair : Munitions)
	{
		if (Pair.Value.Type == Type)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}
