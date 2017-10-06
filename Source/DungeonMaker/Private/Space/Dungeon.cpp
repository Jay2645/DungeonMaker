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
	int32 x = RoomMap.Num() + 128;
	int32 dungeonSize = FMath::CeilToInt(((x * x) / 1378) + ((1319 * x) / 1378) + (26526 / 689));

	UBSPLeaf* rootLeaf = UBSPLeaf::CreateLeaf(this, NULL, TEXT("Root Leaf"), 0, 0, dungeonSize, dungeonSize);
	leaves.Add(rootLeaf);

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

	int32 leafCount = rootLeaf->GetChildLeafCount();
	int32 roomCount = RoomMap.Num();
	UE_LOG(LogDungeonGen, Warning, TEXT("Generated %d leaves and %d rooms."), leafCount, roomCount);

	UBSPLeaf* entranceLeaf = rootLeaf;
	while (entranceLeaf->LeftChild != NULL)
	{
		entranceLeaf = entranceLeaf->LeftChild;
	}
	TSet<UBSPLeaf*> availableLeaves;
	availableLeaves.Add(entranceLeaf);
	// The toProcess array is now empty from the while loop we ran earlier
	toProcess.Add(Mission->Head);
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<UBSPLeaf*> processedLeaves;

	
	PairNodesToLeaves(toProcess, availableLeaves, rng, processedNodes, processedLeaves);
	rootLeaf->DrawDebugLeaf();
}

void ADungeon::PairNodesToLeaves(TArray<UDungeonMissionNode*>& ToProcess, TSet<UBSPLeaf*>& AvailableLeaves, FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves)
{
	//AvailableLeaves = AvailableLeaves.Difference(ProcessedLeaves);

	while (ToProcess.Num() > 0)
	{
		checkf(AvailableLeaves.Num() > 0, TEXT("Not enough leaves for all the rooms we need to generate! You need to make the leaf BSP bigger."));
		
		UDungeonMissionNode* node = ToProcess[0];
		ToProcess.RemoveAt(0);
		if (ProcessedNodes.Contains(node))
		{
			// Already processed this node
			continue;
		}

		// First, try to find a leaf big enough for this room
		TArray<UBSPLeaf*> leavesToTry = AvailableLeaves.Array();
		/*int32 roomXY = ((UDungeonMissionSymbol*)node->Symbol.Symbol)->MinimumRoomSize.WallSize;
		for (UBSPLeaf* availableLeaf : AvailableLeaves)
		{
			int32 leafWidth = availableLeaf->Room.XSize();
			int32 leafHeight = availableLeaf->Room.YSize();

			if (leafWidth >= roomXY && leafHeight >= roomXY)
			{
				// This leaf is big enough to contain this room
				leavesToTry.Add(availableLeaf);
			}
		}
		// If we have no rooms which are big enough, pull from the pool of all leaves
		if (leavesToTry.Num() == 0)
		{
			UE_LOG(LogDungeonGen, Error, TEXT("No leaf was big enough to contain %s! Using a leaf at random."), *node->GetSymbolDescription());
			leavesToTry.Append(AvailableLeaves.Array());
		}*/

		UBSPLeaf* leaf = NULL;
		while (leaf == NULL)
		{
			int32 leafIndex = Rng.RandRange(0, leavesToTry.Num() - 1);
			leaf = leavesToTry[leafIndex];
			leavesToTry.RemoveAt(leafIndex);
			if (ProcessedLeaves.Contains(leaf))
			{
				leaf = NULL;
			}
		}
		AvailableLeaves.Remove(leaf);

		// Let this leaf contain the room symbol
		leaf->RoomSymbol = node;

		ProcessedLeaves.Add(leaf);
		ProcessedNodes.Add(node);

		TSet<UBSPLeaf*> neighboringLeaves;
		neighboringLeaves.Append(leaf->Neighbors);

		TArray<UDungeonMissionNode*> tightlyCoupledNodes;
		for (FMissionNodeData& neighborNode : node->NextNodes)
		{
			if (neighborNode.bTightlyCoupledToParent)
			{
				tightlyCoupledNodes.Add(neighborNode.Node);
			}
			else
			{
				ToProcess.Add(neighborNode.Node);
			}
		}
		if (tightlyCoupledNodes.Num() > 0)
		{
			PairNodesToLeaves(tightlyCoupledNodes, neighboringLeaves, Rng, ProcessedNodes, ProcessedLeaves);
		}
		AvailableLeaves.Append(neighboringLeaves);
	}
}
