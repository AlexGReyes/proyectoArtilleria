#pragma once

#include "CoreMinimal.h"
#include "IGeoDataLayerProvider.h"

/**
 * Phase 1 placeholder for IGeoDataLayerProvider: keeps the contract usable
 * (Simulation/Artillery can already depend on the interface) without
 * building GeoJSON/KML/Shapefile/DEM/3D-Tiles/imagery parsers that were not
 * requested for this module. Every LoadLayer call fails with a clear,
 * explicit "not implemented" result instead of silently no-oping.
 */
class FNullGeoDataLayerProvider : public IGeoDataLayerProvider
{
public:
	virtual bool SupportsFormat(EGeoDataLayerFormat Format) const override;
	virtual TSisaResult<bool> LoadLayer(const FString& Path, EGeoDataLayerFormat Format) override;
};
