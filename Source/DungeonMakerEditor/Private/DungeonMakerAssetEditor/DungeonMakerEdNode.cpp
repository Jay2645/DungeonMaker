#include "DungeonMakerEdNode.h"
#include "DungeonMakerEditorTypes.h"

#define LOCTEXT_NAMESPACE "DungeonMakerEdNode"

void UDungeonMakerEdNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UDungeonMakerEditorTypes::PinCategory_MultipleNodes, TEXT(""), NULL, false, false, TEXT("In"));
	CreatePin(EGPD_Output, UDungeonMakerEditorTypes::PinCategory_MultipleNodes, TEXT(""), NULL, false, false, TEXT("Out"));
}

void UDungeonMakerEdNode::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	GetDungeonMakerEdGraph()->RebuildDungeonMaker();
}

UDungeonMakerEdGraph* UDungeonMakerEdNode::GetDungeonMakerEdGraph()
{
	return Cast<UDungeonMakerEdGraph>(GetGraph());
}

FText UDungeonMakerEdNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (DungeonMakerNode == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}
	else
	{
		return FText::FromString(DungeonMakerNode->GetNodeTitle());
	}
}

void UDungeonMakerEdNode::SetDungeonMakerNode(UDungeonMakerNode* InNode)
{
	DungeonMakerNode = InNode;
}

FText UDungeonMakerEdNode::GetDescription() const
{
	return FText::FromString(DungeonMakerNode->GetNodeTitle());
}

FLinearColor UDungeonMakerEdNode::GetBackgroundColor() const
{
	return DungeonMakerNode->GetBackgroundColor();
}

#undef LOCTEXT_NAMESPACE
