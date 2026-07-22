#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HardwareTypes.h"
#include "SisaResult.h"

DEFINE_SPEC(
	FHardwareConfigSpec,
	"SISA.Hardware.Config",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	bool HasError(const TArray<FSisaError>& Errors, const TCHAR* Code)
	{
		return Errors.ContainsByPredicate(
			[Code](const FSisaError& Error) { return Error.Code == FName(Code); });
	}

	FHardwareConnectionConfig MakeSimulatedConfig()
	{
		FHardwareConnectionConfig Config;
		Config.Kind = EHardwareControllerKind::Simulated;
		Config.Transport = EHardwareTransport::None;
		return Config;
	}

	FHardwareConnectionConfig MakeSerialConfig(EHardwareTransport Transport)
	{
		FHardwareConnectionConfig Config;
		Config.Kind = EHardwareControllerKind::Arduino;
		Config.Transport = Transport;
		Config.PortName = TEXT("COM3");
		Config.BaudRate = 115200;
		return Config;
	}

	FHardwareConnectionConfig MakeNetworkConfig(EHardwareTransport Transport)
	{
		FHardwareConnectionConfig Config;
		Config.Kind = EHardwareControllerKind::Plc;
		Config.Transport = Transport;
		Config.Host = TEXT("192.168.1.50");
		Config.NetworkPort = 502;
		return Config;
	}
}

void FHardwareConfigSpec::Define()
{
	Describe("Simulated configuration", [this]()
	{
		It("accepts the simulated kind with no transport", [this]()
		{
			TArray<FSisaError> Errors;
			TestTrue(TEXT("Valid"), MakeSimulatedConfig().Validate(Errors));
			TestEqual(TEXT("No errors"), Errors.Num(), 0);
		});

		It("rejects a simulated controller asked to use a real transport", [this]()
		{
			FHardwareConnectionConfig Config = MakeSimulatedConfig();
			Config.Transport = EHardwareTransport::Rs485;
			Config.PortName = TEXT("COM3");

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Config.Validate(Errors));
			TestTrue(TEXT("Reports the mismatch"), HasError(Errors, TEXT("Hardware.SimulatedNeedsNoTransport")));
		});

		It("rejects a physical controller with no transport", [this]()
		{
			FHardwareConnectionConfig Config;
			Config.Kind = EHardwareControllerKind::Esp32;
			Config.Transport = EHardwareTransport::None;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Config.Validate(Errors));
			TestTrue(TEXT("Reports the missing transport"), HasError(Errors, TEXT("Hardware.TransportRequired")));
		});
	});

	Describe("Serial family", [this]()
	{
		It("accepts serial, RS-232, RS-485 and CAN with a port and baud rate", [this]()
		{
			const EHardwareTransport Transports[] = {
				EHardwareTransport::Serial,
				EHardwareTransport::Rs232,
				EHardwareTransport::Rs485,
				EHardwareTransport::CanBus
			};

			for (const EHardwareTransport Transport : Transports)
			{
				TArray<FSisaError> Errors;
				TestTrue(TEXT("Valid serial-family config"), MakeSerialConfig(Transport).Validate(Errors));
			}
		});

		It("rejects a missing port name and a non-positive baud rate", [this]()
		{
			FHardwareConnectionConfig Config = MakeSerialConfig(EHardwareTransport::Rs485);
			Config.PortName.Empty();
			Config.BaudRate = 0;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Config.Validate(Errors));
			TestTrue(TEXT("Reports the port"), HasError(Errors, TEXT("Hardware.MissingPortName")));
			TestTrue(TEXT("Reports the baud rate"), HasError(Errors, TEXT("Hardware.InvalidBaudRate")));
		});
	});

	Describe("Network family", [this]()
	{
		It("accepts TCP/IP, UDP and Ethernet with a host and port", [this]()
		{
			const EHardwareTransport Transports[] = {
				EHardwareTransport::TcpIp,
				EHardwareTransport::Udp,
				EHardwareTransport::Ethernet
			};

			for (const EHardwareTransport Transport : Transports)
			{
				TArray<FSisaError> Errors;
				TestTrue(TEXT("Valid network config"), MakeNetworkConfig(Transport).Validate(Errors));
			}
		});

		It("rejects a missing host and an out-of-range port", [this]()
		{
			FHardwareConnectionConfig Config = MakeNetworkConfig(EHardwareTransport::TcpIp);
			Config.Host.Empty();
			Config.NetworkPort = 70000;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Config.Validate(Errors));
			TestTrue(TEXT("Reports the host"), HasError(Errors, TEXT("Hardware.MissingHost")));
			TestTrue(TEXT("Reports the port"), HasError(Errors, TEXT("Hardware.InvalidNetworkPort")));
		});
	});

	Describe("Common fields", [this]()
	{
		It("rejects a negative device address and a non-positive timeout", [this]()
		{
			FHardwareConnectionConfig Config = MakeSerialConfig(EHardwareTransport::Rs485);
			Config.DeviceAddress = -1;
			Config.TimeoutMilliseconds = 0;

			TArray<FSisaError> Errors;
			TestFalse(TEXT("Invalid"), Config.Validate(Errors));
			TestTrue(TEXT("Reports the address"), HasError(Errors, TEXT("Hardware.InvalidDeviceAddress")));
			TestTrue(TEXT("Reports the timeout"), HasError(Errors, TEXT("Hardware.InvalidTimeout")));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
