#pragma once

#include "CoreMinimal.h"
#include "SisaGeoCoordinate.h"
#include "HardwareTypes.generated.h"

struct FSisaError;

/** The kind of device on the other end of the link. */
UENUM(BlueprintType)
enum class EHardwareControllerKind : uint8
{
	/** No physical device: the Phase 1 in-engine mount model. */
	Simulated	UMETA(DisplayName = "Simulated"),
	Arduino		UMETA(DisplayName = "Arduino"),
	Esp32		UMETA(DisplayName = "ESP32"),
	Plc			UMETA(DisplayName = "PLC"),
	Stm32		UMETA(DisplayName = "STM32")
};

/** The physical/link layer used to reach the device. */
UENUM(BlueprintType)
enum class EHardwareTransport : uint8
{
	/** In-process, no I/O at all. */
	None	UMETA(DisplayName = "None (simulated)"),
	Serial	UMETA(DisplayName = "Serial"),
	Rs232	UMETA(DisplayName = "RS-232"),
	Rs485	UMETA(DisplayName = "RS-485"),
	CanBus	UMETA(DisplayName = "CAN Bus"),
	TcpIp	UMETA(DisplayName = "TCP/IP"),
	Udp		UMETA(DisplayName = "UDP"),
	Ethernet UMETA(DisplayName = "Ethernet")
};

/** Link state of a controller. */
UENUM(BlueprintType)
enum class EHardwareConnectionState : uint8
{
	Disconnected	UMETA(DisplayName = "Disconnected"),
	Connecting		UMETA(DisplayName = "Connecting"),
	Connected		UMETA(DisplayName = "Connected"),
	Error			UMETA(DisplayName = "Error")
};

/**
 * What a given controller can actually do. Declared as explicit booleans rather
 * than a bit mask so the panel can bind to each capability directly and so a
 * Phase 2 controller that only reads sensors (no motion) is expressible.
 */
USTRUCT(BlueprintType)
struct HARDWARE_API FHardwareCapabilities
{
	GENERATED_BODY()

	/** Can be commanded to a laying (azimuth/elevation motors or actuators). */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bLayingControl = false;

	/** Reports tube angles from absolute/incremental encoders. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bEncoders = false;

	/** Reports attitude (pitch/roll/yaw) from an IMU. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bImu = false;

	/** Reports a geographic position from a GNSS/GPS receiver. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bGnss = false;

	/** Reports a magnetic/true heading from an electronic compass. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bCompass = false;

	/** Exposes a physical fire trigger. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware")
	bool bFireTrigger = false;
};

/**
 * Everything needed to open a link. One struct covers every Phase 2 transport:
 * the serial family uses PortName/BaudRate, the network family uses Host/Port,
 * and CAN uses PortName plus DeviceAddress as the node id.
 */
USTRUCT(BlueprintType)
struct HARDWARE_API FHardwareConnectionConfig
{
	GENERATED_BODY()

	/** Which device implementation to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware")
	EHardwareControllerKind Kind = EHardwareControllerKind::Simulated;

	/** Which link layer to reach it over. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware")
	EHardwareTransport Transport = EHardwareTransport::None;

	/** Serial/CAN port name, e.g. "COM3" or "can0". Unused by network transports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware|Serial")
	FString PortName;

	/** Serial/CAN bit rate. Unused by network transports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware|Serial", meta = (ClampMin = "0"))
	int32 BaudRate = 115200;

	/** Host name or IP for network transports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware|Network")
	FString Host;

	/** TCP/UDP port for network transports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware|Network", meta = (ClampMin = "0", ClampMax = "65535"))
	int32 NetworkPort = 0;

	/** Bus/node address, for RS-485 and CAN where several devices share a line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware", meta = (ClampMin = "0"))
	int32 DeviceAddress = 0;

	/** How long to wait for the device before declaring the link failed, in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Hardware", meta = (ClampMin = "1"))
	int32 TimeoutMilliseconds = 1000;

	/**
	 * Validates the configuration against the requirements of the chosen
	 * transport. Returns true if the link could be attempted.
	 */
	bool Validate(TArray<FSisaError>& OutErrors) const;
};

/**
 * One snapshot of everything the device reports. Fields a controller cannot
 * measure are left at their defaults and flagged as invalid, so a consumer
 * never mistakes "not fitted" for "reads zero".
 */
USTRUCT(BlueprintType)
struct HARDWARE_API FHardwareTelemetry
{
	GENERATED_BODY()

	/** Engine time the sample was taken, in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	double TimestampSeconds = 0.0;

	/** Tube azimuth from the encoders, degrees clockwise from true North. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	double AzimuthDegrees = 0.0;

	/** Tube elevation from the encoders, degrees above the horizon. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	double ElevationDegrees = 0.0;

	/** True while either axis is still moving. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	bool bMoving = false;

	/** True when the angles above come from a fitted encoder. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	bool bEncoderValid = false;

	/** Position reported by the GNSS receiver. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	FGeoCoordinate Position;

	/** True when Position comes from a fitted receiver with a fix. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	bool bPositionValid = false;

	/** Heading from the electronic compass, degrees clockwise from true North. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	double HeadingDegrees = 0.0;

	/** True when HeadingDegrees comes from a fitted compass. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	bool bHeadingValid = false;

	/** Platform attitude from the IMU: Pitch/Roll/Yaw in degrees. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	FRotator Attitude = FRotator::ZeroRotator;

	/** True when Attitude comes from a fitted IMU. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	bool bAttitudeValid = false;

	/** Number of times the physical trigger has fired since the link was opened. */
	UPROPERTY(BlueprintReadOnly, Category = "SISA|Hardware|Telemetry")
	int32 FireCount = 0;
};
