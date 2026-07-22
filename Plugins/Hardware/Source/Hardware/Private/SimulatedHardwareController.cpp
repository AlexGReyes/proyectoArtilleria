#include "SimulatedHardwareController.h"
#include "Hardware.h"
#include "SisaAngle.h"

FSimulatedHardwareController::FSimulatedHardwareController(FName InControllerName)
	: ControllerName(InControllerName)
{
	RefreshTelemetry(0.0);
}

void FSimulatedHardwareController::SetElevationLimits(double MinDegrees, double MaxDegrees)
{
	MinElevationDegrees = FMath::Min(MinDegrees, MaxDegrees);
	MaxElevationDegrees = FMath::Max(MinDegrees, MaxDegrees);

	CurrentElevationDegrees = FMath::Clamp(CurrentElevationDegrees, MinElevationDegrees, MaxElevationDegrees);
	TargetElevationDegrees = FMath::Clamp(TargetElevationDegrees, MinElevationDegrees, MaxElevationDegrees);
}

void FSimulatedHardwareController::SnapToLaying(double AzimuthDegrees, double ElevationDegrees)
{
	CurrentAzimuthDegrees = SisaAngle::Normalize360(AzimuthDegrees);
	CurrentElevationDegrees = FMath::Clamp(ElevationDegrees, MinElevationDegrees, MaxElevationDegrees);
	TargetAzimuthDegrees = CurrentAzimuthDegrees;
	TargetElevationDegrees = CurrentElevationDegrees;

	RefreshTelemetry(0.0);
}

void FSimulatedHardwareController::SimulateLinkFailure(const FString& Reason)
{
	ConnectionState = EHardwareConnectionState::Error;
	UE_LOG(LogSisaHardware, Warning, TEXT("Simulated controller '%s' link failure: %s"), *ControllerName.ToString(), *Reason);
}

FHardwareCapabilities FSimulatedHardwareController::GetCapabilities() const
{
	// The simulated mount stands in for a fully-instrumented piece, so it claims
	// every capability; a partially-fitted Phase 2 device will claim fewer.
	FHardwareCapabilities Capabilities;
	Capabilities.bLayingControl = true;
	Capabilities.bEncoders = true;
	Capabilities.bImu = true;
	Capabilities.bGnss = true;
	Capabilities.bCompass = true;
	Capabilities.bFireTrigger = true;
	return Capabilities;
}

TSisaResult<bool> FSimulatedHardwareController::Connect(const FHardwareConnectionConfig& Config)
{
	TArray<FSisaError> Errors;
	if (!Config.Validate(Errors))
	{
		ConnectionState = EHardwareConnectionState::Error;
		return TSisaResult<bool>::Err(Errors[0]);
	}

	if (Config.Kind != EHardwareControllerKind::Simulated)
	{
		ConnectionState = EHardwareConnectionState::Error;
		return TSisaResult<bool>::Err(
			TEXT("Hardware.KindMismatch"),
			TEXT("The simulated controller can only serve a configuration of kind Simulated."));
	}

	ConnectionState = EHardwareConnectionState::Connected;
	FireCount = 0;
	RefreshTelemetry(0.0);

	UE_LOG(LogSisaHardware, Log, TEXT("Simulated controller '%s' connected."), *ControllerName.ToString());
	return TSisaResult<bool>::Ok(true);
}

void FSimulatedHardwareController::Disconnect()
{
	if (ConnectionState != EHardwareConnectionState::Disconnected)
	{
		UE_LOG(LogSisaHardware, Log, TEXT("Simulated controller '%s' disconnected."), *ControllerName.ToString());
	}

	ConnectionState = EHardwareConnectionState::Disconnected;

	// The mount stops where it is: a real link dropping does not re-centre a tube.
	TargetAzimuthDegrees = CurrentAzimuthDegrees;
	TargetElevationDegrees = CurrentElevationDegrees;
	RefreshTelemetry(0.0);
}

TSisaResult<bool> FSimulatedHardwareController::RequireConnected(const TCHAR* Operation) const
{
	if (ConnectionState != EHardwareConnectionState::Connected)
	{
		return TSisaResult<bool>::Err(
			TEXT("Hardware.NotConnected"),
			FString::Printf(TEXT("%s requires an open link to controller '%s'."), Operation, *ControllerName.ToString()));
	}
	return TSisaResult<bool>::Ok(true);
}

TSisaResult<bool> FSimulatedHardwareController::CommandLaying(double AzimuthDegrees, double ElevationDegrees)
{
	const TSisaResult<bool> Connected = RequireConnected(TEXT("CommandLaying"));
	if (Connected.IsError())
	{
		return Connected;
	}

	TargetAzimuthDegrees = SisaAngle::Normalize360(AzimuthDegrees);

	// A real mount clamps to its stops rather than refusing the order; report
	// that it was clamped so the operator is not misled about where it will end up.
	const double ClampedElevation = FMath::Clamp(ElevationDegrees, MinElevationDegrees, MaxElevationDegrees);
	const bool bElevationClamped = !FMath::IsNearlyEqual(ClampedElevation, ElevationDegrees, UE_KINDA_SMALL_NUMBER);
	TargetElevationDegrees = ClampedElevation;

	if (bElevationClamped)
	{
		return TSisaResult<bool>::Err(
			TEXT("Hardware.ElevationClamped"),
			FString::Printf(
				TEXT("Elevation %.2f is outside the mount limits [%.2f, %.2f]; the tube will stop at %.2f."),
				ElevationDegrees, MinElevationDegrees, MaxElevationDegrees, ClampedElevation));
	}

	return TSisaResult<bool>::Ok(true);
}

TSisaResult<bool> FSimulatedHardwareController::StopMotion()
{
	const TSisaResult<bool> Connected = RequireConnected(TEXT("StopMotion"));
	if (Connected.IsError())
	{
		return Connected;
	}

	TargetAzimuthDegrees = CurrentAzimuthDegrees;
	TargetElevationDegrees = CurrentElevationDegrees;
	RefreshTelemetry(0.0);
	return TSisaResult<bool>::Ok(true);
}

TSisaResult<bool> FSimulatedHardwareController::TriggerFire()
{
	const TSisaResult<bool> Connected = RequireConnected(TEXT("TriggerFire"));
	if (Connected.IsError())
	{
		return Connected;
	}

	++FireCount;
	Telemetry.FireCount = FireCount;
	return TSisaResult<bool>::Ok(true);
}

void FSimulatedHardwareController::Poll(double DeltaSeconds)
{
	if (ConnectionState != EHardwareConnectionState::Connected || DeltaSeconds <= 0.0)
	{
		return;
	}

	ElapsedSeconds += DeltaSeconds;

	CurrentAzimuthDegrees = SisaAngle::StepTowards(
		CurrentAzimuthDegrees, TargetAzimuthDegrees, TraverseRateDegreesPerSecond * DeltaSeconds);
	CurrentElevationDegrees = SisaAngle::StepTowardsLinear(
		CurrentElevationDegrees, TargetElevationDegrees, ElevationRateDegreesPerSecond * DeltaSeconds);

	RefreshTelemetry(DeltaSeconds);
}

void FSimulatedHardwareController::RefreshTelemetry(double DeltaSeconds)
{
	const bool bConnected = ConnectionState == EHardwareConnectionState::Connected;

	Telemetry.TimestampSeconds = ElapsedSeconds;
	Telemetry.AzimuthDegrees = CurrentAzimuthDegrees;
	Telemetry.ElevationDegrees = CurrentElevationDegrees;
	Telemetry.bEncoderValid = bConnected;

	const bool bAzimuthMoving = !FMath::IsNearlyZero(
		SisaAngle::ShortestDelta(CurrentAzimuthDegrees, TargetAzimuthDegrees), UE_KINDA_SMALL_NUMBER);
	const bool bElevationMoving = !FMath::IsNearlyEqual(
		CurrentElevationDegrees, TargetElevationDegrees, UE_KINDA_SMALL_NUMBER);
	Telemetry.bMoving = bConnected && (bAzimuthMoving || bElevationMoving);

	Telemetry.Position = SimulatedPosition;
	Telemetry.bPositionValid = bConnected;

	// A real compass reads the platform heading; the simulated mount is laid on
	// its own azimuth, so the two coincide.
	Telemetry.HeadingDegrees = CurrentAzimuthDegrees;
	Telemetry.bHeadingValid = bConnected;

	// Level platform: the simulated piece is emplaced, so pitch/roll stay zero
	// and yaw follows the tube.
	Telemetry.Attitude = FRotator(CurrentElevationDegrees, CurrentAzimuthDegrees, 0.0);
	Telemetry.bAttitudeValid = bConnected;

	Telemetry.FireCount = FireCount;
}
