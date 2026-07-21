#pragma once

#include "CoreMinimal.h"
#include "SisaResult.h"
#include "SisaUtmMgrsTypes.h"

/**
 * Contract for loading external map layers (GeoJSON/KML/Shapefile/DEM/3D
 * Tiles/imagery/tactical overlays) into the GIS scene. Phase 1 ships only
 * this contract plus UNullGeoDataLayerProvider (Private/NullGeoDataLayerProvider.h),
 * which reports every format as not-yet-implemented; real parsers are a
 * later module, once a concrete format is actually requested.
 */
class IGeoDataLayerProvider
{
public:
	virtual ~IGeoDataLayerProvider() = default;

	/** Whether this provider has a working implementation for the given format. */
	virtual bool SupportsFormat(EGeoDataLayerFormat Format) const = 0;

	/** Loads the layer at Path (local file or URL, format-dependent) into the scene. */
	virtual TSisaResult<bool> LoadLayer(const FString& Path, EGeoDataLayerFormat Format) = 0;
};
