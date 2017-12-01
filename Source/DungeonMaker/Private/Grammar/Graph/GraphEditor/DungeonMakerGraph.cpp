#include "DungeonMakerGraph.h"
#include "DungeonMakerRuntimePrivatePCH.h"

#define LOCTEXT_NAMESPACE "DungeonMaker"

UDungeonMakerGraph::UDungeonMakerGraph()
{
	NodeType = UDungeonMakerNode::StaticClass();

#if WITH_EDITORONLY_DATA
	EdGraph = nullptr;
#endif
}

UDungeonMakerGraph::~UDungeonMakerGraph()
{

}

int32 UDungeonMakerGraph::AddNode(UDungeonMakerNode* NodeToAdd)
{
	int32 index = AllNodes.Add(NodeToAdd);
	
	// See if we should change the actual node ID based on our input
	// This is the ID of the node in the graph segment we're replacing
	int32 inputID = NodeToAdd->InputNodeID;
	// This is the ID that we should be assigned next
	int32 nextID = index + 1;
	if (inputID > 0)
	{
		// If the user has specified that this node should replace another node
		// in the graph, we need to ensure that they get assigned the proper ID.
		// This means that if there is already a node with that ID, it should get
		// the ID we were "supposed" to have.
		if (NodeIDLookup.Contains(inputID))
		{
			NodeIDLookup[inputID]->NodeID = nextID;
			NodeIDLookup.Add(nextID, NodeIDLookup[inputID]);
		}
		nextID = inputID;
	}

	// Update our ID
	NodeToAdd->NodeID = nextID;
	NodeIDLookup.Add(nextID, NodeToAdd);

	return index;
}

FDungeonMissionGraphOutput UDungeonMakerGraph::CreateOutputGraph() const
{
	FDungeonMissionGraphOutput output = FDungeonMissionGraphOutput();
	if (AllNodes.Num() == 0)
	{
		return output;
	}

	for (int i = 0; i < AllNodes.Num(); i++)
	{
		// There is a crash if you try to use TMap.Contains(Item); this much slower version must be used
		bool bFound = false;
		UDungeonMakerNode* currentNode = AllNodes[i];
		for (auto& kvp : output.Links)
		{
			if (kvp.Key.SymbolID == currentNode->NodeID)
			{
				bFound = true;
				break;
			}
		}
		if (bFound)
		{
			continue;
		}
		
		// Gather up our children
		FNodeChildren nodeChildren = FNodeChildren();
		for (int j = 0; j < currentNode->ChildrenNodes.Num(); j++)
		{
			UDungeonMakerNode* child = currentNode->ChildrenNodes[j];
			FGraphLink childLink;
			childLink.Symbol = child->ToGraphSymbol();
			childLink.bIsTightlyCoupled = child->bTightlyCoupledToParent;
			nodeChildren.Children.Add(childLink);
		}
		output.Links.Add(currentNode->ToGraphSymbol(), nodeChildren);
	}
	if (NodeIDLookup.Contains(1))
	{
		output.Head.Symbol = NodeIDLookup[1]->ToGraphSymbol();
	}
	else
	{
		output.Head.Symbol = AllNodes[0]->ToGraphSymbol();
	}

	UE_LOG(LogMissionGen, Log, TEXT("Completed package: %s"), *output.ToString());

	return output;
}

void UDungeonMakerGraph::Print(bool ToConsole /*= true*/, bool ToScreen /*= true*/)
{
	int Level = 0;
	TArray<UDungeonMakerNode*> CurrLevelNodes = RootNodes;
	TArray<UDungeonMakerNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UDungeonMakerNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			FString Message = FString::Printf(TEXT("%s, Level %d"), *Node->GetNodeTitle(), Level);

			if (ToConsole)
			{
				LOG_WARNING(TEXT("%s"), *Message);
			}

			if (ToScreen && GEngine != nullptr)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, Message);
			}

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}

int UDungeonMakerGraph::GetLevelNum()
{
	int Level = 0;
	TArray<UDungeonMakerNode*> CurrLevelNodes = RootNodes;
	TArray<UDungeonMakerNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UDungeonMakerNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

	return Level;
}

void UDungeonMakerGraph::GetNodesByLevel(int Level, TArray<UDungeonMakerNode*>& Nodes)
{
	int CurrLEvel = 0;
	TArray<UDungeonMakerNode*> NextLevelNodes;

	Nodes = RootNodes;

	while (Nodes.Num() != 0)
	{
		if (CurrLEvel == Level)
			break;

		for (int i = 0; i < Nodes.Num(); ++i)
		{
			UDungeonMakerNode* Node = Nodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		Nodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++CurrLEvel;
	}
}

void UDungeonMakerGraph::ClearGraph()
{
	for (int i = 0; i < AllNodes.Num(); ++i)
	{
		UDungeonMakerNode* Node = AllNodes[i];

		Node->ParentNodes.Reset();
		Node->ChildrenNodes.Reset();
	}

	AllNodes.Reset();
	RootNodes.Reset();
}

#undef LOCTEXT_NAMESPACE
