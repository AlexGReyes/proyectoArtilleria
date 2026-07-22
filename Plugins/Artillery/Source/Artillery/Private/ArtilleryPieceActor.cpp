#include "ArtilleryPieceActor.h"
#include "Artillery.h"
#include "ArtilleryCatalogSubsystem.h"
#include "ArtilleryEvents.h"
#include "ArtilleryFireHistoryComponent.h"
#include "ArtilleryInventoryComponent.h"
#include "ArtilleryLayingComponent.h"
#include "ArtilleryPieceDataAsset.h"
#include "ArtilleryRosterSubsystem.h"
#include "CesiumGeoreferenceAdapter.h"
#include "MunitionRegistrySubsystem.h"
#include "SisaEventBusSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AArtilleryPieceActor::AArtilleryPieceActor()
{
	// The piece itself does not tick; only its laying component does, and only
	// while it is actually slewing.
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	LayingComponent = CreateDefaultSubobject<UArtilleryLayingComponent>(TEXT("Laying"));
	InventoryComponent = CreateDefaultSubobject<UArtilleryInventoryComponent>(TEXT("Inventory"));
	FireHistoryComponent = CreateDefaultSubobject<UArtilleryFireHistoryComponent>(TEXT("FireHistory"));
}

void AArtilleryPieceActor::BeginPlay()
{
	Super::BeginPlay();

	if (InstanceId.IsNone())
	{
		InstanceId = GetFName();
	}

	ResolveDefinition();
	SetGeoPosition(GeoPosition);

	if (UArtilleryRosterSubsystem* Roster = GetWorld() ? GetWorld()->GetSubsystem<UArtilleryRosterSubsystem>() : nullptr)
	{
		Roster->RegisterPiece(this);
	}
}

void AArtilleryPieceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UArtilleryRosterSubsystem* Roster = GetWorld() ? GetWorld()->GetSubsystem<UArtilleryRosterSubsystem>() : nullptr)
	{
		Roster->UnregisterPiece(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AArtilleryPieceActor::ResolveDefinition()
{
	if (PieceDataAsset)
	{
		SetDefinition(PieceDataAsset->Definition);
		return;
	}

	if (!CatalogPieceId.IsNone())
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UArtilleryCatalogSubsystem* Catalog = GameInstance->GetSubsystem<UArtilleryCatalogSubsystem>())
			{
				bool bFound = false;
				const FArtilleryPieceDefinition Found = Catalog->FindById(CatalogPieceId, bFound);
				if (bFound)
				{
					SetDefinition(Found);
					return;
				}

				UE_LOG(LogSisaArtillery, Warning,
					TEXT("Piece '%s' references unknown catalog id '%s'; keeping its inline definition."),
					*GetName(), *CatalogPieceId.ToString());
			}
		}
	}

	// Neither source available: the inline Definition authored on the Actor stands.
	SetDefinition(Definition);
}

void AArtilleryPieceActor::SetDefinition(const FArtilleryPieceDefinition& InDefinition)
{
	Definition = InDefinition;

	if (LayingComponent)
	{
		LayingComponent->SetDefinition(Definition);
	}
}

bool AArtilleryPieceActor::SetGeoPosition(const FGeoCoordinate& InGeoPosition)
{
	GeoPosition = InGeoPosition;

	UWorld* World = GetWorld();
	UCesiumGeoreferenceAdapter* Geo = World ? World->GetSubsystem<UCesiumGeoreferenceAdapter>() : nullptr;
	if (!Geo)
	{
		// Headless/test worlds have no georeference; the geographic position is
		// still authoritative, only the Unreal-space placement is skipped.
		return false;
	}

	SetActorLocation(Geo->GeoToUnreal(GeoPosition));
	return true;
}

void AArtilleryPieceActor::SetOperationalStatus(EArtilleryOperationalStatus NewStatus)
{
	OperationalStatus = NewStatus;
}

bool AArtilleryPieceActor::IsOperational() const
{
	return OperationalStatus == EArtilleryOperationalStatus::Operational
		|| OperationalStatus == EArtilleryOperationalStatus::Degraded;
}

bool AArtilleryPieceActor::TryGetMunitionCaliber(FName MunitionId, double& OutCaliberMillimeters) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UMunitionRegistrySubsystem* Registry = GameInstance ? GameInstance->GetSubsystem<UMunitionRegistrySubsystem>() : nullptr;
	if (!Registry)
	{
		return false;
	}

	bool bFound = false;
	const FMunitionDefinition Munition = Registry->FindById(MunitionId, bFound);
	if (!bFound)
	{
		return false;
	}

	OutCaliberMillimeters = Munition.CaliberMillimeters;
	return true;
}

TSisaResult<bool> AArtilleryPieceActor::CanFire(FName MunitionId) const
{
	if (!IsOperational())
	{
		return TSisaResult<bool>::Err(
			TEXT("Artillery.NotOperational"),
			FString::Printf(TEXT("Piece '%s' is not in a state that permits firing."), *InstanceId.ToString()));
	}

	if (MunitionId.IsNone())
	{
		return TSisaResult<bool>::Err(TEXT("Artillery.NoMunitionSelected"), TEXT("No munition selected."));
	}

	double MunitionCaliber = 0.0;
	if (!TryGetMunitionCaliber(MunitionId, MunitionCaliber))
	{
		return TSisaResult<bool>::Err(
			TEXT("Artillery.UnknownMunition"),
			FString::Printf(TEXT("Munition '%s' is not in the munition registry."), *MunitionId.ToString()));
	}

	if (!Definition.IsMunitionCompatible(MunitionId, MunitionCaliber))
	{
		return TSisaResult<bool>::Err(
			TEXT("Artillery.IncompatibleMunition"),
			FString::Printf(TEXT("Munition '%s' is not compatible with piece '%s'."), *MunitionId.ToString(), *Definition.PieceId.ToString()));
	}

	if (!InventoryComponent || !InventoryComponent->HasRounds(MunitionId))
	{
		return TSisaResult<bool>::Err(
			TEXT("Artillery.OutOfAmmunition"),
			FString::Printf(TEXT("No rounds of '%s' remaining."), *MunitionId.ToString()));
	}

	if (!LayingComponent || !LayingComponent->IsOnTarget())
	{
		return TSisaResult<bool>::Err(
			TEXT("Artillery.NotLaid"),
			TEXT("The tube has not yet reached its ordered laying."));
	}

	return TSisaResult<bool>::Ok(true);
}

bool AArtilleryPieceActor::CanFireWithReason(FName MunitionId, FSisaError& OutError) const
{
	const TSisaResult<bool> Result = CanFire(MunitionId);
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutError = FSisaError();
	return true;
}

TSisaResult<FArtilleryFireRecord> AArtilleryPieceActor::Fire(FName MunitionId)
{
	const TSisaResult<bool> Allowed = CanFire(MunitionId);
	if (Allowed.IsError())
	{
		return TSisaResult<FArtilleryFireRecord>::Err(Allowed.GetError());
	}

	if (!InventoryComponent->ConsumeRounds(MunitionId, 1))
	{
		// Only reachable if the magazine changed between CanFire and here.
		return TSisaResult<FArtilleryFireRecord>::Err(
			TEXT("Artillery.OutOfAmmunition"),
			FString::Printf(TEXT("No rounds of '%s' remaining."), *MunitionId.ToString()));
	}

	FArtilleryFireRecord Record;
	Record.MunitionId = MunitionId;
	Record.LaunchPosition = GeoPosition;
	Record.Laying = LayingComponent->ToBallisticLaunchParams();

	const FArtilleryFireRecord Stored = FireHistoryComponent->AddRecord(Record);

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USisaEventBusSubsystem* Bus = GameInstance->GetSubsystem<USisaEventBusSubsystem>())
		{
			FArtilleryPieceFiredEvent Event;
			Event.PieceInstanceId = InstanceId;
			Event.PieceId = Definition.PieceId;
			Event.Record = Stored;
			Bus->Publish(SisaArtilleryChannels::PieceFired, Event);
		}
	}

	UE_LOG(LogSisaArtillery, Verbose, TEXT("Piece '%s' fired round #%d of '%s' at Az=%.2f El=%.2f"),
		*InstanceId.ToString(), Stored.ShotNumber, *MunitionId.ToString(),
		Stored.Laying.AzimuthDegrees, Stored.Laying.ElevationDegrees);

	return TSisaResult<FArtilleryFireRecord>::Ok(Stored);
}

bool AArtilleryPieceActor::FireRound(FName MunitionId, FArtilleryFireRecord& OutRecord, FSisaError& OutError)
{
	const TSisaResult<FArtilleryFireRecord> Result = Fire(MunitionId);
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutRecord = Result.GetValue();
	OutError = FSisaError();
	return true;
}
