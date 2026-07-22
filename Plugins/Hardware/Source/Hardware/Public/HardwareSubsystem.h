#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "SisaResult.h"
#include "HardwareTypes.h"
#include "HardwareSubsystem.generated.h"

class IHardwareController;

/** Builds a controller instance for a given kind. Phase 2 registers one per real device. */
using FHardwareControllerFactory = TFunction<TSharedRef<IHardwareController>()>;

/**
 * The single point through which SISA reaches hardware: owns the active
 * controller, drives its Poll() once per frame, and republishes telemetry and
 * link-state changes on the SISACore event bus.
 *
 * Controllers are created through registered factories keyed by
 * EHardwareControllerKind. Phase 1 ships one factory (Simulated), registered on
 * Initialize; Phase 2 adds Arduino/ESP32/PLC/STM32 factories from their own
 * module's startup — no call site in Simulation changes, which is the entire
 * point of the IHardwareController seam.
 */
UCLASS()
class HARDWARE_API UHardwareSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem

	/** Registers (or replaces) the factory used to build controllers of a kind. */
	void RegisterFactory(EHardwareControllerKind Kind, FHardwareControllerFactory Factory);

	/** True if a factory is registered for the kind. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Hardware")
	bool HasFactory(EHardwareControllerKind Kind) const { return Factories.Contains(Kind); }

	/**
	 * Builds a controller for Config.Kind and opens the link, replacing any
	 * controller already active. Publishes the resulting state change.
	 */
	TSisaResult<bool> Connect(const FHardwareConnectionConfig& Config);

	/** Blueprint-facing wrapper around Connect. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Hardware", meta = (DisplayName = "Connect"))
	bool ConnectToHardware(const FHardwareConnectionConfig& Config, FSisaError& OutError);

	/** Closes and releases the active controller. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Hardware")
	void Disconnect();

	/** The active controller, or null when none has been connected. */
	TSharedPtr<IHardwareController> GetController() const { return ActiveController; }

	/** Link state of the active controller (Disconnected when there is none). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Hardware")
	EHardwareConnectionState GetConnectionState() const;

	/** True when a controller is connected and ready to take commands. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Hardware")
	bool IsConnected() const { return GetConnectionState() == EHardwareConnectionState::Connected; }

	/** Capabilities of the active controller (all false when there is none). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Hardware")
	FHardwareCapabilities GetCapabilities() const;

	/** Latest telemetry sample from the active controller. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SISA|Hardware")
	FHardwareTelemetry GetTelemetry() const;

	/** Commands the mount to a laying. Fails when no controller is connected. */
	TSisaResult<bool> CommandLaying(double AzimuthDegrees, double ElevationDegrees);

	/** Blueprint-facing wrapper around CommandLaying. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Hardware", meta = (DisplayName = "Command Laying"))
	bool CommandLayingWithReason(double AzimuthDegrees, double ElevationDegrees, FSisaError& OutError);

	/** Halts mount motion. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Hardware")
	bool StopMotion(FSisaError& OutError);

	/** Actuates the physical trigger. */
	UFUNCTION(BlueprintCallable, Category = "SISA|Hardware")
	bool TriggerFire(FSisaError& OutError);

	/**
	 * How often telemetry is republished on the event bus, in seconds. Polling
	 * still happens every frame; only the broadcast is throttled, so a 120 Hz
	 * frame rate does not flood the UI. Zero publishes every frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware", meta = (ClampMin = "0.0"))
	double TelemetryPublishIntervalSeconds = 0.1;

	/** Advances the active controller. Called from the ticker; exposed so tests can drive it deterministically. */
	void PollControllers(double DeltaSeconds);

private:
	/**
	 * Publishes a link state transition on the event bus. The controller is
	 * passed explicitly because a failed Connect() must still report which
	 * device failed, even though it never became the active one.
	 */
	void PublishConnectionEvent(
		const TSharedPtr<IHardwareController>& Controller,
		EHardwareConnectionState PreviousState,
		const FString& ErrorMessage);

	/** Ticker entry point. */
	bool HandleTick(float DeltaSeconds);

	/** Controller builders keyed by device kind. */
	TMap<EHardwareControllerKind, FHardwareControllerFactory> Factories;

	/** The controller currently in use, if any. */
	TSharedPtr<IHardwareController> ActiveController;

	/** Handle of the per-frame ticker driving PollControllers. */
	FTSTicker::FDelegateHandle TickerHandle;

	/** Seconds accumulated since the last telemetry publish. */
	double SecondsSinceTelemetryPublish = 0.0;

	/** Link state at the previous poll, used to detect transitions the controller made on its own. */
	EHardwareConnectionState LastObservedState = EHardwareConnectionState::Disconnected;
};
