#pragma once

#include "CoreMinimal.h"
#include "ArtilleryTypes.h"
#include "ArtilleryLaying.generated.h"

/**
 * The current pointing of a piece: where the tube actually is, and where it has
 * been ordered to go. Kept as a value object so the whole slewing model is pure
 * math, testable without a UWorld (see FArtilleryLayingSolver).
 */
USTRUCT(BlueprintType)
struct ARTILLERY_API FArtilleryLayingState
{
	GENERATED_BODY()

	/** Reference azimuth the carriage is laid on, degrees clockwise from true North. The traverse arc is centered here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Laying")
	double MountAzimuthDegrees = 0.0;

	/** Current tube azimuth, degrees clockwise from true North, normalized to [0, 360). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Laying")
	double CurrentAzimuthDegrees = 0.0;

	/** Current tube elevation above the local horizon, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Laying")
	double CurrentElevationDegrees = 0.0;

	/** Ordered azimuth, already clamped to the traverse arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Laying")
	double TargetAzimuthDegrees = 0.0;

	/** Ordered elevation, already clamped to the elevation limits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Artillery|Laying")
	double TargetElevationDegrees = 0.0;
};

/**
 * Pure laying math for an artillery piece: angle normalization, traverse/
 * elevation limit clamping, and rate-limited slewing toward an ordered laying.
 * Free of Actors, components and UWorld so it can be unit-tested directly; the
 * component layer (UArtilleryLayingComponent) only drives it with delta time.
 */
class ARTILLERY_API FArtilleryLayingSolver
{
public:
	/** Wraps an azimuth into [0, 360). */
	static double NormalizeAzimuth(double AzimuthDegrees);

	/** Signed shortest angular difference To - From, in (-180, 180]. */
	static double ShortestAzimuthDelta(double FromDegrees, double ToDegrees);

	/**
	 * Clamps a desired azimuth into the piece's traverse arc, i.e. within
	 * TraverseHalfArcDegrees either side of MountAzimuthDegrees. A half-arc of
	 * 180 degrees (full traverse) leaves any azimuth untouched.
	 */
	static double ClampAzimuthToArc(double DesiredAzimuthDegrees, double MountAzimuthDegrees, double TraverseHalfArcDegrees);

	/** True if the azimuth lies inside the traverse arc (within a small tolerance). */
	static bool IsAzimuthWithinArc(double AzimuthDegrees, double MountAzimuthDegrees, double TraverseHalfArcDegrees);

	/** Clamps a desired elevation into [MinElevationDegrees, MaxElevationDegrees] of the definition. */
	static double ClampElevation(double DesiredElevationDegrees, const FArtilleryPieceDefinition& Definition);

	/**
	 * Sets the ordered laying, clamping it to the piece's mechanical limits.
	 * Returns the resulting state; the current angles are left untouched — the
	 * tube reaches them only through Step().
	 */
	static FArtilleryLayingState OrderLaying(
		const FArtilleryLayingState& State,
		const FArtilleryPieceDefinition& Definition,
		double DesiredAzimuthDegrees,
		double DesiredElevationDegrees);

	/**
	 * Advances the current angles toward the ordered ones by at most one
	 * DeltaSeconds worth of traverse/elevation rate. Azimuth takes the shortest
	 * path; both axes move simultaneously, as on a real mount.
	 */
	static FArtilleryLayingState Step(
		const FArtilleryLayingState& State,
		const FArtilleryPieceDefinition& Definition,
		double DeltaSeconds);

	/** True once both axes are within ToleranceDegrees of the ordered laying. */
	static bool IsOnTarget(const FArtilleryLayingState& State, double ToleranceDegrees = 0.05);
};
