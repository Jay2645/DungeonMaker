#pragma once

#include "CoreMinimal.h"
#include "StateMachineBranch.h"
#include "GraphNode.h"
#include "GraphInputGrammar.h"
#include "GraphOutputGrammar.h"
#include "GraphEdge.generated.h"

class GraphGrammar;

UCLASS(BlueprintType)
class DUNGEONMAKER_API UGraphEdge : public UStateMachineBranch
{
	GENERATED_BODY()
public:
	// Does this edge have a "tight" coupling to the next node?
	UPROPERTY(EditAnywhere)
	bool bIsTightlyCoupled;

	UFUNCTION(BlueprintCallable, Category = "State Machine")
	virtual UGraphInputGrammar* TryCoupledBranch(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource,
		int32 DataIndex, int32& OutDataIndex);
};