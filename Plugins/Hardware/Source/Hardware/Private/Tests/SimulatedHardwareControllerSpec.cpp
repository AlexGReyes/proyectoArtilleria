#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SimulatedHardwareController.h"

DEFINE_SPEC(
	FSimulatedHardwareControllerSpec,
	"SISA.Hardware.SimulatedController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	FHardwareConnectionConfig SimulatedConfig()
	{
		FHardwareConnectionConfig Config;
		Config.Kind = EHardwareControllerKind::Simulated;
		Config.Transport = EHardwareTransport::None;
		return Config;
	}

	TSharedRef<FSimulatedHardwareController> NewConnectedController()
	{
		TSharedRef<FSimulatedHardwareController> Controller = MakeShared<FSimulatedHardwareController>();
		Controller->Connect(SimulatedConfig());
		return Controller;
	}

	/** Drives the controller for a whole number of 0.1 s polls. */
	void PollFor(FSimulatedHardwareController& Controller, double Seconds)
	{
		const int32 Steps = FMath::RoundToInt(Seconds / 0.1);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Controller.Poll(0.1);
		}
	}
}

void FSimulatedHardwareControllerSpec::Define()
{
	Describe("Connection lifecycle", [this]()
	{
		It("starts disconnected", [this]()
		{
			FSimulatedHardwareController Controller;
			TestEqual(TEXT("Disconnected"), Controller.GetConnectionState(), EHardwareConnectionState::Disconnected);
			TestFalse(TEXT("Encoders invalid while down"), Controller.ReadTelemetry().bEncoderValid);
		});

		It("connects with a valid simulated configuration", [this]()
		{
			FSimulatedHardwareController Controller;
			TestTrue(TEXT("Connected"), Controller.Connect(SimulatedConfig()).IsOk());
			TestEqual(TEXT("State"), Controller.GetConnectionState(), EHardwareConnectionState::Connected);
		});

		It("refuses a configuration for a physical device", [this]()
		{
			FHardwareConnectionConfig Config;
			Config.Kind = EHardwareControllerKind::Arduino;
			Config.Transport = EHardwareTransport::Serial;
			Config.PortName = TEXT("COM3");

			FSimulatedHardwareController Controller;
			const TSisaResult<bool> Result = Controller.Connect(Config);
			TestTrue(TEXT("Failed"), Result.IsError());
			TestEqual(TEXT("Reports the mismatch"), Result.GetError().Code, FName(TEXT("Hardware.KindMismatch")));
			TestEqual(TEXT("State is Error"), Controller.GetConnectionState(), EHardwareConnectionState::Error);
		});

		It("refuses an invalid configuration", [this]()
		{
			FHardwareConnectionConfig Config = SimulatedConfig();
			Config.TimeoutMilliseconds = 0;

			FSimulatedHardwareController Controller;
			TestTrue(TEXT("Failed"), Controller.Connect(Config).IsError());
		});

		It("leaves the tube where it is on disconnect", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SnapToLaying(120.0, 30.0);
			Controller->Disconnect();

			TestEqual(TEXT("Disconnected"), Controller->GetConnectionState(), EHardwareConnectionState::Disconnected);
			TestEqual(TEXT("Azimuth held"), Controller->ReadTelemetry().AzimuthDegrees, 120.0, 1e-9);
			TestFalse(TEXT("Telemetry flagged invalid"), Controller->ReadTelemetry().bEncoderValid);
		});

		It("can be forced into the error state", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SimulateLinkFailure(TEXT("cable unplugged"));
			TestEqual(TEXT("Error"), Controller->GetConnectionState(), EHardwareConnectionState::Error);
			TestTrue(TEXT("Commands now fail"), Controller->CommandLaying(10.0, 10.0).IsError());
		});
	});

	Describe("Commands while disconnected", [this]()
	{
		It("rejects every command", [this]()
		{
			FSimulatedHardwareController Controller;
			TestEqual(TEXT("Laying"), Controller.CommandLaying(90.0, 30.0).GetError().Code, FName(TEXT("Hardware.NotConnected")));
			TestEqual(TEXT("Stop"), Controller.StopMotion().GetError().Code, FName(TEXT("Hardware.NotConnected")));
			TestEqual(TEXT("Fire"), Controller.TriggerFire().GetError().Code, FName(TEXT("Hardware.NotConnected")));
		});

		It("does not move when polled", [this]()
		{
			FSimulatedHardwareController Controller;
			PollFor(Controller, 5.0);
			TestEqual(TEXT("Still at zero"), Controller.ReadTelemetry().AzimuthDegrees, 0.0, 1e-9);
		});
	});

	Describe("Mount motion", [this]()
	{
		It("slews toward the commanded laying at the configured rates", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SetTraverseRate(10.0);
			Controller->SetElevationRate(4.0);

			TestTrue(TEXT("Accepted"), Controller->CommandLaying(90.0, 40.0).IsOk());
			PollFor(*Controller, 1.0);

			const FHardwareTelemetry Sample = Controller->ReadTelemetry();
			TestEqual(TEXT("Azimuth after 1 s"), Sample.AzimuthDegrees, 10.0, 1e-6);
			TestEqual(TEXT("Elevation after 1 s"), Sample.ElevationDegrees, 4.0, 1e-6);
			TestTrue(TEXT("Reported as moving"), Sample.bMoving);
		});

		It("arrives without overshooting and stops reporting motion", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SetTraverseRate(10.0);
			Controller->SetElevationRate(4.0);
			Controller->CommandLaying(90.0, 40.0);

			PollFor(*Controller, 15.0);

			const FHardwareTelemetry Sample = Controller->ReadTelemetry();
			TestEqual(TEXT("Azimuth reached"), Sample.AzimuthDegrees, 90.0, 1e-6);
			TestEqual(TEXT("Elevation reached"), Sample.ElevationDegrees, 40.0, 1e-6);
			TestFalse(TEXT("No longer moving"), Sample.bMoving);
		});

		It("clamps an elevation order to the mount limits and says so", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SetElevationLimits(-5.0, 45.0);

			const TSisaResult<bool> Result = Controller->CommandLaying(0.0, 80.0);
			TestTrue(TEXT("Reported as clamped"), Result.IsError());
			TestEqual(TEXT("Clamp error code"), Result.GetError().Code, FName(TEXT("Hardware.ElevationClamped")));

			PollFor(*Controller, 60.0);
			TestEqual(TEXT("Stopped at the stop"), Controller->ReadTelemetry().ElevationDegrees, 45.0, 1e-6);
		});

		It("halts where it is on StopMotion", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SetTraverseRate(10.0);
			Controller->CommandLaying(180.0, 0.0);
			PollFor(*Controller, 2.0);

			TestTrue(TEXT("Stopped"), Controller->StopMotion().IsOk());
			const double AzimuthAtStop = Controller->ReadTelemetry().AzimuthDegrees;

			PollFor(*Controller, 5.0);
			TestEqual(TEXT("Did not resume"), Controller->ReadTelemetry().AzimuthDegrees, AzimuthAtStop, 1e-9);
			TestFalse(TEXT("Not moving"), Controller->ReadTelemetry().bMoving);
		});
	});

	Describe("Telemetry and trigger", [this]()
	{
		It("reports the configured position and a heading matching the tube", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->SetSimulatedPosition(FGeoCoordinate(4.71, -74.07, 2640.0));
			Controller->SnapToLaying(135.0, 20.0);

			const FHardwareTelemetry Sample = Controller->ReadTelemetry();
			TestTrue(TEXT("Position valid"), Sample.bPositionValid);
			TestEqual(TEXT("Latitude"), Sample.Position.Latitude, 4.71, 1e-9);
			TestTrue(TEXT("Heading valid"), Sample.bHeadingValid);
			TestEqual(TEXT("Heading follows the tube"), Sample.HeadingDegrees, 135.0, 1e-9);
			TestTrue(TEXT("Attitude valid"), Sample.bAttitudeValid);
		});

		It("advertises every capability", [this]()
		{
			const FHardwareCapabilities Capabilities = FSimulatedHardwareController().GetCapabilities();
			TestTrue(TEXT("Laying control"), Capabilities.bLayingControl);
			TestTrue(TEXT("Encoders"), Capabilities.bEncoders);
			TestTrue(TEXT("IMU"), Capabilities.bImu);
			TestTrue(TEXT("GNSS"), Capabilities.bGnss);
			TestTrue(TEXT("Compass"), Capabilities.bCompass);
			TestTrue(TEXT("Fire trigger"), Capabilities.bFireTrigger);
		});

		It("counts trigger pulls and resets the count on reconnect", [this]()
		{
			TSharedRef<FSimulatedHardwareController> Controller = NewConnectedController();
			Controller->TriggerFire();
			Controller->TriggerFire();
			TestEqual(TEXT("Two shots"), Controller->ReadTelemetry().FireCount, 2);

			Controller->Disconnect();
			Controller->Connect(SimulatedConfig());
			TestEqual(TEXT("Reset"), Controller->ReadTelemetry().FireCount, 0);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
