#pragma once

#include "CoreMinimal.h"
#include "SisaGeoCoordinate.generated.h"

/**
 * A pure WGS84 geodetic coordinate value object (latitude/longitude/height).
 * Deliberately holds no conversion logic — that behavior lives in the GIS
 * plugin (UCesiumGeoreferenceAdapter / FUtmMgrsConverter) so this struct can
 * be shared by every SISA plugin without pulling in a Cesium dependency.
 */
USTRUCT(BlueprintType)
struct SISACORE_API FGeoCoordinate
{
	GENERATED_BODY()

	/** Degrees, range [-90, 90]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double Latitude = 0.0;

	/** Degrees, range [-180, 180]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double Longitude = 0.0;

	/** Meters above the WGS84 ellipsoid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double HeightMeters = 0.0;

	FGeoCoordinate() = default;

	FGeoCoordinate(double InLatitude, double InLongitude, double InHeightMeters)
		: Latitude(InLatitude)
		, Longitude(InLongitude)
		, HeightMeters(InHeightMeters)
	{
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(Lat=%.7f, Lon=%.7f, H=%.2fm)"), Latitude, Longitude, HeightMeters);
	}
};

/**
 * A local East-North-Up direction/orientation frame anchored at a given
 * geodetic coordinate. Used to express azimuth/elevation of artillery pieces
 * without depending on Cesium's East-South-Up convention directly.
 */
USTRUCT(BlueprintType)
struct SISACORE_API FEnuCoordinate
{
	GENERATED_BODY()

	/** The geodetic coordinate this ENU frame is anchored at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	FGeoCoordinate Origin;

	/** Meters, +X = East. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double East = 0.0;

	/** Meters, +Y = North. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double North = 0.0;

	/** Meters, +Z = Up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double Up = 0.0;
};
