#include "NullGeoDataLayerProvider.h"

bool FNullGeoDataLayerProvider::SupportsFormat(EGeoDataLayerFormat Format) const
{
	return false;
}

TSisaResult<bool> FNullGeoDataLayerProvider::LoadLayer(const FString& Path, EGeoDataLayerFormat Format)
{
	return TSisaResult<bool>::Err(
		TEXT("GIS.Layer.NotImplemented"),
		FString::Printf(TEXT("No IGeoDataLayerProvider implementation is registered yet for format %d (path '%s')."),
			static_cast<int32>(Format), *Path));
}
