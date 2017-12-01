#include "GraphOutputGrammar.h"
#include "GraphNode.h"

int32 FDungeonMissionGraphOutput::Num() const
{
	return Links.Num();
}

TArray<FNumberedGraphSymbol> FDungeonMissionGraphOutput::GetSymbolArray() const
{
	TArray<FNumberedGraphSymbol> values;
	Links.GenerateKeyArray(values);
	return values;
}

TSet<FGraphLink> FDungeonMissionGraphOutput::GetSymbolChildren(const FNumberedGraphSymbol& Symbol) const
{
	check(IsValid(Symbol.Symbol));
	// For whatever reason, this will crash the editor the first time after you make changes to something
	//checkf(Links.Contains(Symbol), TEXT("%s does not contain %s! Did you add it to the links map?"), *GetName(), *Symbol.GetSymbolDescription());
	//return Links[Symbol].Children;

	// We're going to have to do this the slower way
	for (auto kvp : Links)
	{
		FNumberedGraphSymbol thisKey = kvp.Key;
		if (thisKey.Symbol == Symbol.Symbol && thisKey.SymbolID == Symbol.SymbolID)
		{
			return kvp.Value.Children;
		}
	}
	return TSet<FGraphLink>();
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

void FDungeonMissionGraphOutput::Add(FNumberedGraphSymbol Parent, FGraphLink& Link)
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

FString FDungeonMissionGraphOutput::ToString() const
{
	FString output = "";
	TArray<FString> allStringCombinations = GetChildrenStrings(Head);
	for (int i = 0; i < allStringCombinations.Num(); i++)
	{
		output.Append(allStringCombinations[i]);
		if (i + 1 < allStringCombinations.Num())
		{
			output.Append("\n");
		}
	}
	return output;
};

TArray<FString> FDungeonMissionGraphOutput::GetChildrenStrings(FGraphLink Current) const
{
	TArray<FString> childStringList;

	FString linkList = "";
	for (auto kvp : Links)
	{
		linkList.Append(kvp.Key.GetSymbolDescription() + ", ");
	}
	checkf(Links.Contains(Current.Symbol), TEXT("Output %s was missing entry for %s! Link length: %d"), *linkList, *Current.Symbol.GetSymbolDescription(), Links.Num());

	for (FGraphLink child : Links[Current.Symbol].Children)
	{
		FString linkName = child.Symbol.GetSymbolDescription();
		TArray<FString> childStrings = GetChildrenStrings(child);
		if (childStrings.Num() == 0)
		{
			childStringList.Add(linkName);
		}
		else
		{
			for (int i = 0; i < childStrings.Num(); i++)
			{
				FString childName = linkName;
				if (child.bIsTightlyCoupled)
				{
					childName.Append("=>");
				}
				else
				{
					childName.Append("->");
				}
				childName.Append(childStrings[i]);
				childStringList.Add(childName);
			}
		}
	}
	return childStringList;
}