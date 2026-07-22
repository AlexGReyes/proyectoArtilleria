#include "ArtilleryRosterSubsystem.h"
#include "Artillery.h"
#include "ArtilleryEvents.h"
#include "ArtilleryPieceActor.h"
#include "SisaEventBusSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	/** The event bus lives on the GameInstance; the roster on the World. */
	USisaEventBusSubsystem* GetEventBus(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USisaEventBusSubsystem>() : nullptr;
	}
}

bool UArtilleryRosterSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::Editor;
}

bool UArtilleryRosterSubsystem::RegisterPiece(AArtilleryPieceActor* Piece)
{
	if (!Piece || Piece->InstanceId.IsNone())
	{
		return false;
	}

	if (const TWeakObjectPtr<AArtilleryPieceActor>* Existing = Pieces.Find(Piece->InstanceId))
	{
		if (Existing->IsValid() && Existing->Get() != Piece)
		{
			UE_LOG(LogSisaArtillery, Warning,
				TEXT("Two artillery pieces share the instance id '%s'; the second registration is ignored."),
				*Piece->InstanceId.ToString());
		}
		return false;
	}

	Pieces.Add(Piece->InstanceId, Piece);
	RegistrationOrder.Add(Piece->InstanceId);

	if (USisaEventBusSubsystem* Bus = GetEventBus(GetWorld()))
	{
		FArtilleryPieceRosterEvent Event;
		Event.PieceInstanceId = Piece->InstanceId;
		Event.bRegistered = true;
		Bus->Publish(SisaArtilleryChannels::PieceRosterChanged, Event);
	}

	return true;
}

bool UArtilleryRosterSubsystem::UnregisterPiece(AArtilleryPieceActor* Piece)
{
	if (!Piece || Pieces.Remove(Piece->InstanceId) == 0)
	{
		return false;
	}

	RegistrationOrder.Remove(Piece->InstanceId);

	if (SelectedInstanceId == Piece->InstanceId)
	{
		SelectPiece(NAME_None);
	}

	if (USisaEventBusSubsystem* Bus = GetEventBus(GetWorld()))
	{
		FArtilleryPieceRosterEvent Event;
		Event.PieceInstanceId = Piece->InstanceId;
		Event.bRegistered = false;
		Bus->Publish(SisaArtilleryChannels::PieceRosterChanged, Event);
	}

	return true;
}

AArtilleryPieceActor* UArtilleryRosterSubsystem::FindByInstanceId(FName InstanceId) const
{
	const TWeakObjectPtr<AArtilleryPieceActor>* Found = Pieces.Find(InstanceId);
	return Found ? Found->Get() : nullptr;
}

TArray<AArtilleryPieceActor*> UArtilleryRosterSubsystem::GetAllPieces() const
{
	TArray<AArtilleryPieceActor*> Result;
	Result.Reserve(RegistrationOrder.Num());
	for (const FName& InstanceId : RegistrationOrder)
	{
		if (AArtilleryPieceActor* Piece = FindByInstanceId(InstanceId))
		{
			Result.Add(Piece);
		}
	}
	return Result;
}

TArray<AArtilleryPieceActor*> UArtilleryRosterSubsystem::GetPiecesByStatus(EArtilleryOperationalStatus Status) const
{
	TArray<AArtilleryPieceActor*> Result;
	for (AArtilleryPieceActor* Piece : GetAllPieces())
	{
		if (Piece->OperationalStatus == Status)
		{
			Result.Add(Piece);
		}
	}
	return Result;
}

bool UArtilleryRosterSubsystem::SelectPiece(FName InstanceId)
{
	if (!InstanceId.IsNone() && !Pieces.Contains(InstanceId))
	{
		return false;
	}

	if (InstanceId == SelectedInstanceId)
	{
		return true;
	}

	const FName Previous = SelectedInstanceId;
	SelectedInstanceId = InstanceId;

	if (USisaEventBusSubsystem* Bus = GetEventBus(GetWorld()))
	{
		FArtilleryPieceSelectionEvent Event;
		Event.SelectedInstanceId = SelectedInstanceId;
		Event.PreviousInstanceId = Previous;
		Bus->Publish(SisaArtilleryChannels::PieceSelectionChanged, Event);
	}

	return true;
}

AArtilleryPieceActor* UArtilleryRosterSubsystem::GetSelectedPiece() const
{
	return FindByInstanceId(SelectedInstanceId);
}
