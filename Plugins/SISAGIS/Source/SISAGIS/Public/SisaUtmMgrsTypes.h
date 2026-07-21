#pragma once

#include "CoreMinimal.h"
#include "SisaUtmMgrsTypes.generated.h"

/**
 * A Universal Transverse Mercator coordinate on the WGS84 ellipsoid.
 * Valid for UTM zones 1-60 (i.e. latitudes roughly [-80, 84]); the polar
 * regions (UPS, MGRS zones A/B/Y/Z) are out of scope for a land-based
 * artillery simulator and are not supported.
 */
USTRUCT(BlueprintType)
struct SISAGIS_API FUtmCoordinate
{
	GENERATED_BODY()

	/** UTM zone number, [1, 60]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	int32 ZoneNumber = 0;

	/** True if the coordinate is in the northern hemisphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	bool bNorthernHemisphere = true;

	/** Meters east of the zone's false-easting origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double Easting = 0.0;

	/** Meters north of the equator (or of the 10,000,000m false-northing for the southern hemisphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double Northing = 0.0;

	/** Height above the WGS84 ellipsoid, in meters — carried through unchanged from the source FGeoCoordinate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|GIS")
	double HeightMeters = 0.0;
};

/**
 * Formats a future IGeoDataLayerProvider implementation may support. Phase 1
 * ships only the contract (see IGeoDataLayerProvider / UNullGeoDataLayerProvider);
 * parsers for these formats are deliberately out of scope for this module.
 */
UENUM(BlueprintType)
enum class EGeoDataLayerFormat : uint8
{
	GeoJson,
	Kml,
	Shapefile,
	Dem,
	ThreeDTiles,
	SatelliteImagery,
	TacticalLayer
};
