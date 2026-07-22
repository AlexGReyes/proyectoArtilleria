#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SisaGeoCoordinate.h"
#include "BallisticTypes.h"
#include "ArtilleryTypes.generated.h"

struct FSisaError;

/** The functional category of an artillery piece. */
UENUM(BlueprintType)
enum class EArtilleryPieceType : uint8
{
	Howitzer		UMETA(DisplayName = "Howitzer"),
	Gun				UMETA(DisplayName = "Gun"),
	Mortar			UMETA(DisplayName = "Mortar"),
	RocketLauncher	UMETA(DisplayName = "Rocket Launcher"),
	SelfPropelled	UMETA(DisplayName = "Self-Propelled")
};

/** Operational state of a piece, shown as its status in the left-hand panel. */
UENUM(BlueprintType)
enum class EArtilleryOperationalStatus : uint8
{
	/** Ready to lay and fire. */
	Operational		UMETA(DisplayName = "Operational"),
	/** Usable but impaired (reduced rate of fire, partial damage). */
	Degraded		UMETA(DisplayName = "Degraded"),
	/** Temporarily unavailable: servicing, reloading a magazine, cooling. */
	Maintenance		UMETA(DisplayName = "Maintenance"),
	/** Moving between positions; cannot fire. */
	Displacing		UMETA(DisplayName = "Displacing"),
	/** Destroyed or otherwise permanently out of action. */
	OutOfAction		UMETA(DisplayName = "Out of Action")
};

/**
 * The static, data-driven definition of an artillery piece: everything that
 * comes from the catalog rather than from the piece's current situation.
 * Inherits FTableRowBase so it can be authored as a Data Table row (or imported
 * from CSV/JSON); it is also wrapped by UArtilleryPieceDataAsset for per-asset
 * authoring. Mirrors the Ammunition module's FMunitionDefinition approach — no
 * piece values are hard-coded in C++.
 */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryPieceDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Stable machine identifier of the model, unique within the catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	FName PieceId;

	/** Human-readable display name for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	FText DisplayName;

	/** Manufacturer/model designation, e.g. "M114A1". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	FString Model;

	/** Functional category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	EArtilleryPieceType Type = EArtilleryPieceType::Howitzer;

	/** Bore caliber in millimeters. Must match the munitions it can fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	double CaliberMillimeters = 155.0;

	/** Total mass of the piece, in kilograms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	double MassKilograms = 5800.0;

	/** Minimum engageable range, in meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Range")
	double MinRangeMeters = 500.0;

	/** Maximum engageable range, in meters. Must be > MinRangeMeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Range")
	double MaxRangeMeters = 14600.0;

	/**
	 * Half-width of the traverse arc, in degrees, measured either side of the
	 * mount's reference azimuth (the direction the carriage is laid on). Set to
	 * 180 for a fully-traversing turret/self-propelled mount.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Limits", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	double TraverseHalfArcDegrees = 25.0;

	/** Lowest elevation above the horizon the tube can be laid to, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Limits", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
	double MinElevationDegrees = 0.0;

	/** Highest elevation above the horizon the tube can be laid to, in degrees. Must be > MinElevationDegrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Limits", meta = (ClampMin = "-90.0", ClampMax = "90.0"))
	double MaxElevationDegrees = 65.0;

	/** Horizontal slew rate ("velocidad de giro horizontal"), in degrees per second. Must be > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Limits", meta = (ClampMin = "0.0"))
	double TraverseRateDegreesPerSecond = 5.0;

	/** Elevation rate ("velocidad de elevación"), in degrees per second. Must be > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Limits", meta = (ClampMin = "0.0"))
	double ElevationRateDegreesPerSecond = 3.0;

	/** Munition ids this piece is allowed to fire. Empty means "any munition of matching caliber". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery")
	TArray<FName> CompatibleMunitionIds;

	/**
	 * Validates the definition, appending a descriptive FSisaError for each
	 * problem found. Returns true if the definition is internally consistent.
	 */
	bool Validate(TArray<FSisaError>& OutErrors) const;

	/** True if the given range, in meters, falls inside [MinRangeMeters, MaxRangeMeters]. */
	bool IsRangeEngageable(double RangeMeters) const
	{
		return RangeMeters >= MinRangeMeters && RangeMeters <= MaxRangeMeters;
	}

	/** True if this piece may fire the munition with the given id and caliber. */
	bool IsMunitionCompatible(FName MunitionId, double MunitionCaliberMillimeters) const;
};

/** One line of a piece's magazine: how many rounds of a given munition it holds. */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryAmmoStock
{
	GENERATED_BODY()

	/** Munition id, as known to the Ammunition plugin's registry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Inventory")
	FName MunitionId;

	/** Number of rounds currently held. Never negative. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Inventory", meta = (ClampMin = "0"))
	int32 Rounds = 0;

	FArtilleryAmmoStock() = default;

	FArtilleryAmmoStock(FName InMunitionId, int32 InRounds)
		: MunitionId(InMunitionId)
		, Rounds(InRounds)
	{
	}
};

/**
 * One entry of a piece's fire history ("historial de disparos"). The laying and
 * munition fields are filled at the moment of firing; the impact fields are
 * filled in afterwards by whoever owns the ballistic solve (Simulation), which
 * is why they are optional and default to "not resolved".
 */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryFireRecord
{
	GENERATED_BODY()

	/** Monotonic per-piece sequence number, starting at 1. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	int32 ShotNumber = 0;

	/** UTC timestamp of the shot. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	FDateTime TimestampUtc;

	/** Munition fired. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	FName MunitionId;

	/** Geographic position of the piece at the moment of firing. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	FGeoCoordinate LaunchPosition;

	/** Laying used for the shot (azimuth from true North, elevation above horizon). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	FBallisticLaunchParams Laying;

	/** True once the impact fields below have been filled from a ballistic solve. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	bool bImpactResolved = false;

	/** Geographic impact point, valid only when bImpactResolved. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	FGeoCoordinate ImpactPosition;

	/** Horizontal range achieved, in meters, valid only when bImpactResolved. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	double RangeMeters = 0.0;

	/** Time of flight, in seconds, valid only when bImpactResolved. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Artillery|History")
	double TimeOfFlightSeconds = 0.0;
};
