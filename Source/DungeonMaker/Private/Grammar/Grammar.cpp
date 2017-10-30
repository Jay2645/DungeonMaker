// Fill out your copyright notice in the Description page of Project Settings.

#include "Grammar.h"

EGrammarResultType UGrammar::NodeMatchesGrammar(const UObject* ReferenceObject, UGrammarAlphabet* Node, FGrammarResult& OutGrammar) const
{
	if (OutGrammar.Grammar != NULL && OutGrammar.Grammar->RuleInput->Contains(Node))
	{
		// We've prepared a response for this symbol
		// Now it's time to find out if that response is accept, reject, or InProgress.

		// Add this to the input shape
		OutGrammar.GrammarInputShape.Add(Node);

		// Run the state machine
#if !UE_BUILD_SHIPPING
		UE_LOG(LogDungeonGen, Log, TEXT("Checking if state machine %s accepts node %s"), *RuleInput->GetName(), *Node->Description.ToString());

		UE_LOG(LogDungeonGen, Log, TEXT("Input shape:"));
		for (int i = 0; i < OutGrammar.GrammarInputShape.Num(); i++)
		{
			UE_LOG(LogDungeonGen, Log, TEXT("%d: %s"), i, *OutGrammar.GrammarInputShape[i]->Description.ToString());
		}
#endif

		FStateMachineResult machineResult = RuleInput->RunState(ReferenceObject, OutGrammar.GrammarInputShape);

		switch (machineResult.CompletionType)
		{
		case EStateMachineCompletionType::Accepted:
			// Good to go! We can replace this grammar
			UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s accepts node %s"), *OutGrammar.Grammar->ConvertToString(), *Node->Description.ToString());
			OutGrammar.NextState = machineResult.FinalState;
			OutGrammar.GrammarResult = EGrammarResultType::Accepted;
			return EGrammarResultType::Accepted;

		case EStateMachineCompletionType::Rejected:
			// We cannot replace this grammar at all
			UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s rejects node %s"), *OutGrammar.Grammar->ConvertToString(), *Node->Description.ToString());
			OutGrammar.GrammarResult = EGrammarResultType::Rejected;
			return EGrammarResultType::Rejected;
		
		case EStateMachineCompletionType::OutOfSteps:
			UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s ran out of steps processing node %s"), *OutGrammar.Grammar->ConvertToString(), *Node->Description.ToString());
			break;
		
		case EStateMachineCompletionType::NotAccepted:
			UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s is still processing node %s"), *OutGrammar.Grammar->ConvertToString(), *Node->Description.ToString());
			break;
			// Other results mean that the machine is still in progress
		}
		// We did not explicitly accept or reject this node
		UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s is still processing node %s"), *OutGrammar.Grammar->ConvertToString(), *Node->Description.ToString());
		OutGrammar.NextState = machineResult.FinalState;
		OutGrammar.GrammarResult = EGrammarResultType::InProgress;
		return EGrammarResultType::InProgress;
	}
	// We don't know about this symbol at all
	UE_LOG(LogDungeonGen, Log, TEXT("Grammar dungeon shape %s doesn't know anything about node %s"), *GetName(), *Node->Description.ToString());
	OutGrammar.GrammarResult = EGrammarResultType::Rejected;
	return EGrammarResultType::Rejected;
}

FString UGrammar::ConvertToString() const
{
	return RuleInput->GetName();
}
