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
	int32 x = RoomMap.Num() * DungeonSizeMultiplier;
	int32 dungeonSize = FMath::CeilToInt(((x * x) / 1378) + ((1319 * x) / 1378) + (26526 / 689));

	RootLeaf = UBSPLeaf::CreateLeaf(this, NULL, TEXT("Root Leaf"), 0, 0, dungeonSize, dungeonSize);
	leaves.Add(RootLeaf);

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
			if (leaf->SideIsLargerThan(MaxRoomSize) || rng.GetFraction() > 0.25f)
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

	StartLeaf = RootLeaf;
	while (StartLeaf->LeftChild != NULL)
	{
		StartLeaf = StartLeaf->LeftChild;
	}
	TSet<FBSPLink> availableLeaves;
	FBSPLink start;
	start.AvailableLeaf = StartLeaf;
	start.FromLeaf = NULL;
	availableLeaves.Add(start);
	// The toProcess array is now empty from the while loop we ran earlier
	//toProcess.Add(Mission->Head);
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<UBSPLeaf*> processedLeaves;
	PairNodesToLeaves(Mission->Head, availableLeaves, rng, processedNodes, processedLeaves, StartLeaf, availableLeaves);
	/*for (UBSPLeaf* leaf : processedLeaves)
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
	}*/
	RootLeaf->DrawDebugLeaf();
}

bool ADungeon::PairNodesToLeaves(UDungeonMissionNode* Node, 
								TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng,
								TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves,
								UBSPLeaf* EntranceLeaf, TSet<FBSPLink>& AllOpenLeaves,
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
		FBSPLink leafLink;
		if (bIsTightCoupling)
		{
			leafLink = GetOpenLeaf(Node, AvailableLeaves, Rng, ProcessedLeaves);
			leaf = leafLink.AvailableLeaf;
			if (leaf == NULL)
			{
				// No open leaf available for us; back out
				UE_LOG(LogDungeonGen, Warning, TEXT("%s could not find an open leaf."), *Node->GetSymbolDescription());
				return false;
			}
		}
		else
		{
			leafLink = GetOpenLeaf(Node, AllOpenLeaves, Rng, ProcessedLeaves);
			leaf = leafLink.AvailableLeaf;
			if (leaf == NULL)
			{
				// No open leaf available for us; back out
				UE_LOG(LogDungeonGen, Warning, TEXT("%s could not find an open leaf."), *Node->GetSymbolDescription());
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
		// Let this leaf contain the room symbol
		leaf->SetMissionNode(Node, Rng);
		if (leafLink.FromLeaf != NULL)
		{
			leaf->AddMissionLeaf(leafLink.FromLeaf);
		}
		// Set up a connection to our "parent" leaf (the leaf the player enters this room from)
		// This is one of the neighboring leaves that we have already processed
		/*TSet<UBSPLeaf*> processedNeighbors;
		for (UBSPLeaf* neighbor : leaf->LeafNeighbors.Intersect(ProcessedLeaves))
		{
			// This is a neighbor of ours which has been processed already
			if (neighbor->RoomSymbol == NULL || neighbor->RoomSymbol->Symbol.Symbol == NULL)
			{
				continue;
			}
			UDungeonMissionSymbol* neighborSymbol = (UDungeonMissionSymbol*)neighbor->RoomSymbol->Symbol.Symbol;
			if (!neighborSymbol->bAllowedToHaveChildren)
			{
				// We couldn't have come from this node, since it's not allowed to have children
				continue;
			}
			if (Node->IsChildOf(neighbor->RoomSymbol))
			{
				processedNeighbors.Add(neighbor);
			}
		}
		if (processedNeighbors.Num() > 0)
		{
			// Let this leaf contain the room symbol
			leaf->SetMissionNode(Node, Rng);
			UBSPLeaf* entrance = processedNeighbors.Array()[Rng.RandRange(0, processedNeighbors.Num() - 1)];
			leaf->AddMissionLeaf(entrance);
		}
		else if (StartLeaf == leaf)
		{
			// This is the first leaf we're trying to process
			leaf->SetMissionNode(Node, Rng);
		}
		else
		{
			// Failed to find a connection back to the entrance -- back out
			ProcessedNodes.Remove(Node);
			// Restart -- next time, we'll select a different leaf
			UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find a path back to the entrance."), *Node->GetSymbolDescription());
			return PairNodesToLeaves(Node, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, EntranceLeaf, AllOpenLeaves, bIsTightCoupling);
		}*/

		// Grab all our neighbor leaves, excluding those which have already been processed
		TSet<FBSPLink> neighboringLeaves;
		for (UBSPLeaf* neighbor : leaf->LeafNeighbors)
		{
			if (ProcessedLeaves.Contains(neighbor))
			{
				continue;
			}
			FBSPLink link;
			link.AvailableLeaf = neighbor;
			link.FromLeaf = leaf;
			neighboringLeaves.Add(link);
		}
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
					UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves to match our tightly-coupled leaves."), *Node->GetSymbolDescription());
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
				UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToLeaves(Node, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, EntranceLeaf, AllOpenLeaves, bIsTightCoupling);
			}
		}
		
		return true;
}

FBSPLink ADungeon::GetOpenLeaf(UDungeonMissionNode* Node, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UBSPLeaf*>& ProcessedLeaves)
{

	TSet<UDungeonMissionNode*> nodesToCheck;
	for (FMissionNodeData& neighborNode : Node->NextNodes)
	{
		if (neighborNode.bTightlyCoupledToParent)
		{
			nodesToCheck.Add(neighborNode.Node);
		}
	}
	FBSPLink leaf = FBSPLink();
	do
	{
		if (AvailableLeaves.Num() == 0)
		{
			return FBSPLink();
		}
		//checkf(AvailableLeaves.Num() > 0, TEXT("Not enough leaves for all the rooms we need to generate! We failed to generate room %s (ID %d). You may need to make the leaf BSP bigger."), *Node->GetSymbolDescription(), Node->Symbol.SymbolID);
		int32 leafIndex = Rng.RandRange(0, AvailableLeaves.Num() - 1);
		FBSPLink leafLink = AvailableLeaves.Array()[leafIndex];
		AvailableLeaves.Remove(leafLink);
		leaf = leafLink;
		if (ProcessedLeaves.Contains(leaf.AvailableLeaf))
		{
			// Already processed this leaf
			leaf = FBSPLink();
			continue;
		}
		TSet<UBSPLeaf*> neighbors = leaf.AvailableLeaf->LeafNeighbors.Difference(ProcessedLeaves);
		if (neighbors.Num() < nodesToCheck.Num())
		{
			// This leaf wouldn't have enough neighbors to attach all our tightly-coupled nodes
			UE_LOG(LogDungeonGen, Warning, TEXT("Abandoning processing %s for node %s because it has fewer neighbors (%d) than it does tightly-coupled nodes (%d)."), *leaf.AvailableLeaf->GetName(), *Node->GetSymbolDescription(), AvailableLeaves.Num(), nodesToCheck.Num());
			leaf = FBSPLink();
			continue;
		}
	} while (leaf.AvailableLeaf == NULL);
	return leaf;
}

