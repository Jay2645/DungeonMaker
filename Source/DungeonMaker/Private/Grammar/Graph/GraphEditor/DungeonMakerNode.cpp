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
			FString title = NodeType->Description.ToString() + " (";
			title.AppendInt(NodeID);
			title.AppendChar(')');
			 return title;
		}
	}
	else
	{
		return CustomNodeTitle;
	}
}

FString UDungeonMakerNode::ToString(int32 IndentLevel, bool bPrintChildren)
{
	FString output;
#if !UE_BUILD_SHIPPING
	for (int i = 0; i < IndentLevel; i++)
	{
		output.AppendChar(' ');
	}
	if (bTightlyCoupledToParent)
	{
		output.Append("=>");
	}
	else
	{
		output.Append("->");
	}
	output.Append(NodeType->Description.ToString());
	output.Append(" (");
	output.AppendInt(NodeID);
	output.AppendChar(')');

	if (bPrintChildren)
	{
		for (UDungeonMakerNode* node : ChildrenNodes)
		{
			output.Append("\n");
			output.Append(node->ToString(IndentLevel + 4));
		}
	}
#endif
	return output;
}

bool UDungeonMakerNode::IsChildOf(UDungeonMakerNode* ParentSymbol) const
{
	for (UDungeonMakerNode* parent : ParentNodes)
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
	if (NodeType == NULL)
	{
		return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
	}
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
