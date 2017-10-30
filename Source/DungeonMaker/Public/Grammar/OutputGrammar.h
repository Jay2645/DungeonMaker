// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grammar/GrammarAlphabet.h"
#include "OutputGrammar.generated.h"

/**
* This is a "replacement" that gets added to the overall grammar "space".
* Future grammar iterations go through this output and try to find grammars in this output,
* and so on until no more matches can be found.
* The Grammar class is in charge of matching "alphabet" symbols to the OutputGrammars that replace them.
*/
UCLASS(BlueprintType, Abstract)
class DUNGEONMAKER_API UOutputGrammar : public UDataAsset
{
	GENERATED_BODY()
};
