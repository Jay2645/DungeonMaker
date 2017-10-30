// Fill out your copyright notice in the Description page of Project Settings.

#include "StateMachineState.h"
#include "StateMachineSymbol.h"

UStateMachineState::UStateMachineState()
{
	bLoopByDefault = true;
}

UStateMachineState::UStateMachineState(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bLoopByDefault = true;
}

FStateMachineResult UStateMachineState::RunState(const UObject* ReferenceObject,
	const TArray<UStateMachineSymbol*>& DataSource, int32 DataIndex, int32 RemainingSteps) const
{
	TArray<UStateMachineBranch*> branches;
	branches.Append(InstancedBranches);
	branches.Append(SharedBranches);
	return RunStateWithBranches(ReferenceObject, DataSource, branches, TArray<UStateMachineBranch*>(), DataIndex, RemainingSteps);
}

FStateMachineResult UStateMachineState::RunStateWithBranches(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource,
	TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps) const
{
	bool bMustEndNow = (bTerminateImmediately || !DataSource.IsValidIndex(DataIndex));

	if (RemainingSteps != 0 && !bMustEndNow)
	{
		UStateMachineState* destinationState = NULL;
		int32 destinationDataIndex = DataIndex;
		for (int32 i = 0; i < Branches.Num(); i++)
		{
			// Make sure the branch isn't null
			check(Branches[i]);
			destinationState = Branches[i]->TryBranch(ReferenceObject, DataSource, DataIndex, destinationDataIndex);
			if (destinationState != NULL)
			{
				TakenBranches.Add(Branches[i]);
				return destinationState->RunStateWithBranches(ReferenceObject, DataSource, Branches, TakenBranches, destinationDataIndex, RemainingSteps - 1);
			}
		}
		if (bLoopByDefault)
		{
			return LoopStateWithBranches(ReferenceObject, DataSource, Branches, TakenBranches, DataIndex, RemainingSteps);
		}
		bMustEndNow = true;
	}

	return FStateMachineResult(this, DataIndex, bMustEndNow ? CompletionType : EStateMachineCompletionType::OutOfSteps);
}

FStateMachineResult UStateMachineState::LoopStateWithBranches(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource,
	TArray<UStateMachineBranch*> Branches, TArray<UStateMachineBranch*> TakenBranches, int32 DataIndex, int32 RemainingSteps) const
{
	TakenBranches.Add(NULL);
	return RunStateWithBranches(ReferenceObject, DataSource, Branches, TakenBranches, DataIndex + 1, RemainingSteps - 1);
}

bool UStateMachineState::Contains(const UStateMachineSymbol* Symbol) const
{
	for (int i = 0; i < InstancedBranches.Num(); i++)
	{
		if (InstancedBranches[i]->AcceptableInputs.Contains(Symbol))
		{
			return true;
		}
	}
	for (int i = 0; i < SharedBranches.Num(); i++)
	{
		if (SharedBranches[i]->AcceptableInputs.Contains(Symbol))
		{
			return true;
		}
	}
	return false;
}