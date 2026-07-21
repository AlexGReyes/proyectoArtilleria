#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SisaGeoCoordinate.h"
#include "BallisticTypes.h"
#include "MunitionTypes.generated.h"

struct FSisaError;

/** The functional category of a munition. */
UENUM(BlueprintType)
enum class EMunitionType : uint8
{
	HighExplosive	UMETA(DisplayName = "High Explosive"),
	Smoke			UMETA(DisplayName = "Smoke"),
	Illumination	UMETA(DisplayName = "Illumination"),
	Cluster			UMETA(DisplayName = "Cluster"),
	Guided			UMETA(DisplayName = "Guided"),
	Practice		UMETA(DisplayName = "Practice")
};

/** How/when the munition functions at the target ("tipo de explosión"). */
UENUM(BlueprintType)
enum class EMunitionBurstType : uint8
{
	Impact		UMETA(DisplayName = "Impact"),
	Airburst	UMETA(DisplayName = "Airburst"),
	Proximity	UMETA(DisplayName = "Proximity"),
	Timed		UMETA(DisplayName = "Timed"),
	None		UMETA(DisplayName = "None")
};

/**
 * The affected-area geometry of a round landing at a given geographic point:
 * an inner lethal radius and an outer effective radius. This is the geometric
 * basis Simulation draws as circles over the impact point (Phase 1 scope is
 * area geometry only, no quantitative per-target damage model).
 */
USTRUCT(BlueprintType)
struct AMMUNITION_API FMunitionAffectedArea
{
	GENERATED_BODY()

	/** Geographic center of the affected area (the impact point). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ammunition")
	FGeoCoordinate Center;

	/** Outer radius of meaningful effect, in meters. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ammunition")
	double EffectiveRadiusMeters = 0.0;

	/** Inner radius of near-certain lethality, in meters (<= EffectiveRadiusMeters). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ammunition")
	double LethalRadiusMeters = 0.0;
};

/**
 * The complete data-driven definition of a munition. Inherits FTableRowBase so
 * it can be authored directly as a Data Table row (or imported from CSV/JSON);
 * it is also wrapped by UMunitionDataAsset for per-asset (Primary Data Asset)
 * authoring. No munition values are hard-coded in C++ — they live in assets or
 * tables and flow through this struct.
 */
USTRUCT(BlueprintType)
struct AMMUNITION_API FMunitionDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/** Stable machine identifier, unique within the catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	FName MunitionId;

	/** Human-readable display name for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	FText DisplayName;

	/** Caliber in millimeters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	double CaliberMillimeters = 155.0;

	/** Functional category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	EMunitionType Type = EMunitionType::HighExplosive;

	/** How/when the munition functions at the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	EMunitionBurstType BurstType = EMunitionBurstType::Impact;

	/** Propellant charge, in kilograms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	double PropellantChargeKilograms = 0.0;

	/** Nominal maximum range, in meters (reference/catalog value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	double MaxRangeMeters = 0.0;

	/** Outer radius of meaningful effect, in meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	double EffectiveRadiusMeters = 0.0;

	/** Inner radius of near-certain lethality, in meters (<= EffectiveRadiusMeters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition")
	double LethalRadiusMeters = 0.0;

	/** Muzzle velocity, in meters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition|Ballistics")
	double InitialSpeedMps = 800.0;

	/** Projectile mass, in kilograms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition|Ballistics")
	double MassKg = 43.0;

	/** Ballistic coefficient in kg/m^2 (BC = mass / (Cd * A)); see FBallisticProjectileParams. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ammunition|Ballistics")
	double BallisticCoefficient = 3500.0;

	/**
	 * Validates the definition, appending a descriptive FSisaError for each
	 * problem found. Returns true if the definition is internally consistent.
	 */
	bool Validate(TArray<FSisaError>& OutErrors) const;

	/** Builds the ballistic solver input for this munition. */
	FBallisticProjectileParams ToBallisticProjectileParams() const;

	/** Builds the affected-area geometry for a round landing at ImpactCenter. */
	FMunitionAffectedArea MakeAffectedArea(const FGeoCoordinate& ImpactCenter) const;
};
