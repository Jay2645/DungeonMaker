#include "GraphEdge.h"
#include "GraphGrammar.h"

UGraphInputGrammar* UGraphEdge::TryCoupledBranch(const UObject* ReferenceObject, const TArray<FGraphLink>& DataSource, int32 DataIndex, int32& OutDataIndex)
{
	if (DataSource.IsValidIndex(DataIndex + 1))
	{
		// Check to see if the child should be tightly coupled to the parent
		if (DataSource[DataIndex + 1].bIsTightlyCoupled == bIsTightlyCoupled)
		{
			TArray<UStateMachineSymbol*> dataSource;
			for (int i = 0; i < DataSource.Num(); i++)
			{
				dataSource.Add(DataSource[i].Symbol.Symbol);
			}
			return (UGraphInputGrammar*)TryBranch(ReferenceObject, dataSource, DataIndex, OutDataIndex);
		}
		else
		{
			UE_LOG(LogStateMachine, Log, TEXT("%s does not accept input %s due to coupling mismatch!"), *GetName(), *DataSource[DataIndex].Symbol.GetSymbolDescription());
			OutDataIndex = DataIndex + 1;
			return bReverseInputTest ? (UGraphInputGrammar*)DestinationState : NULL;
		}
	}
	else
	{
		// No children to check for tight coupling
		TArray<UStateMachineSymbol*> dataSource;
		for (int i = 0; i < DataSource.Num(); i++)
		{
			dataSource.Add(DataSource[i].Symbol.Symbol);
		}
		return (UGraphInputGrammar*)TryBranch(ReferenceObject, dataSource, DataIndex, OutDataIndex);
	}
}
