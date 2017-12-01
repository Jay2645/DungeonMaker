// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grammar.h"
#include "GraphNode.h"
#include "GraphEdge.h"
#include "DungeonMakerGraph.h"
#include "GraphGrammar.generated.h"

/**
 * This is a representation of a "rule" for replacing symbols with other symbols.
 * Each rule can be assigned a weight, making it more likely to be chosen.
 * This subclass is intended to be used for representations of graphs or trees.
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UGraphGrammar : public UGrammar
{
	GENERATED_BODY()
public:
	// The graph representing how the input nodes should look after replacement.
	// If defined, this will be used instead of our "normal" output.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UDungeonMakerGraph* OutputGraph;

	EGrammarResultType MatchesGrammar(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource) const;
};