// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachineSymbol.h"
#include "StateMachineBranch.generated.h"

class UStateMachineState;

/**
 * 
 */
UCLASS(EditInlineNew)
class DUNGEONMAKER_API UStateMachineBranch : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Returns Destination State on success, or NULL on failure. For subclasses, OutDataIndex may be something other
	than 1, if a branch is made to consume multiple inputs. */
	UFUNCTION(BlueprintCallable, Category = "State Machine")
	virtual UStateMachineState* TryBranch(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource,
	int32 DataIndex, int32& OutDataIndex);
	// Where we will go if this branch is taken. If this is null, the branch is ignored.
	UPROPERTY(EditAnywhere)
	UStateMachineState* DestinationState;
	// This inverts the branch -- instead of looking FOR something, it's looking for anything BUT something.
	UPROPERTY(EditAnywhere)
	bool bReverseInputTest;
	// All acceptable inputs. The current input atom must be on this list.
	UPROPERTY(EditAnywhere)
	TArray<UStateMachineSymbol*> AcceptableInputs;
};
