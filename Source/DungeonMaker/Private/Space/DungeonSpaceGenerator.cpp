// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonSpaceGenerator.h"


// Sets default values for this component's properties
UDungeonSpaceGenerator::UDungeonSpaceGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonSpaceGenerator::CreateDungeonSpace(int32 DungeonSize, UDungeonMissionNode* Head, FRandomStream& Rng)
{
	RootLeaf = NewObject<UBSPLeaf>();
	RootLeaf->InitializeLeaf(0, 0, DungeonSize, DungeonSize, NULL);


	bool didSplit = true;
	TArray<UBSPLeaf*> leaves;
	leaves.Add(RootLeaf);

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
			if (leaf->SideIsLargerThan(MaxRoomSize) || Rng.GetFraction() > 0.25f)
			{
				if (leaf->Split(Rng))
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

	TSet<UDungeonMissionNode*> processedNodes;
	TSet<UBSPLeaf*> processedLeaves;
	PairNodesToLeaves(Head, availableLeaves, Rng, processedNodes, processedLeaves, StartLeaf, availableLeaves);
	
	// Processed leaves may contain some rejected leaves; empty it and reuse it to find all our mission leaves
	processedLeaves.Empty();
	processedLeaves.Add(StartLeaf);
	while (processedLeaves.Num() > 0)
	{
		UBSPLeaf* current = processedLeaves.Array()[0];
		MissionLeaves.Add(current);
		processedLeaves.Remove(current);
		// Add all neighbors we have not yet processed to the array
		processedLeaves.Append(current->MissionNeighbors.Difference(MissionLeaves));
	}

	for (UBSPLeaf* leaf : MissionLeaves)
	{
		TSet<const UDungeonTile*> roomTiles = leaf->Room.FindAllTiles();
		for (const UDungeonTile* tile : roomTiles)
		{
			if (ComponentLookup.Contains(tile) || tile->TileMesh == NULL)
			{
				continue;
			}
			// Otherwise, create a new InstancedStaticMeshComponent
			UHierarchicalInstancedStaticMeshComponent* tileMesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOuter(), tile->TileID);
			tileMesh->RegisterComponent();
			tileMesh->SetStaticMesh(tile->TileMesh);
			tileMesh->SetCastShadow(false);
			ComponentLookup.Add(tile, tileMesh);
		}
	}

	for (UBSPLeaf* leaf : MissionLeaves)
	{
		leaf->PlaceRoomTiles(ComponentLookup);
	}
}

void UDungeonSpaceGenerator::DrawDebugSpace(AActor* ReferenceActor) const
{
	if (RootLeaf == NULL)
	{
		return;
	}
	RootLeaf->DrawDebugLeaf(ReferenceActor);
}

bool UDungeonSpaceGenerator::PairNodesToLeaves(UDungeonMissionNode* Node,
	TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng,
	TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves,
	UBSPLeaf* EntranceLeaf, TSet<FBSPLink>& AllOpenLeaves,
	bool bIsTightCoupling)
{
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
	
	ProcessedLeaves.Add(leaf);
	ProcessedNodes.Add(Node);
	// Let this leaf contain the room symbol
	leaf->SetMissionNode(Node, DefaultRoomTile, Rng);
	if (leafLink.FromLeaf != NULL)
	{
		leaf->AddMissionLeaf(leafLink.FromLeaf);
	}

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
		}
		else
		{
			nextToProcess.Add(neighborNode.Node);
		}
	}

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

FBSPLink UDungeonSpaceGenerator::GetOpenLeaf(UDungeonMissionNode* Node, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UBSPLeaf*>& ProcessedLeaves)
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
