#include "HardwareTypes.h"
#include "SisaResult.h"

bool FHardwareConnectionConfig::Validate(TArray<FSisaError>& OutErrors) const
{
	const int32 InitialErrorCount = OutErrors.Num();

	const bool bIsSerialFamily =
		Transport == EHardwareTransport::Serial ||
		Transport == EHardwareTransport::Rs232 ||
		Transport == EHardwareTransport::Rs485 ||
		Transport == EHardwareTransport::CanBus;

	const bool bIsNetworkFamily =
		Transport == EHardwareTransport::TcpIp ||
		Transport == EHardwareTransport::Udp ||
		Transport == EHardwareTransport::Ethernet;

	if (Kind == EHardwareControllerKind::Simulated && Transport != EHardwareTransport::None)
	{
		OutErrors.Add(FSisaError(
			TEXT("Hardware.SimulatedNeedsNoTransport"),
			TEXT("The simulated controller performs no I/O; its transport must be None.")));
	}

	if (Kind != EHardwareControllerKind::Simulated && Transport == EHardwareTransport::None)
	{
		OutErrors.Add(FSisaError(
			TEXT("Hardware.TransportRequired"),
			TEXT("A physical controller requires a transport other than None.")));
	}

	if (bIsSerialFamily)
	{
		if (PortName.IsEmpty())
		{
			OutErrors.Add(FSisaError(
				TEXT("Hardware.MissingPortName"),
				TEXT("Serial, RS-232, RS-485 and CAN links require a port name (e.g. \"COM3\").")));
		}
		if (BaudRate <= 0)
		{
			OutErrors.Add(FSisaError(TEXT("Hardware.InvalidBaudRate"), TEXT("Baud rate must be greater than zero.")));
		}
	}

	if (bIsNetworkFamily)
	{
		if (Host.IsEmpty())
		{
			OutErrors.Add(FSisaError(TEXT("Hardware.MissingHost"), TEXT("Network links require a host name or IP address.")));
		}
		if (NetworkPort <= 0 || NetworkPort > 65535)
		{
			OutErrors.Add(FSisaError(TEXT("Hardware.InvalidNetworkPort"), TEXT("Network port must be within [1, 65535].")));
		}
	}

	if (DeviceAddress < 0)
	{
		OutErrors.Add(FSisaError(TEXT("Hardware.InvalidDeviceAddress"), TEXT("Device address cannot be negative.")));
	}

	if (TimeoutMilliseconds <= 0)
	{
		OutErrors.Add(FSisaError(TEXT("Hardware.InvalidTimeout"), TEXT("Timeout must be greater than zero milliseconds.")));
	}

	return OutErrors.Num() == InitialErrorCount;
}
