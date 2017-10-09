// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon.h"
#include "Grammar/Grammar.h"
#include <DrawDebugHelpers.h>

// Sets default values
ADungeon::ADungeon()
{
	PrimaryActorTick.bCanEverTick = false;
	Mission = CreateDefaultSubobject<UDungeonMissionGenerator>(TEXT("Dungeon Mission"));
}

ADungeon::~ADungeon()
{
	for (auto& kvp : RoomMap)
	{
		delete(kvp.Value);
	}
}

// Called when the game starts or when spawned
void ADungeon::BeginPlay()
{
	Super::BeginPlay();

	Mission->TryToCreateDungeon(1234);

	TArray<UDungeonMissionNode*> toProcess;
	toProcess.Add(Mission->Head);

	while (toProcess.Num() > 0)
	{
		UDungeonMissionNode* node = toProcess[0];
		checkf(node->Symbol.Symbol != NULL, TEXT("Dungeon tried processing a null node! Did you set up your DungeonMissionGenerator correctly?"));
		checkf(node->Symbol.Symbol->bIsTerminalNode, TEXT("Dungeon tried processing non-terminal node %s! Do all of your non-terminal nodes get replaced?"), *node->GetSymbolDescription());
		toProcess.RemoveAt(0);
		FDungeonRoom* room = FDungeonRoom::GenerateDungeonRoom((UDungeonMissionSymbol*)node->Symbol.Symbol, DefaultRoomTile);

		RoomMap.Add(node, room);

		for (FMissionNodeData& nextNode : node->NextNodes)
		{
			if (RoomMap.Contains(nextNode.Node))
			{
				continue;
			}
			toProcess.Add(nextNode.Node);
		}
	}

	for (auto& kvp : RoomMap)
	{
		UDungeonMissionNode* node = kvp.Key;
		FDungeonRoom* room = kvp.Value;
		UE_LOG(LogTemp, Warning, TEXT("%s (%d):\n%s"), *node->GetSymbolDescription(), node->Symbol.SymbolID, *room->ToString());
	}

	bool didSplit = true;
	TArray<UBSPLeaf*> leaves;

	// Dungeons grow exponentially; we need to create leaves to match
	// Equation is based on fitting {{16, 54}, {43, 81}, {69, 108}}
	// x^2/1378 + (1319 x)/1378 + 26526/689
	int32 x = RoomMap.Num() * 2;
	int32 dungeonSize = FMath::CeilToInt(((x * x) / 1378) + ((1319 * x) / 1378) + (26526 / 689));

	RootLeaf = UBSPLeaf::CreateLeaf(this, NULL, TEXT("Root Leaf"), 0, 0, dungeonSize, dungeonSize);
	leaves.Add(RootLeaf);

	int32 maxLeafSize = 24;

	FRandomStream rng(1234);

	while (didSplit)
	{
		TArray<UBSPLeaf*> nextLeaves;
		didSplit = false;
		for (int i = 0; i < leaves.Num(); i++)
		{
			UBSPLeaf* leaf = leaves[i];
			if (leaf == NULL)
			{
				UE_LOG(LogDungeonGen, Warning, TEXT("Null leaf found!"));
				continue;
			}
			nextLeaves.Add(leaf);
			if (leaf->HasChildren())
			{
				// Already processed
				continue;
			}
			// If this leaf is too big, or a 75% chance 
			if (leaf->SideIsLargerThan(maxLeafSize) || rng.GetFraction() > 0.25f)
			{
				if (leaf->Split(rng))
				{
					// Add the child leaves to our nextLeaves array
					nextLeaves.Add(leaf->RightChild);
					nextLeaves.Add(leaf->LeftChild);
					didSplit = true;
				}
			}
		}
		// Now that we're out of the for loop, update the array for the next pass
		leaves = nextLeaves;
	}

	// Done splitting; find nearest neighbors
	for (int i = 0; i < leaves.Num(); i++)
	{
		leaves[i]->DetermineNeighbors();
	}

	int32 leafCount = RootLeaf->GetChildLeafCount();
	int32 roomCount = RoomMap.Num();
	UE_LOG(LogDungeonGen, Warning, TEXT("Generated %d leaves and %d rooms."), leafCount, roomCount);

	UBSPLeaf* entranceLeaf = RootLeaf;
	while (entranceLeaf->LeftChild != NULL)
	{
		entranceLeaf = entranceLeaf->LeftChild;
	}
	TSet<UBSPLeaf*> availableLeaves;
	availableLeaves.Add(entranceLeaf);
	// The toProcess array is now empty from the while loop we ran earlier
	//toProcess.Add(Mission->Head);
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<UBSPLeaf*> processedLeaves;
	PairNodesToLeaves(Mission->Head, availableLeaves, rng, processedNodes, processedLeaves, entranceLeaf, availableLeaves);
	for (UBSPLeaf* leaf : processedLeaves)
	{
		// Check to see if all our mission neighbors are next to us
		if (leaf->LeafNeighbors.Intersect(leaf->MissionNeighbors).Num() == leaf->MissionNeighbors.Num())
		{
			// They are
			continue;
		}
		else
		{
			// We have at least 1 mission neighbor which isn't an actual neighbor of ours
			TSet<UBSPLeaf*> missionNeighbors;
			missionNeighbors.Append(leaf->MissionNeighbors);
			for (UBSPLeaf* neighbor : missionNeighbors)
			{
				if (leaf->LeafNeighbors.Contains(neighbor))
				{
					// This one is okay
					continue;
				}
				continue;
				// This isn't our actual neighbor
				leaf->MissionNeighbors.Remove(neighbor);

				TArray<UBSPLeaf*> availableParents;
				for (UBSPLeaf* parentLeaf : leaf->LeafNeighbors)
				{
					if (processedLeaves.Contains(parentLeaf))
					{
						if (parentLeaf->AreChildrenAllowed())
						{
							availableParents.Add(parentLeaf);
						}
					}
				}
				if (availableParents.Num() == 0)
				{
					leaf->AddMissionLeaf(neighbor);
				}
				else
				{
					leaf->AddMissionLeaf(availableParents[rng.RandRange(0, availableParents.Num() - 1)]);
				}
			}
		}
	}
	RootLeaf->DrawDebugLeaf();
}

bool ADungeon::PairNodesToLeaves(UDungeonMissionNode* Node, 
								TSet<UBSPLeaf*>& AvailableLeaves, FRandomStream& Rng,
								TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves,
								UBSPLeaf* EntranceLeaf, TSet<UBSPLeaf*>& AllOpenLeaves,
								bool bIsTightCoupling)
{
		//UDungeonMissionNode* node = ToProcess[0];
		//ToProcess.RemoveAt(0);
		if (ProcessedNodes.Contains(Node))
		{
			// Already processed this node
			return true;
		}
		if (ProcessedNodes.Intersect(Node->ParentNodes).Num() != Node->ParentNodes.Num())
		{
			// We haven't processed all our parent nodes yet!
			// We should be processed further on down the line, once our next parent node
			// finishes being processed.
			UE_LOG(LogDungeonGen, Warning, TEXT("Deferring processing of %s because not all its parents have been processed yet."), *Node->GetSymbolDescription());
			return true;
		}
		if (AvailableLeaves.Num() == 0 && bIsTightCoupling)
		{
			// Out of leaves to process
			UE_LOG(LogDungeonGen, Warning, TEXT("%s is tightly coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
			return false;
		}
		if (AllOpenLeaves.Num() == 0 && !bIsTightCoupling)
		{
			UE_LOG(LogDungeonGen, Warning, TEXT("%s is loosely coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
			return false;
		}
		UE_LOG(LogDungeonGen, Log, TEXT("Creating room for %s! Leaves available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableLeaves.Num(), Node->NextNodes.Num());
		// Find an open leaf to add this to
		UBSPLeaf* leaf = NULL;
		if (bIsTightCoupling)
		{
			UE_LOG(LogDungeonGen, Log, TEXT("%s is tightly coupled to its parent. Selecting from pool of %d available leaves."), *Node->GetSymbolDescription(), AvailableLeaves.Num());
			leaf = GetOpenLeaf(Node, AvailableLeaves, Rng, ProcessedLeaves);
			if (leaf == NULL)
			{
				// No open leaf available for us; back out
				return false;
			}
		}
		else
		{
			UE_LOG(LogDungeonGen, Log, TEXT("%s is loosely coupled to its parent. Selecting from pool of %d available leaves."), *Node->GetSymbolDescription(), AllOpenLeaves.Num());
			leaf = GetOpenLeaf(Node, AllOpenLeaves, Rng, ProcessedLeaves);
			if (leaf == NULL)
			{
				// No open leaf available for us; back out
				return false;
			}
		}
		/*if (bIsTightCoupling)
		{
			// Our current node is tightly coupled to the entrance leaf
			// Sever all connections to any leaf but the entrance
			TSet<UBSPLeaf*> neighbors;
			neighbors.Append(leaf->LeafNeighbors);
			for (UBSPLeaf* neighbor : neighbors)
			{
				if (neighbor != EntranceLeaf && ProcessedLeaves.Contains(neighbor))
				{
					neighbor->LeafNeighbors.Remove(leaf);
					leaf->LeafNeighbors.Remove(neighbor);
				}
			}
		}*/

		ProcessedLeaves.Add(leaf);
		ProcessedNodes.Add(Node);

		// Grab all our neighbor leaves, excluding those which have already been processed
		TSet<UBSPLeaf*> neighboringLeaves = leaf->LeafNeighbors.Difference(ProcessedLeaves);
		// Find all the tightly coupled nodes attached to our current node
		//TArray<UDungeonMissionNode*> tightlyCoupledNodes;
		TArray<UDungeonMissionNode*> nextToProcess;
		for (FMissionNodeData& neighborNode : Node->NextNodes)
		{
			if (neighborNode.bTightlyCoupledToParent)
			{
				// If we're tightly coupled to our parent, ensure we get added to a neighboring leaf
				bool bSuccesfullyPairedChild = PairNodesToLeaves(neighborNode.Node, neighboringLeaves, Rng, ProcessedNodes, ProcessedLeaves, leaf, AllOpenLeaves, true);
				if (!bSuccesfullyPairedChild)
				{
					// Failed to find a child leaf; back out
					ProcessedNodes.Remove(Node);
					// Restart -- next time, we'll select a different leaf
					return PairNodesToLeaves(Node, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, EntranceLeaf, AllOpenLeaves, bIsTightCoupling);
				}
				//tightlyCoupledNodes.Add(neighborNode.Node);
			}
			else
			{
				nextToProcess.Add(neighborNode.Node);
			}
		}

		// If we have a tightly-coupled node, ensure that it gets assigned to a neighboring leaf
		/*if (tightlyCoupledNodes.Num() > 0)
		{
			checkf(tightlyCoupledNodes.Num() <= neighboringLeaves.Num(), TEXT("%s has more tightly coupled nodes (%d) than we do available leaves (%d)!"), *node->GetSymbolDescription(), tightlyCoupledNodes.Num(), neighboringLeaves.Num());
			UE_LOG(LogDungeonGen, Log, TEXT("%s has %d tightly coupled nodes attached to it, and %d available leaves."), *node->GetSymbolDescription(), tightlyCoupledNodes.Num(), neighboringLeaves.Num());
			PairNodesToLeaves(tightlyCoupledNodes, neighboringLeaves, Rng, ProcessedNodes, ProcessedLeaves, leaf, false);
		}*/
		if (((UDungeonMissionSymbol*)Node->Symbol.Symbol)->bAllowedToHaveChildren)
		{
			AvailableLeaves.Append(neighboringLeaves);
		}
		else
		{
			for (UBSPLeaf* neighbor : leaf->LeafNeighbors)
			{
				//if (neighbor != EntranceLeaf)
				//{
					neighbor->LeafNeighbors.Remove(leaf);
				//}
			}
			leaf->LeafNeighbors.Empty(1);
			//leaf->LeafNeighbors.Add(EntranceLeaf);
		}
		AllOpenLeaves.Append(AvailableLeaves);
		// Now we process all non-tightly coupled nodes
		for (int i = 0; i < nextToProcess.Num(); i++)
		{
			bool bSuccesfullyPairedChild = PairNodesToLeaves(nextToProcess[i], AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, leaf, AllOpenLeaves, false);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				return PairNodesToLeaves(Node, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, EntranceLeaf, AllOpenLeaves, bIsTightCoupling);
			}
		}

		// Let this leaf contain the room symbol
		leaf->SetMissionNode(Node, Rng);
		leaf->AddMissionLeaf(EntranceLeaf);
		// Set up a connection to our "parent" leaf (the leaf the player enters this room from)
		return true;
}

UBSPLeaf* ADungeon::GetOpenLeaf(UDungeonMissionNode* Node, TSet<UBSPLeaf*>& AvailableLeaves, FRandomStream& Rng, TSet<UBSPLeaf*>& ProcessedLeaves)
{

	TSet<UDungeonMissionNode*> nodesToCheck;
	for (FMissionNodeData& neighborNode : Node->NextNodes)
	{
		if (neighborNode.bTightlyCoupledToParent)
		{
			nodesToCheck.Add(neighborNode.Node);
		}
	}
	UBSPLeaf* leaf = NULL;
	do
	{
		if (AvailableLeaves.Num() == 0)
		{
			return NULL;
		}
		//checkf(AvailableLeaves.Num() > 0, TEXT("Not enough leaves for all the rooms we need to generate! We failed to generate room %s (ID %d). You may need to make the leaf BSP bigger."), *Node->GetSymbolDescription(), Node->Symbol.SymbolID);
		int32 leafIndex = Rng.RandRange(0, AvailableLeaves.Num() - 1);
		leaf = AvailableLeaves.Array()[leafIndex];
		AvailableLeaves.Remove(leaf);
		if (ProcessedLeaves.Contains(leaf))
		{
			// Already processed this leaf
			leaf = NULL;
			continue;
		}
		TSet<UBSPLeaf*> neighbors = leaf->LeafNeighbors.Difference(ProcessedLeaves);
		if (neighbors.Num() < nodesToCheck.Num())
		{
			// This leaf wouldn't have enough neighbors to attach all our tightly-coupled nodes
			UE_LOG(LogDungeonGen, Log, TEXT("Abandoning processing %s for node %s because it has fewer neighbors (%d) than it does tightly-coupled nodes (%d)."), *leaf->GetName(), *Node->GetSymbolDescription(), AvailableLeaves.Num(), nodesToCheck.Num());
			leaf = NULL;
			continue;
		}
	} while (leaf == NULL);
	return leaf;
}

