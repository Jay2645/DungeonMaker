// Fill out your copyright notice in the Description page of Project Settings.

#include "StateMachineBranch.h"

UStateMachineState* UStateMachineBranch::TryBranch(const UObject* ReferenceObject, const TArray<UStateMachineSymbol*>& DataSource,
	int32 DataIndex, int32& OutDataIndex)
{
	OutDataIndex = DataIndex + 1;
	if (DataSource.IsValidIndex(DataIndex) && AcceptableInputs.Contains(DataSource[DataIndex]))
	{
		UE_LOG(LogStateMachine, Verbose, TEXT("%s accepts input %s!"), *GetName(), *DataSource[DataIndex]->Description.ToString());
		return bReverseInputTest ? NULL : DestinationState;
	}
	else
	{
#if !UE_BUILD_SHIPPING
		FString acceptedInputs = "";
		for (int i = 0; i < AcceptableInputs.Num(); i++)
		{
			acceptedInputs.Append(AcceptableInputs[i]->Description.ToString());
			if (i + 1 < AcceptableInputs.Num())
			{
				acceptedInputs.Append(", ");
			}
		}
		UE_LOG(LogStateMachine, Verbose, TEXT("%s does not accept input %s! Acceptable inputs: %s"), *GetName(), *DataSource[DataIndex]->Description.ToString(), *acceptedInputs);
#endif
		return bReverseInputTest ? DestinationState : NULL;
	}
}