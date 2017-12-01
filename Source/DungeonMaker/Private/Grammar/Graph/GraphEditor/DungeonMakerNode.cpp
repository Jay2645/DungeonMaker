#include "DungeonMakerNode.h"

#define LOCTEXT_NAMESPACE "DungeonMakerNode"

UDungeonMakerNode::UDungeonMakerNode()
{
	DefaultBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	TightlyCoupledBackgroundColor = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
}

UDungeonMakerNode::~UDungeonMakerNode()
{

}

FString UDungeonMakerNode::GetNodeTitle()
{
	if (CustomNodeTitle.IsEmpty())
	{
		if (NodeType == NULL)
		{
			return "Null Node";
		}
		else
		{
			return NodeType->Description.ToString();
		}
	}
	else
	{
		return CustomNodeTitle;
	}
}


FNumberedGraphSymbol UDungeonMakerNode::ToGraphSymbol() const
{
	FNumberedGraphSymbol symbol;
	symbol.Symbol = NodeType;
	symbol.SymbolID = NodeID;
	return symbol;
}

UDungeonMakerGraph* UDungeonMakerNode::GetGraph()
{
	return Cast<UDungeonMakerGraph>(GetOuter());
}


FLinearColor UDungeonMakerNode::GetBackgroundColor() const
{
	if (bTightlyCoupledToParent)
	{
		return TightlyCoupledBackgroundColor;
	}
	else
	{
		return DefaultBackgroundColor;
	}
}

#undef LOCTEXT_NAMESPACE
