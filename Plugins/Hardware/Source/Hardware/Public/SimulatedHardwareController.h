#pragma once

#include "CoreMinimal.h"
#include "IHardwareController.h"

/**
 * The Phase 1 default: a controller with no I/O that models a motorised mount
 * in process. It slews toward the commanded laying at configurable rates (using
 * the same SisaAngle math Artillery lays a real piece with), reports encoder,
 * GNSS, compass and IMU telemetry, and counts trigger pulls.
 *
 * Its purpose is to let Simulation be written, run and tested against the real
 * IHardwareController contract before any physical device exists — including
 * failure paths, since commanding a disconnected controller fails here exactly
 * as it will on a real link.
 */
class HARDWARE_API FSimulatedHardwareController : public IHardwareController
{
public:
	explicit FSimulatedHardwareController(FName InControllerName = TEXT("SimulatedController"));

	/** Traverse rate of the simulated mount, in degrees per second. */
	void SetTraverseRate(double DegreesPerSecond) { TraverseRateDegreesPerSecond = FMath::Max(DegreesPerSecond, 0.0); }

	/** Elevation rate of the simulated mount, in degrees per second. */
	void SetElevationRate(double DegreesPerSecond) { ElevationRateDegreesPerSecond = FMath::Max(DegreesPerSecond, 0.0); }

	/** Mechanical elevation limits of the simulated mount, in degrees. */
	void SetElevationLimits(double MinDegrees, double MaxDegrees);

	/** The position the simulated GNSS receiver reports. */
	void SetSimulatedPosition(const FGeoCoordinate& Position) { SimulatedPosition = Position; }

	/** Places the tube at a laying immediately, bypassing the slew (scenario setup, tests). */
	void SnapToLaying(double AzimuthDegrees, double ElevationDegrees);

	/**
	 * Forces the link into the error state, so Simulation's failure handling can
	 * be exercised without unplugging anything.
	 */
	void SimulateLinkFailure(const FString& Reason);

	//~ Begin IHardwareController
	virtual FName GetControllerName() const override { return ControllerName; }
	virtual EHardwareControllerKind GetKind() const override { return EHardwareControllerKind::Simulated; }
	virtual FHardwareCapabilities GetCapabilities() const override;
	virtual TSisaResult<bool> Connect(const FHardwareConnectionConfig& Config) override;
	virtual void Disconnect() override;
	virtual EHardwareConnectionState GetConnectionState() const override { return ConnectionState; }
	virtual TSisaResult<bool> CommandLaying(double AzimuthDegrees, double ElevationDegrees) override;
	virtual TSisaResult<bool> StopMotion() override;
	virtual TSisaResult<bool> TriggerFire() override;
	virtual FHardwareTelemetry ReadTelemetry() const override { return Telemetry; }
	virtual void Poll(double DeltaSeconds) override;
	//~ End IHardwareController

private:
	/** Rebuilds the telemetry snapshot from the current mount state. */
	void RefreshTelemetry(double DeltaSeconds);

	/** Fails a command when the link is not open, with a uniform error. */
	TSisaResult<bool> RequireConnected(const TCHAR* Operation) const;

	FName ControllerName;
	EHardwareConnectionState ConnectionState = EHardwareConnectionState::Disconnected;

	double TraverseRateDegreesPerSecond = 5.0;
	double ElevationRateDegreesPerSecond = 3.0;
	double MinElevationDegrees = -5.0;
	double MaxElevationDegrees = 70.0;

	double CurrentAzimuthDegrees = 0.0;
	double CurrentElevationDegrees = 0.0;
	double TargetAzimuthDegrees = 0.0;
	double TargetElevationDegrees = 0.0;

	FGeoCoordinate SimulatedPosition;
	FHardwareTelemetry Telemetry;
	double ElapsedSeconds = 0.0;
	int32 FireCount = 0;
};
