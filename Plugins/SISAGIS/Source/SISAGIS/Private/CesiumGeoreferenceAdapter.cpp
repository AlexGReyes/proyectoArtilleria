#include "CesiumGeoreferenceAdapter.h"
#include "UtmMgrsConverter.h"
#include "CesiumGeoreference.h"
#include "CesiumWgs84Ellipsoid.h"

bool UCesiumGeoreferenceAdapter::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::Editor || WorldType == EWorldType::PIE;
}

ACesiumGeoreference* UCesiumGeoreferenceAdapter::GetGeoreference() const
{
	return ACesiumGeoreference::GetDefaultGeoreference(GetWorld());
}

FVector UCesiumGeoreferenceAdapter::GeoToUnreal(const FGeoCoordinate& Geo) const
{
	if (ACesiumGeoreference* Georeference = GetGeoreference())
	{
		// Cesium packs Longitude/Latitude/Height into FVector(X, Y, Z) in that order.
		return Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(
			FVector(Geo.Longitude, Geo.Latitude, Geo.HeightMeters));
	}
	return FVector::ZeroVector;
}

FGeoCoordinate UCesiumGeoreferenceAdapter::UnrealToGeo(const FVector& UnrealPosition) const
{
	if (ACesiumGeoreference* Georeference = GetGeoreference())
	{
		const FVector Llh = Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(UnrealPosition);
		return FGeoCoordinate(Llh.Y, Llh.X, Llh.Z);
	}
	return FGeoCoordinate();
}

FVector UCesiumGeoreferenceAdapter::GeoToEcef(const FGeoCoordinate& Geo) const
{
	return UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
		FVector(Geo.Longitude, Geo.Latitude, Geo.HeightMeters));
}

FGeoCoordinate UCesiumGeoreferenceAdapter::EcefToGeo(const FVector& Ecef) const
{
	const FVector Llh = UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(Ecef);
	return FGeoCoordinate(Llh.Y, Llh.X, Llh.Z);
}

TSisaResult<FUtmCoordinate> UCesiumGeoreferenceAdapter::GeoToUtm(const FGeoCoordinate& Geo) const
{
	return FUtmMgrsConverter::ToUtm(Geo);
}

TSisaResult<FGeoCoordinate> UCesiumGeoreferenceAdapter::UtmToGeo(const FUtmCoordinate& Utm) const
{
	return FUtmMgrsConverter::FromUtm(Utm);
}

TSisaResult<FString> UCesiumGeoreferenceAdapter::GeoToMgrs(const FGeoCoordinate& Geo, int32 PrecisionDigits) const
{
	return FUtmMgrsConverter::ToMgrs(Geo, PrecisionDigits);
}

TSisaResult<FGeoCoordinate> UCesiumGeoreferenceAdapter::MgrsToGeo(const FString& Mgrs) const
{
	return FUtmMgrsConverter::FromMgrs(Mgrs);
}

FRotator UCesiumGeoreferenceAdapter::EnuRotatorToUnreal(const FRotator& EnuRotator, const FVector& UnrealLocation) const
{
	// EnuRotator.Yaw is interpreted as a compass azimuth (degrees clockwise from true
	// North). Cesium's East-South-Up frame measures yaw clockwise from East instead, so
	// the two are related by a constant -90 degree offset around the shared Up axis --
	// both conventions rotate clockwise viewed from above, so no handedness correction
	// is needed, only the reference-direction offset. Pitch (elevation above the local
	// horizon) and Roll pass through unchanged.
	FRotator EsuRotator = EnuRotator;
	EsuRotator.Yaw = EnuRotator.Yaw - 90.0;

	if (ACesiumGeoreference* Georeference = GetGeoreference())
	{
		return Georeference->TransformEastSouthUpRotatorToUnreal(EsuRotator, UnrealLocation);
	}
	return EnuRotator;
}
