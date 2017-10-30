// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachineState.h"
#include "OutputGrammar.h"
#include "GrammarAlphabet.h"
#include "Grammar.generated.h"

class UGrammar;

UENUM(BlueprintType)
enum class EGrammarResultType : uint8
{
	// This state is accepted.
	Accepted,
	// This state is rejected.
	Rejected,
	// We don't know yet
	InProgress
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FGrammarResult
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const UGrammar* Grammar;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const UStateMachineState* NextState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGrammarResultType GrammarResult;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<UStateMachineSymbol*> GrammarInputShape;

	FGrammarResult()
	{
		Grammar = NULL;
		NextState = NULL;
		GrammarResult = EGrammarResultType::Rejected;
		GrammarInputShape = TArray<UStateMachineSymbol*>();
	}
};

/**
 * This is a representation of a "rule" for replacing symbols with other symbols.
 * Each rule can be assigned a weight, making it more likely to be chosen.
 */
UCLASS(BlueprintType, Abstract)
class DUNGEONMAKER_API UGrammar : public UDataAsset
{
	GENERATED_BODY()
public:
	// This contains the input chain for this grammar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const UStateMachineState* RuleInput;
	
	// This contains the output chain for this grammar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	const UOutputGrammar* RuleOutput;

	// How likely we are to select this grammar.
	// Higher numbers mean we are more likely to select it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Weight;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions")
		virtual EGrammarResultType NodeMatchesGrammar(const UObject* ReferenceObject, UGrammarAlphabet* Node, FGrammarResult& OutGrammar) const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeons|Missions")
		FString ConvertToString() const;
};
