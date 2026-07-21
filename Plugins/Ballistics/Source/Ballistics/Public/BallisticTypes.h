#pragma once

#include "CoreMinimal.h"
#include "BallisticTypes.generated.h"

/**
 * The ballistic properties of a projectile needed to solve its flight. These
 * mirror the physically-relevant fields of a SISA munition (see the Ammunition
 * module, which will feed these values from a data asset). All units SI.
 */
USTRUCT(BlueprintType)
struct BALLISTICS_API FBallisticProjectileParams
{
	GENERATED_BODY()

	/** Muzzle velocity in meters per second. Must be > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double InitialSpeedMps = 800.0;

	/** Projectile mass in kilograms. Must be > 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double MassKg = 43.0;

	/**
	 * Ballistic coefficient in kg/m^2, defined here as BC = mass / (Cd * A),
	 * where Cd is the drag coefficient and A the reference cross-sectional area.
	 * Larger BC = less drag deceleration. Must be > 0. The drag deceleration
	 * used by the solver is a = 0.5 * rho * |v_rel| * v_rel / BC.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double BallisticCoefficient = 3500.0;
};

/**
 * Launch geometry, expressed relative to the local horizon and true North.
 * The solver works in a local East-North-Up (ENU) frame with the muzzle at the
 * origin; Artillery/Simulation map that local frame to geographic/Unreal space
 * via the GIS plugin's IGeoCoordinateService.
 */
USTRUCT(BlueprintType)
struct BALLISTICS_API FBallisticLaunchParams
{
	GENERATED_BODY()

	/** Compass azimuth in degrees, clockwise from true North (0 = North, 90 = East). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double AzimuthDegrees = 0.0;

	/** Elevation angle in degrees above the local horizon, range [0, 90]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double ElevationDegrees = 45.0;
};

/**
 * Ground-level weather that drives the atmosphere model. These are the fields
 * the Simulation weather panel edits. Air density is derived from temperature,
 * pressure and humidity by FStandardAtmosphereModel rather than being entered
 * directly.
 */
USTRUCT(BlueprintType)
struct BALLISTICS_API FEnvironmentConditions
{
	GENERATED_BODY()

	/** Air temperature at ground level, in degrees Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics|Weather")
	double TemperatureCelsius = 15.0;

	/** Air pressure at ground level, in Pascals (sea-level standard = 101325). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics|Weather")
	double PressurePascals = 101325.0;

	/** Relative humidity, 0..1 (0 = dry air, 1 = saturated). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics|Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double RelativeHumidity01 = 0.0;

	/** Wind speed in meters per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics|Weather")
	double WindSpeedMps = 0.0;

	/**
	 * Direction the wind blows FROM, in degrees clockwise from true North
	 * (meteorological convention: 90 = wind coming from the East, blowing West).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics|Weather")
	double WindFromDirectionDegrees = 0.0;

	/**
	 * Returns the wind velocity as an ENU vector (X=East, Y=North, Z=Up) in m/s,
	 * i.e. the direction the air is actually moving toward. A wind coming FROM
	 * the North (WindFromDirectionDegrees = 0) blows toward the South, giving a
	 * negative North (Y) component.
	 */
	FVector GetWindVelocityEnu() const
	{
		const double FromRad = FMath::DegreesToRadians(WindFromDirectionDegrees);
		// "Toward" direction is the reverse of the "from" direction.
		const double EastComponent = -WindSpeedMps * FMath::Sin(FromRad);
		const double NorthComponent = -WindSpeedMps * FMath::Cos(FromRad);
		return FVector(EastComponent, NorthComponent, 0.0);
	}
};

/** Tunable numerical parameters for a solve. Defaults are sensible for field artillery. */
USTRUCT(BlueprintType)
struct BALLISTICS_API FBallisticsSolveConfig
{
	GENERATED_BODY()

	/** Fixed RK4 integration step, in seconds. Smaller = more accurate, slower. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double IntegrationTimeStepSeconds = 0.01;

	/** Safety cap on simulated flight time, in seconds, to avoid runaway integration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double MaxFlightTimeSeconds = 300.0;

	/**
	 * Spacing between points emitted into the output trajectory polyline, in
	 * seconds. Independent of the integration step: the solver integrates finely
	 * but only records a point roughly every OutputSampleIntervalSeconds, keeping
	 * FBallisticTrajectory light enough for a Niagara trail.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double OutputSampleIntervalSeconds = 0.25;

	/** Local ENU Up (Z) height at which impact is detected, in meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double ImpactPlaneHeightMeters = 0.0;

	/** Gravitational acceleration magnitude, in m/s^2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Ballistics")
	double GravityMps2 = 9.80665;
};

/** A single sampled point along a solved trajectory, in the local ENU frame. */
USTRUCT(BlueprintType)
struct BALLISTICS_API FTrajectoryPoint
{
	GENERATED_BODY()

	/** Time since launch, in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double TimeSeconds = 0.0;

	/** Position in local ENU meters (X=East, Y=North, Z=Up), origin at the muzzle. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	FVector PositionEnu = FVector::ZeroVector;

	/** Velocity in local ENU meters/second. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	FVector VelocityEnu = FVector::ZeroVector;

	FTrajectoryPoint() = default;

	FTrajectoryPoint(double InTime, const FVector& InPosition, const FVector& InVelocity)
		: TimeSeconds(InTime)
		, PositionEnu(InPosition)
		, VelocityEnu(InVelocity)
	{
	}
};

/**
 * The full result of a solve: the sampled flight path plus summary metrics.
 * All positions/velocities are in the local ENU frame (meters, meters/second)
 * with the origin at the muzzle.
 */
USTRUCT(BlueprintType)
struct BALLISTICS_API FBallisticTrajectory
{
	GENERATED_BODY()

	/** Sampled polyline of the flight, from launch to impact. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	TArray<FTrajectoryPoint> Points;

	/** Total time of flight, in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double TimeOfFlightSeconds = 0.0;

	/** Maximum height above launch, in meters (apogee). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double MaxHeightMeters = 0.0;

	/** Horizontal distance from launch to impact, in meters. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double RangeMeters = 0.0;

	/** Impact position in local ENU meters. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	FVector ImpactPointEnu = FVector::ZeroVector;

	/** Velocity at impact in local ENU meters/second. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	FVector ImpactVelocityEnu = FVector::ZeroVector;

	/** Speed (magnitude) at impact, in meters/second. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double ResidualSpeedMps = 0.0;

	/** Kinetic energy at impact, in Joules (0.5 * m * v^2). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	double ImpactEnergyJoules = 0.0;

	/** True if the projectile reached the impact plane within the time cap. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Ballistics")
	bool bImpacted = false;
};
