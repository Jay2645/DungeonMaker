#pragma once

#include "GraphGrammar.h"

#include "DungeonMissionGrammar.generated.h"

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
};