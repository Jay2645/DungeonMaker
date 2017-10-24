// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachineBranch.h"
#include "StateMachineResult.h"
#include "StateMachineState.generated.h"

class UStateMachineSymbol;

/**
 * 
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UStateMachineState : public UDataAsset
{
public:
	GENERATED_BODY()
	UStateMachineState();
	UStateMachineState(const FObjectInitializer& Initializer);

	UPROPERTY(EditAnywhere, Instanced, Category = "Branches")
	TArray<UStateMachineBranch*> InstancedBranches;
	UPROPERTY(EditAnywhere, Category = "Branches")
	TArray<UStateMachineBranch*> SharedBranches;

	UFUNCTION(BlueprintCallable, Category = "State Machine")
	FStateMachineResult RunState(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource, int32 DataIndex = 0, int32 RemainingSteps = -1) const;

	UFUNCTION(BlueprintCallable, Category = "State Machine")
	FStateMachineResult RunStateWithBranches(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource, TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex = 0, int32 RemainingSteps = -1) const;


	UFUNCTION(BlueprintCallable, Category = "State Machine")
	bool Contains(const UStateMachineSymbol* Symbol) const;
protected:
	// Loop to the next state. Used when the current state isn't recognized for whatever reason.
	FStateMachineResult LoopStateWithBranches(const UObject* ReferenceObject,
		const TArray<UStateMachineSymbol*>& DataSource, TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps) const;


	// If input runs out on this state, this is how that result will be interpreted. 
	UPROPERTY(EditAnywhere, Category = "State Machine")
		EStateMachineCompletionType CompletionType;

	UPROPERTY(EditAnywhere, Category = "State Machine")
		bool bTerminateImmediately;

	UPROPERTY(EditAnywhere, Category = "State Machine")
		bool bLoopByDefault;
};
