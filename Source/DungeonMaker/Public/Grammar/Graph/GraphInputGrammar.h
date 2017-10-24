#pragma once
#include "CoreMinimal.h"
#include "InputGrammar.h"
#include "GraphOutputGrammar.h"
#include "GraphNode.h"
#include "GraphInputGrammar.generated.h"

class UGraphEdge;

/**
* This is a "replacement" that gets added to the overall grammar "space".
* Future grammar iterations go through this output and try to find grammars in this output,
* and so on until no more matches can be found.
* The Grammar class is in charge of matching "alphabet" symbols to the OutputGrammars that replace them.
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UGraphInputGrammar : public UInputGrammar
{
	GENERATED_BODY()
public:
	FStateMachineResult RunCoupledState(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps);
	FStateMachineResult RunCoupledStateWithBranches(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource, TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps);
};

