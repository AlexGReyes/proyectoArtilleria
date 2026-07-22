#pragma once

#include "CoreMinimal.h"
#include "HardwareTypes.h"
#include "HardwareEvents.generated.h"

/**
 * Channel names for the SISACore event bus. One payload type per channel, by
 * convention (the bus does not type-check). Simulation and the UI subscribe to
 * these instead of polling the controller themselves.
 */
namespace SisaHardwareChannels
{
	/** Payload: FHardwareTelemetryEvent. Published at the subsystem's publish interval while connected. */
	inline const FName Telemetry = TEXT("SISA.Hardware.Telemetry");

	/** Payload: FHardwareConnectionEvent. Published whenever the link state changes. */
	inline const FName ConnectionChanged = TEXT("SISA.Hardware.ConnectionChanged");
}

/** A telemetry sample, tagged with the controller it came from. */
USTRUCT(BlueprintType)
struct HARDWARE_API FHardwareTelemetryEvent
{
	GENERATED_BODY()

	/** Which controller produced the sample. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	FName ControllerName;

	/** The sample itself. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	FHardwareTelemetry Telemetry;
};

/** A link state transition. */
USTRUCT(BlueprintType)
struct HARDWARE_API FHardwareConnectionEvent
{
	GENERATED_BODY()

	/** Which controller changed state. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	FName ControllerName;

	/** Kind of device behind the link. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	EHardwareControllerKind Kind = EHardwareControllerKind::Simulated;

	/** New link state. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	EHardwareConnectionState State = EHardwareConnectionState::Disconnected;

	/** Previous link state. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	EHardwareConnectionState PreviousState = EHardwareConnectionState::Disconnected;

	/** Populated when State is Error: why the link failed. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Events")
	FString ErrorMessage;
};
