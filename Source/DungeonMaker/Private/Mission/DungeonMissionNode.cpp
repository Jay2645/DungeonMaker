// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionNode.h"

UDungeonMakerNode* UDungeonMissionNode::FindChildNodeFromSymbol(FNumberedGraphSymbol ChildSymbol) const
{
	if (ChildrenNodes.Num() == 0)
	{
		return NULL;
	}
	for (UDungeonMakerNode* node : ChildrenNodes)
	{
		if (node->NodeType == ChildSymbol.Symbol && node->NodeID == ChildSymbol.SymbolID)
		{
			return node;
		}
	}
	return NULL;
}

void UDungeonMissionNode::BreakLinkWithNode(const UDungeonMissionNode* Child)
{
	if (ChildrenNodes.Num() == 0 || Child == NULL)
	{
		return;
	}
	for (UDungeonMakerNode* node : ChildrenNodes)
	{
		if (node == Child)
		{
			ChildrenNodes.Remove(node);
			node->ParentNodes.Remove(this);
			FString ourName = ToString(0);
			FString childName = node->ToString(0);
			UE_LOG(LogMissionGen, Log, TEXT("Breaking link between %s and %s."), *ourName, *childName);
			break;
		}
	}
}

void UDungeonMissionNode::AddLinkToNode(UDungeonMissionNode* NewChild, bool bTightlyCoupled)
{
	if (NewChild == NULL)
	{
		return;
	}

	bool bAlreadyHasChild = false;
	for (int i = 0; i < ChildrenNodes.Num(); i++)
	{
		if (ChildrenNodes[i]->NodeID == NewChild->NodeID && 
			ChildrenNodes[i]->NodeType == NewChild->NodeType)
		{
			bAlreadyHasChild = true;
			break;
		}
	}

	if (!bAlreadyHasChild)
	{
		ChildrenNodes.Add(NewChild);
	}

	bool bAlreadyHasParent = false;
	for (int i = 0; i < NewChild->ParentNodes.Num(); i++)
	{
		if (NewChild->ParentNodes[i]->NodeID == NodeID &&
			NewChild->ParentNodes[i]->NodeType == NodeType)
		{
			bAlreadyHasParent = true;
			break;
		}
	}
	if (!bAlreadyHasParent)
	{
		NewChild->bTightlyCoupledToParent = bTightlyCoupled;
		NewChild->ParentNodes.Add(this);
	}

	if (NewChild->bTightlyCoupledToParent)
	{
		UE_LOG(LogMissionGen, Log, TEXT("Parenting %s => %s"), *ToString(0), *NewChild->ToString(0));
	}
	else
	{
		UE_LOG(LogMissionGen, Log, TEXT("Parenting %s -> %s"), *ToString(0), *NewChild->ToString(0));
	}

	UE_LOG(LogMissionGen, Log, TEXT("Dungeon after reparenting: %s"), *ToString(0));
}

int32 UDungeonMissionNode::GetLevelCount()
{
	int32 biggest = 0;
	for (UDungeonMakerNode* node : ChildrenNodes)
	{
		int32 nextNodeLevelCount = ((UDungeonMissionNode*)node)->GetLevelCount();
		if (nextNodeLevelCount > biggest)
		{
			biggest = nextNodeLevelCount;
		}
	}
	return biggest + 1;
}

/*TArray<UDungeonMissionNode*> UDungeonMissionNode::GetDepthFirstSortedNodes(UDungeonMissionNode* Head, bool bOnlyTightlyCoupled)
{
	TSet<UDungeonMissionNode*> visited;
	return DepthVisit(Head, visited, bOnlyTightlyCoupled);
}

TArray<UDungeonMakerNode*> UDungeonMissionNode::GetTopologicalSortedNodes(UDungeonMakerNode* Head)
{
	// Code based on https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search
	TArray<UDungeonMakerNode*> sortedNodes;
	TSet<UDungeonMakerNode*> temporarilyMarked;
	TSet<UDungeonMakerNode*> marked;

	TopologicalVisit(Head, marked, temporarilyMarked, sortedNodes);

	return sortedNodes;
}

void UDungeonMissionNode::TopologicalVisit(UDungeonMakerNode* Node, TSet<UDungeonMakerNode*>& Marked,
	TSet<UDungeonMakerNode*>& TemporaryMarked, TArray<UDungeonMakerNode*>& SortedList)
{
	if (Marked.Contains(Node))
	{
		return;
	}
	if (TemporaryMarked.Contains(Node))
	{
		return;
	}
	TSet<UDungeonMakerNode*> allMarkedNodes = Marked.Union(TemporaryMarked);
	for (UDungeonMakerNode* parent : Node->ParentNodes)
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
}*/

FString UDungeonMissionNode::GetSymbolDescription()
{
	if (NodeType == NULL)
	{
		return "";
	}
	return NodeType->Description.ToString();
}