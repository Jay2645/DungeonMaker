#include "DungeonMakerNode.h"

#define LOCTEXT_NAMESPACE "DungeonMakerNode"

UDungeonMakerNode::UDungeonMakerNode()
{
	NodeType = AActor::StaticClass();
	BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

UDungeonMakerNode::~UDungeonMakerNode()
{

}

FString UDungeonMakerNode::GetNodeTitle()
{
	if (CustomNodeTitle.IsEmpty())
	{
		UClass* C = *NodeType;

		FString Title = C->GetName();
		Title.RemoveFromEnd("_C");

		return Title;
	}
	else
	{
		return CustomNodeTitle;
	}
}

UDungeonMakerGraph* UDungeonMakerNode::GetGraph()
{
	return Cast<UDungeonMakerGraph>(GetOuter());
}

#undef LOCTEXT_NAMESPACE
