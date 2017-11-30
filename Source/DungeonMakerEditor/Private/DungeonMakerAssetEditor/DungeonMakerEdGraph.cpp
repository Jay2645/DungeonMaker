#include "DungeonMakerEdGraph.h"
#include "DungeonMakerEdNode.h"

UDungeonMakerEdGraph::UDungeonMakerEdGraph()
{

}

UDungeonMakerEdGraph::~UDungeonMakerEdGraph()
{

}

void UDungeonMakerEdGraph::RebuildDungeonMaker()
{
	LOG_WARNING(TEXT("UDungeonMakerEdGraph::RebuildDungeonMaker has been called"));

	UDungeonMakerGraph* G = CastChecked<UDungeonMakerGraph>(GetOuter());

	G->ClearGraph();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		UDungeonMakerEdNode* EdNode = Cast<UDungeonMakerEdNode>(Nodes[i]);

		if (EdNode == nullptr || EdNode->DungeonMakerNode == nullptr)
			continue;

		UDungeonMakerNode* GNode = EdNode->DungeonMakerNode;

		G->AllNodes.Add(GNode);

		for (int PinIdx = 0; PinIdx < EdNode->Pins.Num(); ++PinIdx)
		{
			UEdGraphPin* Pin = EdNode->Pins[PinIdx];

			if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
				continue;

			for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
			{
				UDungeonMakerEdNode* ChildEdNode = Cast<UDungeonMakerEdNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode());

				if (ChildEdNode == nullptr)
					continue;

				UDungeonMakerNode* ChildNode = ChildEdNode->DungeonMakerNode;

				GNode->ChildrenNodes.Add(ChildNode);

				ChildNode->ParentNodes.Add(GNode);
			}
		}
	}

	for (int i = 0; i < G->AllNodes.Num(); ++i)
	{
		UDungeonMakerNode* Node = G->AllNodes[i];
		if (Node->ParentNodes.Num() == 0)
		{
			G->RootNodes.Add(Node);
		}
	}
}

#if WITH_EDITOR
void UDungeonMakerEdGraph::PostEditUndo()
{
	Super::PostEditUndo();

	RebuildDungeonMaker();

	Modify();
}
#endif

