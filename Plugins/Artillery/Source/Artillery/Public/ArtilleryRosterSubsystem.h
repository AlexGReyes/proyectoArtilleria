#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ArtilleryTypes.h"
#include "ArtilleryRosterSubsystem.generated.h"

class AArtilleryPieceActor;

/**
 * The roster of pieces actually placed in the current world, plus which one the
 * operator has selected. Pieces register themselves in BeginPlay and unregister
 * in EndPlay, so the left-hand panel can list them and the world/panel selection
 * stay in sync through one authority.
 *
 * A UWorldSubsystem rather than a GameInstance one: the roster is a property of
 * the loaded level, and must not survive a level change holding dead Actors.
 * Selection changes and roster changes are published on the SISACore event bus
 * (see SisaArtilleryChannels) so the UI never needs an Artillery include.
 */
UCLASS()
class ARTILLERY_API UArtilleryRosterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Adds a piece to the roster. Ignores null/duplicate registrations. Returns true if it was added. */
	bool RegisterPiece(AArtilleryPieceActor* Piece);

	/** Removes a piece from the roster, clearing the selection if it was the selected one. */
	bool UnregisterPiece(AArtilleryPieceActor* Piece);

	/** Finds a placed piece by its runtime instance id, or null. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	AArtilleryPieceActor* FindByInstanceId(FName InstanceId) const;

	/** Every piece currently placed, in registration order. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	TArray<AArtilleryPieceActor*> GetAllPieces() const;

	/** Every placed piece whose operational status matches. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	TArray<AArtilleryPieceActor*> GetPiecesByStatus(EArtilleryOperationalStatus Status) const;

	/** Number of pieces currently placed. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	int32 Num() const { return Pieces.Num(); }

	/**
	 * Selects a piece by instance id, publishing FArtilleryPieceSelectionEvent.
	 * Pass NAME_None to clear the selection. Returns false if the id is unknown
	 * (the selection is then left unchanged).
	 */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Roster")
	bool SelectPiece(FName InstanceId);

	/** Clears the selection. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Artillery|Roster")
	void ClearSelection() { SelectPiece(NAME_None); }

	/** The selected piece, or null when nothing is selected. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	AArtilleryPieceActor* GetSelectedPiece() const;

	/** Instance id of the selected piece, or NAME_None. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Artillery|Roster")
	FName GetSelectedInstanceId() const { return SelectedInstanceId; }

	//~ Begin UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~ End UWorldSubsystem

private:
	/** Placed pieces keyed by instance id. Weak: the roster never keeps a destroyed Actor alive. */
	UPROPERTY()
	TMap<FName, TWeakObjectPtr<AArtilleryPieceActor>> Pieces;

	/** Registration order, so the panel lists pieces predictably. */
	TArray<FName> RegistrationOrder;

	/** Currently selected piece, or NAME_None. */
	FName SelectedInstanceId;
};
