#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GraphGrammar.h"
#include "GraphOutputGrammar.h"

#include "DungeonMissionGrammar.generated.h"

//class UDungeonMissionStateMachine;

/*USTRUCT(BlueprintType)
struct FDungeonMissionGrammarReplaceResult
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FGraphGrammarCoupling InputCoupling;
	UPROPERTY(EditAnywhere)
	FGraphGrammarShape OutputResult;
	UPROPERTY(EditAnywhere)
	float Weight;
};*/

/**
* This is a "replacement" that gets added to the overall grammar "space".
* Future grammar iterations go through this output and try to find grammars in this output,
* and so on until no more matches can be found.
* The Grammar class is in charge of matching "alphabet" symbols to the OutputGrammars that replace them.
*/
/*UCLASS(BlueprintType)
class UDungeonMissionOutputGrammar : public UGraphOutputGrammar
{
	GENERATED_BODY()
public:

};*/

/**
* This is a representation of a "rule" for replacing symbols with other symbols.
* Each rule can be assigned a weight, making it more likely to be chosen.
* This subclass is intended to be used for representations of graphs or trees, which
* is how a dungeon mission is represented.
*/
UCLASS(BlueprintType)
class UDungeonMissionGrammar : public UGraphGrammar
{
	GENERATED_BODY()
public:
	// Returns the replacement shape contained within our output grammar.
	/*UFUNCTION(BlueprintPure, Category = "World Generation|Dungeons|Missions")
	FGraphGrammarShape GetDungeonReplacementGrammarShape() const;*/
};