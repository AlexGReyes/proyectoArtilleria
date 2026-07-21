#pragma once

#include "CoreMinimal.h"
#include "SisaResult.generated.h"

/**
 * An error carried by a failed TSisaResult<T>. Kept as a plain USTRUCT (not a
 * template) so it can be logged, displayed in UI, or sent over the event bus
 * on its own.
 */
USTRUCT(BlueprintType)
struct SISACORE_API FSisaError
{
	GENERATED_BODY()

	/** Short machine-readable identifier, e.g. "GIS.Mgrs.OutOfRange". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Error")
	FName Code;

	/** Human-readable detail, safe to show in the event console panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SISA|Error")
	FString Message;

	FSisaError() = default;

	FSisaError(FName InCode, FString InMessage)
		: Code(InCode)
		, Message(MoveTemp(InMessage))
	{
	}
};

/**
 * Explicit success/failure result, used instead of exceptions throughout
 * SISA's Application-layer services (Unreal C++ conventionally avoids
 * exceptions). Not a USTRUCT: Unreal Header Tool does not support templated
 * USTRUCTs, so this type is for internal C++ use between SISA plugins, not
 * for direct Blueprint exposure.
 */
template <typename T>
class TSisaResult
{
public:
	static TSisaResult<T> Ok(T InValue)
	{
		TSisaResult<T> Result;
		Result.bIsOk = true;
		Result.Value = MoveTemp(InValue);
		return Result;
	}

	static TSisaResult<T> Err(FSisaError InError)
	{
		TSisaResult<T> Result;
		Result.bIsOk = false;
		Result.Error = MoveTemp(InError);
		return Result;
	}

	static TSisaResult<T> Err(FName Code, const FString& Message)
	{
		return Err(FSisaError(Code, Message));
	}

	bool IsOk() const { return bIsOk; }
	bool IsError() const { return !bIsOk; }

	const T& GetValue() const
	{
		checkf(bIsOk, TEXT("TSisaResult::GetValue() called on an error result (%s: %s)"), *Error.Code.ToString(), *Error.Message);
		return Value.GetValue();
	}

	const FSisaError& GetError() const
	{
		checkf(!bIsOk, TEXT("TSisaResult::GetError() called on a success result"));
		return Error;
	}

private:
	bool bIsOk = false;
	TOptional<T> Value;
	FSisaError Error;
};
