#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SisaEventBusSubsystem.generated.h"

/**
 * Type-erased base so channels of different payload types can live in one
 * TMap. Never used polymorphically beyond storage + destruction.
 */
class FSisaEventChannelBase
{
public:
	virtual ~FSisaEventChannelBase() = default;
};

/**
 * A single event channel for payload type T: a set of callbacks keyed by
 * FDelegateHandle so callers can Unsubscribe individually.
 */
template <typename T>
class TSisaEventChannel : public FSisaEventChannelBase
{
public:
	FDelegateHandle Subscribe(TFunction<void(const T&)> Callback)
	{
		FDelegateHandle Handle(FDelegateHandle::GenerateNewHandle);
		Callbacks.Add(Handle, MoveTemp(Callback));
		return Handle;
	}

	void Unsubscribe(FDelegateHandle Handle)
	{
		Callbacks.Remove(Handle);
	}

	void Publish(const T& Payload)
	{
		// Copy the values out first: a callback is allowed to Subscribe/Unsubscribe
		// in reaction to the event without invalidating the map we're iterating.
		TArray<TFunction<void(const T&)>> CallbacksSnapshot;
		Callbacks.GenerateValueArray(CallbacksSnapshot);
		for (const TFunction<void(const T&)>& Callback : CallbacksSnapshot)
		{
			Callback(Payload);
		}
	}

private:
	TMap<FDelegateHandle, TFunction<void(const T&)>> Callbacks;
};

/**
 * Minimal pub/sub bus shared across SISA plugins so that, for example, the
 * Simulation plugin can react to an Artillery fire event without including
 * any Artillery header (Observer pattern, keeps plugins decoupled).
 *
 * Channels are identified by FName and typed only by convention: publishing
 * type T2 on a channel that was first subscribed with type T1 is undefined
 * behavior (there is no RTTI-based check). Callers must document and respect
 * one payload type per channel name.
 */
UCLASS()
class SISACORE_API USisaEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	template <typename T>
	FDelegateHandle Subscribe(FName Channel, TFunction<void(const T&)> Callback)
	{
		TSharedPtr<FSisaEventChannelBase>& ChannelBase = Channels.FindOrAdd(Channel);
		if (!ChannelBase.IsValid())
		{
			ChannelBase = MakeShared<TSisaEventChannel<T>>();
		}
		return StaticCastSharedPtr<TSisaEventChannel<T>>(ChannelBase)->Subscribe(MoveTemp(Callback));
	}

	template <typename T>
	void Unsubscribe(FName Channel, FDelegateHandle Handle)
	{
		if (TSharedPtr<FSisaEventChannelBase>* ChannelBase = Channels.Find(Channel))
		{
			StaticCastSharedPtr<TSisaEventChannel<T>>(*ChannelBase)->Unsubscribe(Handle);
		}
	}

	template <typename T>
	void Publish(FName Channel, const T& Payload)
	{
		if (TSharedPtr<FSisaEventChannelBase>* ChannelBase = Channels.Find(Channel))
		{
			StaticCastSharedPtr<TSisaEventChannel<T>>(*ChannelBase)->Publish(Payload);
		}
	}

	virtual void Deinitialize() override
	{
		Channels.Empty();
		Super::Deinitialize();
	}

private:
	TMap<FName, TSharedPtr<FSisaEventChannelBase>> Channels;
};
