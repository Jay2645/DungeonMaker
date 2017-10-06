// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachineSymbol.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStateMachine, Log, All);

/**
 * 
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UStateMachineSymbol : public UDataAsset
{
	GENERATED_BODY()
public:
	// The display value for this input atom, mainly for debugging purposes
	UPROPERTY(EditAnywhere)
	FName Description;
};
