#include "ArtilleryCatalogSubsystem.h"
#include "Artillery.h"
#include "ArtilleryPieceDataAsset.h"
#include "SisaResult.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"

bool UArtilleryCatalogSubsystem::RegisterDefinition(const FArtilleryPieceDefinition& Definition)
{
	TArray<FSisaError> Errors;
	if (!Definition.Validate(Errors))
	{
		for (const FSisaError& Error : Errors)
		{
			UE_LOG(LogSisaArtillery, Warning, TEXT("Rejected artillery piece '%s': %s (%s)"),
				*Definition.PieceId.ToString(), *Error.Message, *Error.Code.ToString());
		}
		return false;
	}

	Pieces.Add(Definition.PieceId, Definition);
	return true;
}

int32 UArtilleryCatalogSubsystem::LoadFromDataTable(const UDataTable* DataTable)
{
	if (!DataTable)
	{
		return 0;
	}

	if (DataTable->GetRowStruct() != FArtilleryPieceDefinition::StaticStruct())
	{
		UE_LOG(LogSisaArtillery, Warning,
			TEXT("LoadFromDataTable: '%s' row struct is not FArtilleryPieceDefinition; skipping."), *DataTable->GetName());
		return 0;
	}

	int32 AddedCount = 0;
	for (const TPair<FName, uint8*>& Row : DataTable->GetRowMap())
	{
		const FArtilleryPieceDefinition* Definition = reinterpret_cast<const FArtilleryPieceDefinition*>(Row.Value);
		if (Definition && RegisterDefinition(*Definition))
		{
			++AddedCount;
		}
	}
	return AddedCount;
}

int32 UArtilleryCatalogSubsystem::LoadFromAssetManager()
{
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		return 0;
	}

	const FPrimaryAssetType PieceType(TEXT("ArtilleryPiece"));
	TArray<FPrimaryAssetId> AssetIds;
	Manager->GetPrimaryAssetIdList(PieceType, AssetIds);

	int32 AddedCount = 0;
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath AssetPath = Manager->GetPrimaryAssetPath(AssetId);
		if (const UArtilleryPieceDataAsset* Asset = Cast<UArtilleryPieceDataAsset>(AssetPath.TryLoad()))
		{
			if (RegisterDefinition(Asset->Definition))
			{
				++AddedCount;
			}
		}
	}
	return AddedCount;
}

FArtilleryPieceDefinition UArtilleryCatalogSubsystem::FindById(FName PieceId, bool& bFound) const
{
	if (const FArtilleryPieceDefinition* Found = Pieces.Find(PieceId))
	{
		bFound = true;
		return *Found;
	}
	bFound = false;
	return FArtilleryPieceDefinition();
}

TArray<FArtilleryPieceDefinition> UArtilleryCatalogSubsystem::GetAll() const
{
	TArray<FArtilleryPieceDefinition> Result;
	Pieces.GenerateValueArray(Result);
	return Result;
}

TArray<FArtilleryPieceDefinition> UArtilleryCatalogSubsystem::GetByType(EArtilleryPieceType Type) const
{
	TArray<FArtilleryPieceDefinition> Result;
	for (const TPair<FName, FArtilleryPieceDefinition>& Pair : Pieces)
	{
		if (Pair.Value.Type == Type)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}
