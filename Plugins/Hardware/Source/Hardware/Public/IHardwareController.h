#pragma once

#include "CoreMinimal.h"
#include "SisaResult.h"
#include "HardwareTypes.h"

/**
 * The seam between SISA and any physical artillery control hardware — the
 * contract that makes the Phase 1 → Phase 2 transition a matter of swapping an
 * implementation rather than rewriting simulation logic. Simulation and
 * Artillery talk to this interface only; no consumer ever includes a serial,
 * CAN or socket header.
 *
 * A plain C++ interface (not a UINTERFACE) for the same reason as
 * IGeoCoordinateService: several methods return TSisaResult<T>, a template,
 * which Unreal Header Tool cannot reflect. Blueprint access, when needed, goes
 * through UHardwareSubsystem.
 *
 * Threading: every method is called from the game thread. A Phase 2
 * implementation that needs real I/O must do it on its own thread/reader and
 * hand finished samples back here, so Poll() and ReadTelemetry() never block.
 */
class IHardwareController
{
public:
	virtual ~IHardwareController() = default;

	/** Stable identifier of this controller instance, for logs and the panel. */
	virtual FName GetControllerName() const = 0;

	/** The kind of device this implementation drives. */
	virtual EHardwareControllerKind GetKind() const = 0;

	/** What this controller can actually do — consumers must check before commanding. */
	virtual FHardwareCapabilities GetCapabilities() const = 0;

	/**
	 * Opens the link. Implementations validate the configuration first and must
	 * be safe to call again after a failure. Returns an error describing why the
	 * link could not be opened.
	 */
	virtual TSisaResult<bool> Connect(const FHardwareConnectionConfig& Config) = 0;

	/** Closes the link. Safe to call when already disconnected. */
	virtual void Disconnect() = 0;

	/** Current link state. */
	virtual EHardwareConnectionState GetConnectionState() const = 0;

	/**
	 * Commands the mount to a laying, in degrees (azimuth clockwise from true
	 * North, elevation above the horizon). Fails if the link is down or the
	 * controller has no laying control.
	 */
	virtual TSisaResult<bool> CommandLaying(double AzimuthDegrees, double ElevationDegrees) = 0;

	/** Halts motion, leaving the tube where it is. */
	virtual TSisaResult<bool> StopMotion() = 0;

	/** Actuates the physical trigger. Fails if the controller has no fire trigger. */
	virtual TSisaResult<bool> TriggerFire() = 0;

	/** The most recent telemetry sample. Never blocks. */
	virtual FHardwareTelemetry ReadTelemetry() const = 0;

	/**
	 * Advances the controller by DeltaSeconds: drains incoming frames, refreshes
	 * telemetry, and (for the simulated controller) advances the mount model.
	 * Driven by UHardwareSubsystem once per frame.
	 */
	virtual void Poll(double DeltaSeconds) = 0;
};
