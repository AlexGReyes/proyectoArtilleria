#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IGeoCoordinateService.h"
#include "CesiumGeoreferenceAdapter.generated.h"

class ACesiumGeoreference;

/**
 * The Phase 1 (and, unchanged through Phase 2/3) implementation of
 * IGeoCoordinateService: the only class in SISA that is allowed to include
 * Cesium headers. A UWorldSubsystem because ACesiumGeoreference is a
 * per-level Actor — resolve this via
 * GetWorld()->GetSubsystem<UCesiumGeoreferenceAdapter>().
 */
UCLASS()
class SISAGIS_API UCesiumGeoreferenceAdapter : public UWorldSubsystem, public IGeoCoordinateService
{
	GENERATED_BODY()

public:
	//~ Begin IGeoCoordinateService
	virtual FVector GeoToUnreal(const FGeoCoordinate& Geo) const override;
	virtual FGeoCoordinate UnrealToGeo(const FVector& UnrealPosition) const override;
	virtual FVector GeoToEcef(const FGeoCoordinate& Geo) const override;
	virtual FGeoCoordinate EcefToGeo(const FVector& Ecef) const override;
	virtual TSisaResult<FUtmCoordinate> GeoToUtm(const FGeoCoordinate& Geo) const override;
	virtual TSisaResult<FGeoCoordinate> UtmToGeo(const FUtmCoordinate& Utm) const override;
	virtual TSisaResult<FString> GeoToMgrs(const FGeoCoordinate& Geo, int32 PrecisionDigits = 5) const override;
	virtual TSisaResult<FGeoCoordinate> MgrsToGeo(const FString& Mgrs) const override;
	virtual FRotator EnuRotatorToUnreal(const FRotator& EnuRotator, const FVector& UnrealLocation) const override;
	//~ End IGeoCoordinateService

	//~ Begin UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~ End UWorldSubsystem

private:
	ACesiumGeoreference* GetGeoreference() const;
};
