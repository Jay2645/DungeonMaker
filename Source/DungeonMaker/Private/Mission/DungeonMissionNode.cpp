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

FString UDungeonMissionNode::GetSymbolDescription()
{
	return Symbol.GetSymbolDescription();
}