#pragma once

#include "CoreMinimal.h"
#include "SisaResult.h"
#include "SisaGeoCoordinate.h"
#include "SisaUtmMgrsTypes.h"

/**
 * Stateless WGS84 UTM/MGRS math. Deliberately has no UObject-ness and no
 * Cesium dependency so it is trivially unit-testable (see
 * Tests/UtmMgrsConverterSpec.cpp) and reusable outside of a UWorld.
 *
 * Implements the standard Snyder (USGS Professional Paper 1395) ellipsoidal
 * transverse Mercator series for the forward/inverse UTM conversion, and the
 * NGA 100km-grid-square scheme for MGRS. Two known limitations, both
 * out of scope for a land-based artillery simulator:
 *   1. The Norway/Svalbard UTM zone-boundary exceptions (zones 32V/31X-37X)
 *      are NOT implemented; zone numbers there use the plain longitude-based
 *      formula instead of the exception grid.
 *   2. Polar regions (UPS, MGRS bands A/B/Y/Z) are not supported.
 * The 100km grid-square letter derivation is the most fiddly part of this
 * file; treat it with extra scrutiny and cross-check against a reference
 * geodesy tool before relying on it operationally.
 */
class FUtmMgrsConverter
{
public:
	static TSisaResult<FUtmCoordinate> ToUtm(const FGeoCoordinate& Geo);
	static TSisaResult<FGeoCoordinate> FromUtm(const FUtmCoordinate& Utm);

	static TSisaResult<FString> ToMgrs(const FGeoCoordinate& Geo, int32 PrecisionDigits = 5);
	static TSisaResult<FGeoCoordinate> FromMgrs(const FString& Mgrs);

private:
	// WGS84 ellipsoid constants.
	static constexpr double SemiMajorAxis = 6378137.0;
	static constexpr double Flattening = 1.0 / 298.257223563;
	static constexpr double ScaleFactor = 0.9996;

	static double GetEccentricitySquared();

	static int32 GetZoneNumber(double LongitudeDegrees);
	static double GetCentralMeridian(int32 ZoneNumber);
	static TCHAR GetLatitudeBandLetter(double LatitudeDegrees);
	static bool GetLatitudeBandRangeDegrees(TCHAR Band, double& OutMinLat, double& OutMaxLat);

	static void GetGridSquareId(int32 ZoneNumber, double Easting, double Northing, TCHAR& OutColLetter, TCHAR& OutRowLetter);
	static bool DecodeGridSquareId(int32 ZoneNumber, TCHAR ColLetter, TCHAR RowLetter, double& OutEasting100kBase, double& OutNorthing100kBase);
};
