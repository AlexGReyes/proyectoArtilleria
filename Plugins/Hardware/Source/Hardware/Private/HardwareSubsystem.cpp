#include "HardwareSubsystem.h"
#include "Hardware.h"
#include "HardwareEvents.h"
#include "IHardwareController.h"
#include "SimulatedHardwareController.h"
#include "SisaEventBusSubsystem.h"
#include "Engine/GameInstance.h"

void UHardwareSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Phase 1 default. Phase 2 modules register their own factories on startup
	// and Simulation keeps calling Connect() unchanged.
	RegisterFactory(EHardwareControllerKind::Simulated,
		[]() -> TSharedRef<IHardwareController> { return MakeShared<FSimulatedHardwareController>(); });

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UHardwareSubsystem::HandleTick));
}

void UHardwareSubsystem::Deinitialize()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	Disconnect();
	Factories.Empty();

	Super::Deinitialize();
}

void UHardwareSubsystem::RegisterFactory(EHardwareControllerKind Kind, FHardwareControllerFactory Factory)
{
	if (!Factory)
	{
		return;
	}

	Factories.Add(Kind, MoveTemp(Factory));
}

TSisaResult<bool> UHardwareSubsystem::Connect(const FHardwareConnectionConfig& Config)
{
	const FHardwareControllerFactory* Factory = Factories.Find(Config.Kind);
	if (!Factory)
	{
		return TSisaResult<bool>::Err(
			TEXT("Hardware.NoFactoryRegistered"),
			TEXT("No controller implementation is registered for the requested device kind (Phase 2 registers the real ones)."));
	}

	// Replace whatever was active first, so a failed reconnect never leaves two
	// controllers holding the same port.
	Disconnect();

	const EHardwareConnectionState PreviousState = LastObservedState;
	TSharedRef<IHardwareController> Controller = (*Factory)();

	const TSisaResult<bool> ConnectResult = Controller->Connect(Config);
	if (ConnectResult.IsError())
	{
		// The failed controller is not kept, so the subsystem stays disconnected;
		// the event still names it and carries the reason.
		LastObservedState = EHardwareConnectionState::Disconnected;
		PublishConnectionEvent(Controller, PreviousState, ConnectResult.GetError().Message);

		UE_LOG(LogSisaHardware, Warning, TEXT("Connect failed: %s (%s)"),
			*ConnectResult.GetError().Message, *ConnectResult.GetError().Code.ToString());
		return ConnectResult;
	}

	ActiveController = Controller;
	LastObservedState = Controller->GetConnectionState();
	SecondsSinceTelemetryPublish = 0.0;
	PublishConnectionEvent(ActiveController, PreviousState, FString());

	return TSisaResult<bool>::Ok(true);
}

bool UHardwareSubsystem::ConnectToHardware(const FHardwareConnectionConfig& Config, FSisaError& OutError)
{
	const TSisaResult<bool> Result = Connect(Config);
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutError = FSisaError();
	return true;
}

void UHardwareSubsystem::Disconnect()
{
	if (!ActiveController.IsValid())
	{
		return;
	}

	const EHardwareConnectionState PreviousState = ActiveController->GetConnectionState();
	ActiveController->Disconnect();
	LastObservedState = ActiveController->GetConnectionState();
	PublishConnectionEvent(ActiveController, PreviousState, FString());

	ActiveController.Reset();
	LastObservedState = EHardwareConnectionState::Disconnected;
}

EHardwareConnectionState UHardwareSubsystem::GetConnectionState() const
{
	return ActiveController.IsValid() ? ActiveController->GetConnectionState() : EHardwareConnectionState::Disconnected;
}

FHardwareCapabilities UHardwareSubsystem::GetCapabilities() const
{
	return ActiveController.IsValid() ? ActiveController->GetCapabilities() : FHardwareCapabilities();
}

FHardwareTelemetry UHardwareSubsystem::GetTelemetry() const
{
	return ActiveController.IsValid() ? ActiveController->ReadTelemetry() : FHardwareTelemetry();
}

TSisaResult<bool> UHardwareSubsystem::CommandLaying(double AzimuthDegrees, double ElevationDegrees)
{
	if (!ActiveController.IsValid())
	{
		return TSisaResult<bool>::Err(TEXT("Hardware.NoController"), TEXT("No hardware controller is connected."));
	}

	if (!ActiveController->GetCapabilities().bLayingControl)
	{
		return TSisaResult<bool>::Err(
			TEXT("Hardware.NoLayingControl"),
			TEXT("The connected controller cannot drive the mount."));
	}

	return ActiveController->CommandLaying(AzimuthDegrees, ElevationDegrees);
}

bool UHardwareSubsystem::CommandLayingWithReason(double AzimuthDegrees, double ElevationDegrees, FSisaError& OutError)
{
	const TSisaResult<bool> Result = CommandLaying(AzimuthDegrees, ElevationDegrees);
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutError = FSisaError();
	return true;
}

bool UHardwareSubsystem::StopMotion(FSisaError& OutError)
{
	if (!ActiveController.IsValid())
	{
		OutError = FSisaError(TEXT("Hardware.NoController"), TEXT("No hardware controller is connected."));
		return false;
	}

	const TSisaResult<bool> Result = ActiveController->StopMotion();
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutError = FSisaError();
	return true;
}

bool UHardwareSubsystem::TriggerFire(FSisaError& OutError)
{
	if (!ActiveController.IsValid())
	{
		OutError = FSisaError(TEXT("Hardware.NoController"), TEXT("No hardware controller is connected."));
		return false;
	}

	if (!ActiveController->GetCapabilities().bFireTrigger)
	{
		OutError = FSisaError(TEXT("Hardware.NoFireTrigger"), TEXT("The connected controller has no fire trigger."));
		return false;
	}

	const TSisaResult<bool> Result = ActiveController->TriggerFire();
	if (Result.IsError())
	{
		OutError = Result.GetError();
		return false;
	}

	OutError = FSisaError();
	return true;
}

bool UHardwareSubsystem::HandleTick(float DeltaSeconds)
{
	PollControllers(DeltaSeconds);
	return true; // keep ticking
}

void UHardwareSubsystem::PollControllers(double DeltaSeconds)
{
	if (!ActiveController.IsValid())
	{
		return;
	}

	ActiveController->Poll(DeltaSeconds);

	// A controller can change state on its own (a real link dropping, or
	// SimulateLinkFailure); surface that without waiting for the next command.
	const EHardwareConnectionState CurrentState = ActiveController->GetConnectionState();
	if (CurrentState != LastObservedState)
	{
		const EHardwareConnectionState PreviousState = LastObservedState;
		LastObservedState = CurrentState;
		PublishConnectionEvent(ActiveController, PreviousState, FString());
	}

	if (CurrentState != EHardwareConnectionState::Connected)
	{
		return;
	}

	SecondsSinceTelemetryPublish += DeltaSeconds;
	if (SecondsSinceTelemetryPublish < TelemetryPublishIntervalSeconds)
	{
		return;
	}
	SecondsSinceTelemetryPublish = 0.0;

	if (const UGameInstance* OwningGameInstance = GetGameInstance())
	{
		if (USisaEventBusSubsystem* Bus = OwningGameInstance->GetSubsystem<USisaEventBusSubsystem>())
		{
			FHardwareTelemetryEvent Event;
			Event.ControllerName = ActiveController->GetControllerName();
			Event.Telemetry = ActiveController->ReadTelemetry();
			Bus->Publish(SisaHardwareChannels::Telemetry, Event);
		}
	}
}

void UHardwareSubsystem::PublishConnectionEvent(
	const TSharedPtr<IHardwareController>& Controller,
	EHardwareConnectionState PreviousState,
	const FString& ErrorMessage)
{
	const UGameInstance* OwningGameInstance = GetGameInstance();
	USisaEventBusSubsystem* Bus = OwningGameInstance ? OwningGameInstance->GetSubsystem<USisaEventBusSubsystem>() : nullptr;
	if (!Bus)
	{
		return;
	}

	FHardwareConnectionEvent Event;
	Event.ControllerName = Controller.IsValid() ? Controller->GetControllerName() : NAME_None;
	Event.Kind = Controller.IsValid() ? Controller->GetKind() : EHardwareControllerKind::Simulated;
	// The controller's own view wins: after a failed Connect it reports Error even
	// though the subsystem itself stays disconnected.
	Event.State = Controller.IsValid() ? Controller->GetConnectionState() : LastObservedState;
	Event.PreviousState = PreviousState;
	Event.ErrorMessage = ErrorMessage;
	Bus->Publish(SisaHardwareChannels::ConnectionChanged, Event);
}
