#pragma once

#include "CoreMinimal.h"
#include "SisaGeoCoordinate.h"
#include "SisaResult.h"
#include "SisaUtmMgrsTypes.h"

/**
 * The single seam other SISA plugins use to talk about geodetic coordinates.
 * No other plugin (Artillery, Ballistics, Simulation, ...) should include a
 * Cesium header directly — they depend on this interface instead, resolved
 * from a UWorld via GetSubsystem<UCesiumGeoreferenceAdapter>(), which is the
 * Phase 1 implementation.
 *
 * Deliberately a plain C++ interface (not a UINTERFACE): several methods
 * return TSisaResult<T>, a template, which Unreal Header Tool cannot
 * reflect. This interface is for C++-to-C++ consumption only, matching the
 * project requirement that all simulation logic live in C++.
 */
class IGeoCoordinateService
{
public:
	virtual ~IGeoCoordinateService() = default;

	/** Converts a WGS84 geodetic coordinate to a position in the current level's Unreal world space. */
	virtual FVector GeoToUnreal(const FGeoCoordinate& Geo) const = 0;

	/** Converts a position in the current level's Unreal world space back to WGS84. */
	virtual FGeoCoordinate UnrealToGeo(const FVector& UnrealPosition) const = 0;

	/** Converts a WGS84 geodetic coordinate to Earth-Centered, Earth-Fixed (ECEF) meters. */
	virtual FVector GeoToEcef(const FGeoCoordinate& Geo) const = 0;

	/** Converts an Earth-Centered, Earth-Fixed (ECEF) position back to WGS84. */
	virtual FGeoCoordinate EcefToGeo(const FVector& Ecef) const = 0;

	/** Converts a WGS84 geodetic coordinate to UTM (zones 1-60 only, see FUtmMgrsConverter). */
	virtual TSisaResult<FUtmCoordinate> GeoToUtm(const FGeoCoordinate& Geo) const = 0;

	/** Converts a UTM coordinate back to WGS84. */
	virtual TSisaResult<FGeoCoordinate> UtmToGeo(const FUtmCoordinate& Utm) const = 0;

	/** Converts a WGS84 geodetic coordinate to an MGRS string at the given digit precision (1-5, i.e. 10km down to 1m). */
	virtual TSisaResult<FString> GeoToMgrs(const FGeoCoordinate& Geo, int32 PrecisionDigits = 5) const = 0;

	/** Parses an MGRS string back to WGS84. */
	virtual TSisaResult<FGeoCoordinate> MgrsToGeo(const FString& Mgrs) const = 0;

	/**
	 * Given a Rotator expressed in a local East-North-Up frame anchored at UnrealLocation
	 * (Unreal world space), returns the equivalent Rotator in Unreal world space. Used to
	 * turn an artillery piece's azimuth/elevation (naturally expressed relative to true
	 * north and the local horizon) into a world-space orientation.
	 */
	virtual FRotator EnuRotatorToUnreal(const FRotator& EnuRotator, const FVector& UnrealLocation) const = 0;
};
