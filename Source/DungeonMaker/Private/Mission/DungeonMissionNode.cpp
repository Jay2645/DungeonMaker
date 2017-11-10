// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionNode.h"

UDungeonMissionNode* UDungeonMissionNode::FindChildNodeFromSymbol(FNumberedGraphSymbol ChildSymbol) const
{
	if (NextNodes.Num() == 0)
	{
		return NULL;
	}
	for (const FMissionNodeData& node : NextNodes)
	{
		check(node.Node && node.Node->Symbol.Symbol);
		if (node.Node->Symbol == ChildSymbol)
		{
			return node.Node;
		}
	}
	return NULL;
}

void UDungeonMissionNode::BreakLinkWithNode(const UDungeonMissionNode* Child)
{
	if (NextNodes.Num() == 0 || Child == NULL)
	{
		return;
	}
	for (FMissionNodeData& node : NextNodes)
	{
		check(node.Node);
		if (node.Node == Child)
		{
			NextNodes.Remove(node);
			break;
		}
	}
}

void UDungeonMissionNode::PrintNode(int32 IndentLevel)
{
#if !UE_BUILD_SHIPPING
	FString output;
	for (int i = 0; i < IndentLevel; i++)
	{
		output.AppendChar(' ');
	}
	output.Append("->");
	output.Append(GetSymbolDescription());
	output.Append(" (");
	output.AppendInt(Symbol.SymbolID);
	output.AppendChar(')');
	UE_LOG(LogDungeonGen, Log, TEXT("%s"), *output);
	
	for (FMissionNodeData& node : NextNodes)
	{
		node.Node->PrintNode(IndentLevel + 4);
	}
#endif
}

int32 UDungeonMissionNode::GetLevelCount()
{
	int32 biggest = 0;
	for (FMissionNodeData& node : NextNodes)
	{
		int32 nextNodeLevelCount = node.Node->GetLevelCount();
		if (nextNodeLevelCount > biggest)
		{
			biggest = nextNodeLevelCount;
		}
	}
	return biggest + 1;
}


bool UDungeonMissionNode::IsChildOf(UDungeonMissionNode* ParentSymbol) const
{
	for (UDungeonMissionNode* parent : ParentNodes)
	{
		if (parent == ParentSymbol)
		{
			return true;
		}
		// If our parent is a child of this symbol, so are we
		if (parent->IsChildOf(ParentSymbol))
		{
			return true;
		}
	}
	return false;
}

TArray<UDungeonMissionNode*> UDungeonMissionNode::GetDepthFirstSortedNodes(UDungeonMissionNode* Head, bool bOnlyTightlyCoupled)
{
	TSet<UDungeonMissionNode*> visited;
	return DepthVisit(Head, visited, bOnlyTightlyCoupled);
}

TArray<UDungeonMissionNode*> UDungeonMissionNode::GetTopologicalSortedNodes(UDungeonMissionNode* Head)
{
	// Code based on https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
	TArray<UDungeonMissionNode*> sortedNodes;
	TSet<UDungeonMissionNode*> temporarilyMarked;
	TSet<UDungeonMissionNode*> marked;

	TopologicalVisit(Head, marked, temporarilyMarked, sortedNodes);

	return sortedNodes;
}

void UDungeonMissionNode::TopologicalVisit(UDungeonMissionNode* Node, TSet<UDungeonMissionNode*>& Marked, 
	TSet<UDungeonMissionNode*>& TemporaryMarked, TArray<UDungeonMissionNode*>& SortedList)
{
	if (Marked.Contains(Node))
	{
		return;
	}
	if (TemporaryMarked.Contains(Node))
	{
		return;
	}
	TSet<UDungeonMissionNode*> allMarkedNodes = Marked.Union(TemporaryMarked);
	for (UDungeonMissionNode* parent : Node->ParentNodes)
	{
		if (!allMarkedNodes.Contains(parent))
		{
			// We haven't processed all our parents yet
			return;
		}
	}
	TemporaryMarked.Add(Node);
	// Visit all children
	for (FMissionNodeData child : Node->NextNodes)
	{
		TopologicalVisit(child.Node, Marked, TemporaryMarked, SortedList);
	}
	TemporaryMarked.Remove(Node);
	Marked.Add(Node);
	// Insert at the head
	SortedList.Insert(Node, 0);
}

TArray<UDungeonMissionNode*> UDungeonMissionNode::DepthVisit(UDungeonMissionNode* Node,
	TSet<UDungeonMissionNode*>& Visited, bool bOnlyTightlyCoupled)
{
	TArray<UDungeonMissionNode*> output;
	output.Add(Node);
	Visited.Add(Node);
	for (FMissionNodeData child : Node->NextNodes)
	{
		if (Visited.Contains(child.Node))
		{
			continue;
		}
		if (bOnlyTightlyCoupled && !child.bTightlyCoupledToParent)
		{
			// Not tightly-coupled; we don't care about it
			continue;
		}
		output.Append(DepthVisit(child.Node, Visited, bOnlyTightlyCoupled));
	}
	return output;
}

FString UDungeonMissionNode::GetSymbolDescription()
{
	return Symbol.GetSymbolDescription();
}