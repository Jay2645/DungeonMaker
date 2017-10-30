// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateMachineResult.generated.h"

class UStateMachineState;

/**
*
*/
UENUM(BlueprintType)
enum class EStateMachineCompletionType : uint8
{
	// This state is not marked as accepted.
	NotAccepted,
	// This state is explicitly accepted.
	Accepted,
	// This state is explicitly rejected.
	Rejected,
	// Our input string ran out of steps while the machine was still running.
	OutOfSteps			UMETA(Hidden)
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FStateMachineResult
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EStateMachineCompletionType CompletionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	const UStateMachineState* FinalState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DataIndex;

	FStateMachineResult()
	{
		FinalState = NULL;
		DataIndex = -1;
		CompletionType = EStateMachineCompletionType::OutOfSteps;
	}

	FStateMachineResult(const UStateMachineState* MachineFinalState, int32 FinalDataIndex, EStateMachineCompletionType FinalCompletionType)
	{
		FinalState = MachineFinalState;
		DataIndex = FinalDataIndex;
		CompletionType = FinalCompletionType;
	}
};