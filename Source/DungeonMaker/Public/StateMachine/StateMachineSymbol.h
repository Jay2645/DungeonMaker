// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DungeonMaker.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachineSymbol.generated.h"

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
