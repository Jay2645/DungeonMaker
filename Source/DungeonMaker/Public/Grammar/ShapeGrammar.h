// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grammar.h"
#include "ShapeGrammar.generated.h"

/**
 * This is a representation of a "rule" for replacing symbols with other symbols.
 * Each rule can be assigned a weight, making it more likely to be chosen.
 * This subclass is intended to be used for representations of polygons or shapes.
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UShapeGrammar : public UGrammar
{
	GENERATED_BODY()
	
};
