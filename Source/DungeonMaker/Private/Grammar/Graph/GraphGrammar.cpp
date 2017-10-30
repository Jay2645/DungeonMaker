// Fill out your copyright notice in the Description page of Project Settings.

#include "GraphGrammar.h"

EGrammarResultType UGraphGrammar::MatchesGrammar(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource) const
{
	FStateMachineResult result = ((UGraphInputGrammar*)RuleInput)->RunCoupledState(ReferenceObject, DataSource, TArray<UStateMachineBranch*>(), 0, -1);
	
	if (result.CompletionType == EStateMachineCompletionType::Accepted)
	{
		return EGrammarResultType::Accepted;
	}
	else if (result.CompletionType == EStateMachineCompletionType::NotAccepted)
	{
		return EGrammarResultType::InProgress;
	}
	else
	{
		return EGrammarResultType::Rejected;
	}
}