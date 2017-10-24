#include "GraphOutputGrammar.h"

int32 UGraphOutputGrammar::Num() const
{
	return Links.Num();
}

TArray<FNumberedGraphSymbol> UGraphOutputGrammar::GetSymbolArray() const
{
	TArray<FNumberedGraphSymbol> values;
	Links.GenerateKeyArray(values);
	return values;
}

TSet<FGraphLink> UGraphOutputGrammar::GetSymbolChildren(const FNumberedGraphSymbol& Symbol) const
{
	check(IsValid(Symbol.Symbol));
	checkf(Links.Contains(Symbol), TEXT("%s does not contain %s! Did you add it to the links map?"), *GetName(), *Symbol.GetSymbolDescription());
	return Links[Symbol].Children;
}

//TArray<TArray<FGraphLink>> GetAllChildren() const
//{
//	TArray<FGraphLink> head;
//	head.Add(Head);
//	return GetAllChildVariants(head);
//}

///*
//This gets all possible chains of all children of the given graph.
//*/
//TArray<TArray<FGraphLink>> GetAllChildVariants(TArray<FGraphLink> Current) const
//{
//	// Each iteration, we grab the current number of children
//	TArray<FGraphLink> children = GetSymbolChildren(Current[Current.Num() - 1].Symbol);
//	TArray<TArray<FGraphLink>> output;
//	// If we have no children, we just return our current chain
//	// This is the only possible chain we can return
//	if (children.Num() == 0)
//	{
//		output.Add(Current);
//		return output;
//	}
//	else
//	{
//		// If we have children, iterate over each child
//		for (int i = 0; i < children.Num(); i++)
//		{
//			// Copy our current input
//			TArray<FGraphLink> childArray = TArray<FGraphLink>(Current, 1);
//			// Add this child to the current chain
//			childArray.Add(children[i]);
//			// Get all possible chains involving the children of this child
//			output.Append(GetAllChildVariants(childArray));
//		}
//		return output;
//	}
//}

void UGraphOutputGrammar::Add(FNumberedGraphSymbol Parent, FGraphLink& Link)
{
	if (Head.Symbol.Symbol == NULL)
	{
		Head = Link;
	}
	Links.Add(Link.Symbol, FNodeChildren());
	if (Links.Contains(Parent))
	{
		Links[Parent].Children.Add(Link);
	}
};

FString UGraphOutputGrammar::ToString() const
{
	FString output = "";
	for (auto& elem : Links)
	{
		FNumberedGraphSymbol parent = elem.Key;
		FString symbolDescription = parent.GetSymbolDescription();
		TSet<FGraphLink> children = elem.Value.Children;
		for (FGraphLink& link : children)
		{
			if (link.bIsTightlyCoupled)
			{
				output.Append(symbolDescription + "=>" + link.Symbol.GetSymbolDescription() + "\n");
			}
			else
			{
				output.Append(symbolDescription + "->" + link.Symbol.GetSymbolDescription() + "\n");
			}
		}
		output.Append("\n");
	}
	return output;
};