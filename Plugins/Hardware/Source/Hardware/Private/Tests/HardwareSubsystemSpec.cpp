#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HardwareSubsystem.h"
#include "SimulatedHardwareController.h"
#include "Engine/GameInstance.h"

DEFINE_SPEC(
	FHardwareSubsystemSpec,
	"SISA.Hardware.Subsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	/**
	 * A UGameInstanceSubsystem built directly (Within=GameInstance outer), as in
	 * the SISACore and Ammunition specs. Initialize() is not run — it needs a
	 * live subsystem collection — so the test registers the simulated factory
	 * itself and drives PollControllers by hand instead of via the ticker.
	 */
	UHardwareSubsystem* NewTestSubsystem(bool bRegisterSimulatedFactory = true)
	{
		UGameInstance* OwningGameInstance = NewObject<UGameInstance>();
		UHardwareSubsystem* Subsystem = NewObject<UHardwareSubsystem>(OwningGameInstance);

		if (bRegisterSimulatedFactory)
		{
			Subsystem->RegisterFactory(EHardwareControllerKind::Simulated,
				[]() -> TSharedRef<IHardwareController> { return MakeShared<FSimulatedHardwareController>(); });
		}

		// Publish every poll so the throttling does not hide state changes here.
		Subsystem->TelemetryPublishIntervalSeconds = 0.0;
		return Subsystem;
	}

	FHardwareConnectionConfig SimulatedConfig()
	{
		FHardwareConnectionConfig Config;
		Config.Kind = EHardwareControllerKind::Simulated;
		Config.Transport = EHardwareTransport::None;
		return Config;
	}
}

void FHardwareSubsystemSpec::Define()
{
	Describe("Factories", [this]()
	{
		It("reports which kinds it can build", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			TestTrue(TEXT("Simulated available"), Subsystem->HasFactory(EHardwareControllerKind::Simulated));
			TestFalse(TEXT("Arduino not available in Phase 1"), Subsystem->HasFactory(EHardwareControllerKind::Arduino));
		});

		It("refuses to connect a kind with no registered factory", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();

			FHardwareConnectionConfig Config;
			Config.Kind = EHardwareControllerKind::Stm32;
			Config.Transport = EHardwareTransport::CanBus;
			Config.PortName = TEXT("can0");

			const TSisaResult<bool> Result = Subsystem->Connect(Config);
			TestTrue(TEXT("Failed"), Result.IsError());
			TestEqual(TEXT("Reports the missing factory"), Result.GetError().Code, FName(TEXT("Hardware.NoFactoryRegistered")));
			TestFalse(TEXT("Still disconnected"), Subsystem->IsConnected());
		});

		It("lets a later registration replace an earlier one", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->RegisterFactory(EHardwareControllerKind::Simulated,
				[]() -> TSharedRef<IHardwareController>
				{
					return MakeShared<FSimulatedHardwareController>(TEXT("ReplacementController"));
				});

			TestTrue(TEXT("Connected"), Subsystem->Connect(SimulatedConfig()).IsOk());
			TestEqual(TEXT("Built by the new factory"),
				Subsystem->GetController()->GetControllerName(), FName(TEXT("ReplacementController")));
		});
	});

	Describe("Connection", [this]()
	{
		It("connects the simulated controller and exposes its capabilities", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			TestTrue(TEXT("Connected"), Subsystem->Connect(SimulatedConfig()).IsOk());
			TestTrue(TEXT("IsConnected"), Subsystem->IsConnected());
			TestEqual(TEXT("State"), Subsystem->GetConnectionState(), EHardwareConnectionState::Connected);
			TestTrue(TEXT("Laying control advertised"), Subsystem->GetCapabilities().bLayingControl);
		});

		It("reports disconnected state and empty capabilities with no controller", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			TestEqual(TEXT("State"), Subsystem->GetConnectionState(), EHardwareConnectionState::Disconnected);
			TestFalse(TEXT("No laying control"), Subsystem->GetCapabilities().bLayingControl);
			TestFalse(TEXT("Telemetry invalid"), Subsystem->GetTelemetry().bEncoderValid);
			TestFalse(TEXT("Controller is null"), Subsystem->GetController().IsValid());
		});

		It("releases the controller on disconnect", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->Connect(SimulatedConfig());
			Subsystem->Disconnect();

			TestFalse(TEXT("Not connected"), Subsystem->IsConnected());
			TestFalse(TEXT("Controller released"), Subsystem->GetController().IsValid());
		});

		It("replaces the active controller when connecting again", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->Connect(SimulatedConfig());
			const TSharedPtr<IHardwareController> First = Subsystem->GetController();

			Subsystem->Connect(SimulatedConfig());
			TestNotEqual(TEXT("A new controller took over"), Subsystem->GetController().Get(), First.Get());
			TestEqual(TEXT("The old one was closed"), First->GetConnectionState(), EHardwareConnectionState::Disconnected);
		});
	});

	Describe("Commands", [this]()
	{
		It("fails every command with no controller connected", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			FSisaError Error;

			TestFalse(TEXT("Laying"), Subsystem->CommandLayingWithReason(90.0, 30.0, Error));
			TestEqual(TEXT("Reports no controller"), Error.Code, FName(TEXT("Hardware.NoController")));
			TestFalse(TEXT("Stop"), Subsystem->StopMotion(Error));
			TestFalse(TEXT("Fire"), Subsystem->TriggerFire(Error));
		});

		It("drives the mount through the controller", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->Connect(SimulatedConfig());

			FSisaError Error;
			TestTrue(TEXT("Commanded"), Subsystem->CommandLayingWithReason(45.0, 20.0, Error));

			for (int32 Step = 0; Step < 200; ++Step)
			{
				Subsystem->PollControllers(0.1);
			}

			const FHardwareTelemetry Sample = Subsystem->GetTelemetry();
			TestEqual(TEXT("Azimuth reached"), Sample.AzimuthDegrees, 45.0, 1e-6);
			TestEqual(TEXT("Elevation reached"), Sample.ElevationDegrees, 20.0, 1e-6);
			TestFalse(TEXT("Motion finished"), Sample.bMoving);
		});

		It("passes the trigger through and counts it in telemetry", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->Connect(SimulatedConfig());

			FSisaError Error;
			TestTrue(TEXT("Fired"), Subsystem->TriggerFire(Error));
			TestEqual(TEXT("Counted"), Subsystem->GetTelemetry().FireCount, 1);
		});

		It("does nothing when polled with no controller", [this]()
		{
			UHardwareSubsystem* Subsystem = NewTestSubsystem();
			Subsystem->PollControllers(0.1);
			TestFalse(TEXT("Still disconnected"), Subsystem->IsConnected());
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
