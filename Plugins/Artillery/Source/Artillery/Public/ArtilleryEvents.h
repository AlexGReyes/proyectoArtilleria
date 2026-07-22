#pragma once

#include "CoreMinimal.h"
#include "SisaGeoCoordinate.h"
#include "ArtilleryTypes.h"
#include "ArtilleryEvents.generated.h"

/**
 * Channel names for the SISACore event bus (USisaEventBusSubsystem). The bus is
 * typed only by convention — one payload type per channel name — so each name
 * below documents the exact struct it carries. Simulation and the UI subscribe
 * to these without including any Artillery Actor header.
 */
namespace SisaArtilleryChannels
{
	/** Payload: FArtilleryPieceFiredEvent. Published the instant a round leaves a piece. */
	inline const FName PieceFired = TEXT("SISA.Artillery.PieceFired");

	/** Payload: FArtilleryPieceRosterEvent. Published when a piece enters or leaves the roster. */
	inline const FName PieceRosterChanged = TEXT("SISA.Artillery.PieceRosterChanged");

	/** Payload: FArtilleryPieceSelectionEvent. Published when the selected piece changes. */
	inline const FName PieceSelectionChanged = TEXT("SISA.Artillery.PieceSelectionChanged");
}

/**
 * Broadcast when a piece fires. Carries everything Simulation needs to run the
 * ballistic solve and draw the trail/affected area, without reaching back into
 * the Artillery Actor.
 */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryPieceFiredEvent
{
	GENERATED_BODY()

	/** Runtime instance id of the firing piece (unique per placed piece, not per model). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FName PieceInstanceId;

	/** Catalog id of the piece's model. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FName PieceId;

	/** The shot as recorded in the piece's fire history. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FArtilleryFireRecord Record;
};

/** Broadcast when a piece is added to or removed from the live roster. */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryPieceRosterEvent
{
	GENERATED_BODY()

	/** Runtime instance id of the piece. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FName PieceInstanceId;

	/** True when the piece was registered, false when it was unregistered. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	bool bRegistered = false;
};

/** Broadcast when the operator selects a different piece (left panel or world click). */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryPieceSelectionEvent
{
	GENERATED_BODY()

	/** Newly selected piece, or NAME_None when the selection was cleared. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FName SelectedInstanceId;

	/** Previously selected piece, or NAME_None. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|Events")
	FName PreviousInstanceId;
};
