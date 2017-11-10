// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateMachineSymbol.h"
#include "GrammarAlphabet.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMissionGen, Log, All);

/**
 * Very similar to a State Machine Symbol, but with an additional bool.
 * This bool can be used to determine whether additional actions should be done
 * on this symbol.
 */
UCLASS()
class DUNGEONMAKER_API UGrammarAlphabet : public UStateMachineSymbol
{
	GENERATED_BODY()
public:
	// Can this symbol can be "transformed" by the grammar system into another symbol, or is it static?
	// If true, this symbol is "permanent" and should have a room type dedicated to it.
	UPROPERTY(EditAnywhere)
	bool bIsTerminalNode;
	// Can this node be changed at runtime?
	UPROPERTY(EditAnywhere)
	bool bChangedAtRuntime;
};
