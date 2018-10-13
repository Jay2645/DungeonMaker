// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionNode.h"
#include "DungeonMaker.h"

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
			UE_LOG(LogMissionGen, Verbose, TEXT("Breaking link between %s and %s."), *ourName, *childName);
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

#if !UE_BUILD_SHIPPING
	if (NewChild->bTightlyCoupledToParent)
	{
		UE_LOG(LogMissionGen, Verbose, TEXT("Parenting %s => %s"), *ToString(0), *NewChild->ToString(0));
	}
	else
	{
		UE_LOG(LogMissionGen, Verbose, TEXT("Parenting %s -> %s"), *ToString(0), *NewChild->ToString(0));
	}
	UE_LOG(LogMissionGen, Verbose, TEXT("Dungeon after reparenting: %s"), *ToString(0));
#endif
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

FString UDungeonMissionNode::GetSymbolDescription()
{
	if (NodeType == NULL)
	{
		return "";
	}
	return NodeType->Description.ToString();
}