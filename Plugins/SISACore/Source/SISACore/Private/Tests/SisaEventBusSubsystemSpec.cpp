#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "SisaEventBusSubsystem.h"

DEFINE_SPEC(
	FSisaEventBusSubsystemSpec,
	"SISA.Core.EventBus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	// USisaEventBusSubsystem is a UGameInstanceSubsystem, which UCLASS(Within=GameInstance)
	// constrains to only ever be constructed with a UGameInstance Outer. NewObject<T>() with
	// no Outer defaults to the transient package and trips an ensure at runtime, so every test
	// below goes through this helper instead.
	USisaEventBusSubsystem* NewTestEventBus()
	{
		UGameInstance* OwningGameInstance = NewObject<UGameInstance>();
		return NewObject<USisaEventBusSubsystem>(OwningGameInstance);
	}
}

void FSisaEventBusSubsystemSpec::Define()
{
	Describe("Publish/Subscribe", [this]()
	{
		It("delivers the payload to a single subscriber", [this]()
		{
			USisaEventBusSubsystem* Bus = NewTestEventBus();
			int32 Received = 0;
			Bus->Subscribe<int32>(TEXT("Test.Channel"), [&Received](const int32& Value) { Received = Value; });
			Bus->Publish<int32>(TEXT("Test.Channel"), 42);
			TestEqual(TEXT("Received value"), Received, 42);
		});

		It("delivers to every subscriber on the same channel", [this]()
		{
			USisaEventBusSubsystem* Bus = NewTestEventBus();
			int32 CallCount = 0;
			Bus->Subscribe<FString>(TEXT("Test.Channel"), [&CallCount](const FString&) { ++CallCount; });
			Bus->Subscribe<FString>(TEXT("Test.Channel"), [&CallCount](const FString&) { ++CallCount; });
			Bus->Publish<FString>(TEXT("Test.Channel"), TEXT("hello"));
			TestEqual(TEXT("Call count"), CallCount, 2);
		});

		It("stops delivering after Unsubscribe", [this]()
		{
			USisaEventBusSubsystem* Bus = NewTestEventBus();
			int32 Received = 0;
			FDelegateHandle Handle = Bus->Subscribe<int32>(
				TEXT("Test.Channel"),
				[&Received](const int32& Value) { Received = Value; });
			Bus->Unsubscribe<int32>(TEXT("Test.Channel"), Handle);
			Bus->Publish<int32>(TEXT("Test.Channel"), 99);
			TestEqual(TEXT("Received value after unsubscribe"), Received, 0);
		});

		It("does nothing when publishing on a channel with no subscribers", [this]()
		{
			USisaEventBusSubsystem* Bus = NewTestEventBus();
			// Should not crash or assert even though "Unused.Channel" was never subscribed to.
			Bus->Publish<int32>(TEXT("Unused.Channel"), 1);
			TestTrue(TEXT("Reached end without failure"), true);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
