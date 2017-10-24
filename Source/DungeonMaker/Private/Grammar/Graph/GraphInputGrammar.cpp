#include "GraphInputGrammar.h"
#include "GraphEdge.h"

FStateMachineResult UGraphInputGrammar::RunCoupledState(const UObject* ReferenceObject,
	const TArray<FGraphLink>& DataSource, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps)
{
	TArray<UStateMachineBranch*> branches;
	if (InstancedBranches.Num() > 0)
	{
		branches.Append(InstancedBranches);
	}
	if (SharedBranches.Num() > 0)
	{
		branches.Append(SharedBranches);
	}
	return RunCoupledStateWithBranches(ReferenceObject, DataSource, branches, TakenBranches, DataIndex, RemainingSteps);
}

FStateMachineResult UGraphInputGrammar::RunCoupledStateWithBranches(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource,
	TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps)
{
	bool bMustEndNow = (bTerminateImmediately || !DataSource.IsValidIndex(DataIndex));

#if !UE_BUILD_SHIPPING
	if (DataSource.IsValidIndex(DataIndex))
	{
		UE_LOG(LogStateMachine, Log, TEXT("Running state machine %s for %s (Chain Length: %d)."), *GetName(), *DataSource[DataIndex].Symbol.GetSymbolDescription(), DataSource.Num());
	}
#endif

	if (RemainingSteps != 0 && !bMustEndNow)
	{
		UGraphInputGrammar* destinationState = NULL;
		int32 destinationDataIndex = DataIndex;
		for (int32 i = 0; i < Branches.Num(); i++)
		{
			// Make sure the branch isn't null
			check(Branches[i]);
			UGraphEdge* edge = (UGraphEdge*)Branches[i];
			destinationState = edge->TryCoupledBranch(ReferenceObject, DataSource, DataIndex, destinationDataIndex);
			if (destinationState != NULL)
			{
				TakenBranches.Add(edge);
				return destinationState->RunCoupledState(ReferenceObject, DataSource, TakenBranches, destinationDataIndex, RemainingSteps - 1);
			}
		}
		bMustEndNow = true;
	}

	UStateMachineState* finishedState = this;
	EStateMachineCompletionType completion = bMustEndNow ? this->CompletionType : EStateMachineCompletionType::OutOfSteps;

#if !UE_BUILD_SHIPPING
	if (DataSource.IsValidIndex(DataIndex))
	{
		UE_LOG(LogStateMachine, Log, TEXT("Finished running state machine %s for %s. Result: %d"), *GetName(), *DataSource[DataIndex].Symbol.GetSymbolDescription(), (int32)completion);
	}
	else
	{
		UE_LOG(LogStateMachine, Log, TEXT("State machine %s had invalid data index. Result: %d"), *GetName(), (uint8)completion);
	}
#endif

	return FStateMachineResult(finishedState, DataIndex, completion);
}