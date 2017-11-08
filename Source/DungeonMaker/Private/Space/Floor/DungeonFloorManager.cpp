// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloorManager.h"
#include "DungeonMissionSymbol.h"


void UDungeonFloorManager::DrawDebugSpace(int32 ZOffset)
{
	DungeonFloor.DrawDungeonFloor(GetOwner(), RoomSize, ZOffset);
}

FFloorRoom UDungeonFloorManager::GetRoomFromFloorCoordinates(FIntVector FloorSpaceCoordinates)
{
	UDungeonFloorManager* manager = FindFloorManagerForLocation(FloorSpaceCoordinates);
	if (manager == NULL)
	{
		return FFloorRoom();
	}
	return manager->DungeonFloor[FloorSpaceCoordinates.Y][FloorSpaceCoordinates.X];
}

FFloorRoom UDungeonFloorManager::GetRoomFromTileSpace(FIntVector TileSpaceLocation)
{
	// Convert to floor space
	FIntVector floorSpaceLocation = ConvertToFloorSpace(TileSpaceLocation);
	return GetRoomFromFloorCoordinates(floorSpaceLocation);
}

// Sets default values for this component's properties
/* UDungeonSpaceGenerator::UDungeonSpaceGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MaxGeneratedRooms = -1;
}

void UDungeonSpaceGenerator::CreateDungeonSpace(UDungeonMissionNode* Head, int32 SymbolCount, FRandomStream& Rng)
{
	TotalSymbolCount = SymbolCount;
	DungeonSpace.AddDefaulted(1);

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

	UE_LOG(LogDungeonGen, Log, TEXT("Created %d leaves, matching %d nodes."), MissionLeaves.Num(), processedNodes.Num());

	// Once we're done making leaves, do some post-processing
	for (UBSPLeaf* leaf : MissionLeaves)
	{
		leaf->UpdateRoomWithNeighbors();
	}

	// Place our rooms in the dungeon, so we know where they are
	// during hallway generation
	for (ADungeonRoom* room : MissionRooms)
	{
		room->UpdateDungeonFloor(DungeonSpace[0]);
	}

	TSet<ADungeonRoom*> newHallways;
	for (ADungeonRoom* room : MissionRooms)
	{
		TSet<ADungeonRoom*> roomHallways = room->MakeHallways(Rng, DefaultFloorTile, 
			DefaultWallTile, DefaultEntranceTile, HallwaySymbol, DungeonSpace[0]);
		for (ADungeonRoom* hallway : roomHallways)
		{
			hallway->UpdateDungeonFloor(DungeonSpace[0]);
		}
		newHallways.Append(roomHallways);
	}
	MissionRooms.Append(newHallways);

	DoFloorWideTileReplacement(DungeonSpace[0], PreGenerationRoomReplacementPhases, Rng);

	for (ADungeonRoom* room : MissionRooms)
	{
		room->DoTileReplacement(DungeonSpace[0], Rng);

		TSet<const UDungeonTile*> roomTiles = room->FindAllTiles();
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
			ComponentLookup.Add(tile, tileMesh);
		}
	}

	DoFloorWideTileReplacement(DungeonSpace[0], PostGenerationRoomReplacementPhases, Rng);

	if (bDebugDungeon)
	{
		DrawDebugSpace();
	}
	else
	{
		for (ADungeonRoom* room : MissionRooms)
		{
			room->PlaceRoomTiles(ComponentLookup, Rng, DungeonSpace[0]);
			room->OnRoomGenerationComplete();
		}
	}
}

void UDungeonSpaceGenerator::DrawDebugSpace()
{
	for (ADungeonRoom* room : MissionRooms)
	{
		room->DrawDebugRoom();
	}
	DungeonSpace[0].DrawDungeonFloor(GetOwner(), 1);
}

void UDungeonSpaceGenerator::DoFloorWideTileReplacement(FDungeonFloor& DungeonFloor, TArray<FRoomReplacements> ReplacementPhases, FRandomStream &Rng)
{
	// Replace them based on our replacement rules
	for (int i = 0; i < ReplacementPhases.Num(); i++)
	{
		TArray<URoomReplacementPattern*> replacementPatterns = ReplacementPhases[i].ReplacementPatterns;
		while (replacementPatterns.Num() > 0)
		{
			int32 rngIndex = Rng.RandRange(0, replacementPatterns.Num() - 1);
			if (!replacementPatterns[rngIndex]->FindAndReplaceFloor(DungeonFloor))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}
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
		UE_LOG(LogDungeonGen, Log, TEXT("Deferring processing of %s because not all its parents have been processed yet (%d / %d)."), *Node->GetSymbolDescription(), ProcessedNodes.Intersect(Node->ParentNodes).Num(), Node->ParentNodes.Num());
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
	if (MaxGeneratedRooms >= 0 && MaxGeneratedRooms <= MissionRooms.Num())
	{
		// Generated max number of rooms
		return true;
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
	leaf->SetMissionNode(Node);
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
			if (MaxGeneratedRooms < 0 || MaxGeneratedRooms > MissionRooms.Num() + 1)
			{
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
	TArray<UDungeonMissionNode*> deferredNodes;
	// Now we process all non-tightly coupled nodes
	for (int i = 0; i < nextToProcess.Num(); i++)
	{
		if (MaxGeneratedRooms < 0 || MaxGeneratedRooms > MissionRooms.Num() + 1)
		{
			// If we're not tightly coupled, ensure that we have all our required parents generated
			if (ProcessedNodes.Intersect(nextToProcess[i]->ParentNodes).Num() != nextToProcess[i]->ParentNodes.Num())
			{
				// Defer processing a bit
				deferredNodes.Add(nextToProcess[i]);
				continue;
			}
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
	}


	// All done making the leaf!
	// Now we make our room
	FString roomName = leaf->RoomSymbol->GetSymbolDescription();
	roomName.Append(" (");
	roomName.AppendInt(leaf->RoomSymbol->Symbol.SymbolID);
	roomName.AppendChar(')');
	ADungeonRoom* room = (ADungeonRoom*)GetWorld()->SpawnActor(((UDungeonMissionSymbol*)Node->Symbol.Symbol)->GetRoomType(Rng));
#if WITH_EDITOR
	room->SetFolderPath("Rooms");
#endif
	room->Rename(*roomName);
	UE_LOG(LogDungeonGen, Log, TEXT("Created room for %s."), *roomName);
	room->InitializeRoom(DefaultFloorTile, DefaultWallTile, DefaultEntranceTile, 
		(float)Node->Symbol.SymbolID / TotalSymbolCount, leaf->LeafSize.XSize(), leaf->LeafSize.YSize(), 
		leaf->XPosition, leaf->YPosition, 0,
		(UDungeonMissionSymbol*)Node->Symbol.Symbol, Rng);

	leaf->SetRoom(room);

	if (room->IsChangedAtRuntime())
	{
		UnresolvedHooks.Add(room);
	}
	MissionRooms.Add(room);
	MissionLeaves.Add(leaf);

	// Attempt to place our deferred nodes
	TMap<UDungeonMissionNode*, uint8> attemptCount;
	const uint8 MAX_ATTEMPT_COUNT = 12;
	while (deferredNodes.Num() > 0)
	{
		UDungeonMissionNode* currentNode = deferredNodes[0];
		deferredNodes.RemoveAt(0);
		if (attemptCount.Contains(currentNode))
		{
			attemptCount[currentNode]++;
		}
		else
		{
			attemptCount.Add(currentNode, 1);
		}

		if (ProcessedNodes.Intersect(currentNode->ParentNodes).Num() == currentNode->ParentNodes.Num())
		{
			bool bSuccesfullyPairedChild = PairNodesToLeaves(currentNode, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, leaf, AllOpenLeaves, false);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToLeaves(Node, AvailableLeaves, Rng, ProcessedNodes, ProcessedLeaves, EntranceLeaf, AllOpenLeaves, bIsTightCoupling);
			}
			else
			{
				// Stop keeping track of this
				attemptCount.Remove(currentNode);
			}
		}
		else
		{
			if (attemptCount[currentNode] < MAX_ATTEMPT_COUNT)
			{
				deferredNodes.Add(currentNode);
			}
		}
	}

	if (attemptCount.Num() > 0)
	{
		UE_LOG(LogDungeonGen, Log, TEXT("Couldn't generate %d rooms!"), attemptCount.Num());
		for (auto& kvp : attemptCount)
		{
			UDungeonMissionNode* node = kvp.Key;
			FString roomName = node->Symbol.GetSymbolDescription();
			roomName.Append(" (");
			roomName.AppendInt(node->Symbol.SymbolID);
			roomName.AppendChar(')');
			UE_LOG(LogDungeonGen, Log, TEXT("%s is missing:"), *roomName);
			TSet<UDungeonMissionNode*> missingNodes = node->ParentNodes.Difference(ProcessedNodes);

			for (UDungeonMissionNode* parent : missingNodes)
			{
				FString parentRoomName = parent->Symbol.GetSymbolDescription();
				parentRoomName.Append(" (");
				parentRoomName.AppendInt(parent->Symbol.SymbolID);
				parentRoomName.AppendChar(')');
				UE_LOG(LogDungeonGen, Log, TEXT("%s"), *parentRoomName);
			}
		}
	}

	return true;
}*/

void UDungeonFloorManager::CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
	int32 SymbolCount, FRandomStream& Rng)
{
	// Create space for each room on the DungeonFloor
	GenerateDungeonRooms(Head, StartLocation, Rng, SymbolCount);
}

void UDungeonFloorManager::InitializeDungeonFloor()
{
	// Initialize the dungeon floor to be the appropriate size
	int32 floorSideSize = FMath::CeilToInt(FMath::Sqrt((float)FloorSize / (float)RoomSize));
	DungeonFloor = FDungeonFloor(floorSideSize, floorSideSize);
}

const UDungeonTile* UDungeonFloorManager::GetTileFromTileSpace(FIntVector TileSpaceLocation)
{
	FIntVector floorSpaceLocation = ConvertToFloorSpace(TileSpaceLocation);
	FFloorRoom room = GetRoomFromFloorCoordinates(floorSpaceLocation);
	if (room.SpawnedRoom == NULL)
	{
		UE_LOG(LogDungeonGen, Warning, TEXT("Tile has not been placed yet at (%d, %d, %d)."), TileSpaceLocation.X, TileSpaceLocation.Y, TileSpaceLocation.Z);
		return NULL;
	}
	FIntVector localTileOffset = TileSpaceLocation - floorSpaceLocation;
	return room.SpawnedRoom->GetTile(localTileOffset.X, localTileOffset.Y);
}

void UDungeonFloorManager::UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile)
{
	FIntVector floorSpaceLocation = ConvertToFloorSpace(TileSpaceLocation);
	FFloorRoom room = GetRoomFromFloorCoordinates(floorSpaceLocation);
	if (room.SpawnedRoom == NULL)
	{
		UE_LOG(LogDungeonGen, Warning, TEXT("Tile has not been placed yet at (%d, %d, %d)."), TileSpaceLocation.X, TileSpaceLocation.Y, TileSpaceLocation.Z);
		return;
	}
	FIntVector localTileOffset = TileSpaceLocation - floorSpaceLocation;
	room.SpawnedRoom->SetTileGridCoordinates(localTileOffset, NewTile);
}

int UDungeonFloorManager::XSize() const
{
	return DungeonFloor.XSize();
}

int UDungeonFloorManager::YSize() const
{
	return DungeonFloor.YSize();
}

void UDungeonFloorManager::ProcessTightlyCoupledNodes(UDungeonMissionNode* Parent,
	FFloorRoom& ParentRoom, TMap<UDungeonMissionNode*, FIntVector>& NodeLocations,
	FRandomStream& Rng, TMap<FIntVector, FIntVector>& AvailableRooms,
	FIntVector& GrandparentLocation, int32 SymbolCount, TMap<FIntVector, FFloorRoom>& ReservedRooms)
{
	// Gather up all the children we need
	TSet<UDungeonMissionNode*> tightlyCoupledChildren;
	for (FMissionNodeData child : Parent->NextNodes)
	{
		// We only care about tightly-coupled nodes
		if (child.bTightlyCoupledToParent)
		{
			tightlyCoupledChildren.Add(child.Node);
		}
	}

	// This keeps track of all nodes we're going to need to reserve
	// It's an array of the "undo" stack for each child
	TMap<UDungeonMissionNode*, TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>>> nodesToReserve;

	// Process all children
	while (tightlyCoupledChildren.Num() < 0)
	{
		UDungeonMissionNode* child = tightlyCoupledChildren.Array()[0];

		TSet<FIntVector> availableParentRooms = GetAvailableLocations(ParentRoom.Location);
		// This node begins a tightly-coupled chain
		// Sort the child nodes
		TArray<UDungeonMissionNode*> nextNodes = UDungeonMissionNode::GetDepthFirstSortedNodes(child, true);

		// Maintain an "undo" list of rooms for this child
		TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>> undoList;

		// This set contains a list of all nodes which have been finalized and placed,
		// but not added to the actual dungeon floor yet
		TSet<FIntVector> placedLocations;
		// ...like our parent room
		placedLocations.Add(ParentRoom.Location);

		// Process each node in the tightly-coupled chain
		// This will break once all tightly-coupled nodes have been processed
		while (nextNodes.Num() > 0)
		{
			// Process the head node
			UDungeonMissionNode* node = nextNodes[0];
			nextNodes.RemoveAt(0);
			if (NodeLocations.Contains(node))
			{
				// Already processed in some way
				continue;
			}

			// Keep track of all locations attempted this cycle
			TSet<FIntVector> attemptedLocations = TSet<FIntVector>(placedLocations);

			// Find an open space bordering our parent
			TArray<FIntVector> parentOpenSpaces = availableParentRooms.Array();
			FIntVector childLocation = parentOpenSpaces[Rng.RandRange(0, parentOpenSpaces.Num())];

			// Update our parent room list using this child as the parent
			// The depth-first search should filter out everything but things which are
			// tightly-coupled to us.
			availableParentRooms = GetAvailableLocations(childLocation, attemptedLocations);

			// Update the lists with our new child location
			attemptedLocations.Add(childLocation);
			undoList.Add(TKeyValuePair<UDungeonMissionNode*, FIntVector>(node, childLocation));

			bool isValid = VerifyParentRoomList(availableParentRooms, undoList, 
				AvailableRooms, nodesToReserve, tightlyCoupledChildren, ParentRoom.Location, Rng, 
				GrandparentLocation, placedLocations, nextNodes, attemptedLocations);
			check(isValid);
		}
		// Tightly-coupled chain has been finalized
		// Let us process it next time
		nodesToReserve.Add(child, undoList);
	}

	// All done!
	// Time to actually reserve the nodes
	for (auto& kvp : nodesToReserve)
	{
		TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>> undoList = kvp.Value;
		for (int i = 0; i < undoList.Num(); i++)
		{
			// Add it to the location lookup
			NodeLocations.Add(undoList[i].Key, undoList[i].Value);
			// Create a room based on this node
			FFloorRoom childRoom = MakeFloorRoom(undoList[i].Key, undoList[i].Value, Rng, SymbolCount);

			// Point the parent to point at this child (and vice versa)
			if (i == 0)
			{
				ParentRoom.NeighboringTightlyCoupledRooms.Add(childRoom.Location);
				childRoom.NeighboringTightlyCoupledRooms.Add(ParentRoom.Location);
				childRoom.IncomingRoom = ParentRoom.Location;
			}
			else
			{
				ReservedRooms[undoList[i - 1].Value].NeighboringTightlyCoupledRooms.Add(childRoom.Location);
				childRoom.NeighboringTightlyCoupledRooms.Add(ReservedRooms[undoList[i - 1].Value].Location);
				childRoom.IncomingRoom = ReservedRooms[undoList[i - 1].Value].Location;
			}

			// Reserve it, but don't add its children to the main open node array yet
			ReservedRooms.Add(undoList[i].Value, childRoom);

			// The room will be actually placed and processed once the "main" search 
			// algorithm catches up to this node. That way, keys don't get placed behind locks
		}
	}
}

bool UDungeonFloorManager::VerifyParentRoomList(TSet<FIntVector>& AvailableParentRooms, 
	TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>>& UndoList, 
	TMap<FIntVector, FIntVector>& AvailableRooms, 
	TMap<UDungeonMissionNode*, TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>>>& NodesToReserve, 
	TSet<UDungeonMissionNode*>& TightlyCoupledChildren, FIntVector& ParentLocation, 
	FRandomStream& Rng, FIntVector& GrandparentLocation, TSet<FIntVector>& PlacedLocations, 
	TArray<UDungeonMissionNode *>& NextNodes, TSet<FIntVector> AttemptedLocations)
{
	// Check to ensure the next step will be able to place a valid room
	// If not:
	// * Select another available parent room, filtering out the room we just tried
	// * If we're out of rooms we haven't tried, step "back in time" to the last 
	// node we processed
	// * If we're out of nodes to process in this chain, something went wrong with
	// the parent to the chain -- try to find another place to stick the parent
	// * Error out if we run out of places to stick our parent room
	while (AvailableParentRooms.Num() == 0)
	{
		// We can't place this room here!
		// We don't have enough spaces to place this chain
		UE_LOG(LogDungeonGen, Warning, TEXT("Ran out of open rooms when trying to create child room!"));

		if (UndoList.Num() == 0)
		{
			// Out of undos for this tightly-coupled chain!
			UE_LOG(LogDungeonGen, Warning, TEXT("Ran out of undos!"));
			// Fail if we're out of rooms
			return false;

			// We're going to pick a new location for our parent
			// This means that all work involving child rooms we've previously worked
			// on is now invalid

			// Mark the nodes as not yet processed
			for (auto& kvp : NodesToReserve)
			{
				TightlyCoupledChildren.Add(kvp.Key);
			}
			// Clear the stack of processed nodes
			NodesToReserve.Empty();

			// Select a new parent location
			TArray<FIntVector> availableRoomLocations;
			AvailableRooms.GetKeys(availableRoomLocations);
			// Select a random location
			ParentLocation = availableRoomLocations[Rng.RandRange(0, availableRoomLocations.Num() - 1)];
			GrandparentLocation = AvailableRooms[ParentLocation];
			AvailableRooms.Remove(ParentLocation);

			// We're restarting from the top, so with different starting point
			// an already-tried location may be valid
			PlacedLocations.Empty();
			PlacedLocations.Add(ParentLocation);

			// Get all newly-available child locations
			AvailableParentRooms = GetAvailableLocations(ParentLocation);
			// We know there's at least one valid room left due to the check above
			// The available parent rooms will get processed in the next iteration
			// of the while loop
		}
		else
		{
			// Fetch the most recent undo
			TKeyValuePair<UDungeonMissionNode*, FIntVector> undo = UndoList[UndoList.Num() - 1];
			// Remove it from the undo list
			UndoList.RemoveAt(UndoList.Num() - 1);
			// Ensure we process this node next
			NextNodes.Insert(undo.Key, 0);
			// Get a new list of parent rooms
			// The attemptedLocations filter ensures we won't select the same location that
			// got us into this mess
			AvailableParentRooms = GetAvailableLocations(undo.Value, AttemptedLocations);
			// If there are no available parent rooms, the while loop we're in right
			// now will restart
			// Otherwise, we'll retry processing this node, selecting a different room
		}
	}
	return true;
}

TSet<FIntVector> UDungeonFloorManager::GetAvailableLocations(FIntVector Location, 
	TSet<FIntVector> IgnoredLocations /*= TSet<FIntVector>()*/)
{
	TSet<FIntVector> availableLocations;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int z = -1; z <= 1; z++)
			{
				// Don't count ourselves
				if (x == 0 && y == 0 && z == 0)
				{
					continue;
				}
				// Don't count diagonals
				if ((x == 1 || x == -1) && (y == 1 || y == -1))
				{
					continue;
				}
				// Z only counts directly above or directly below
				if ((z == 1 || z == -1) && x != 0 && y != 0)
				{
					continue;
				}
				// We only want to include Z if we have a valid Z neighboring us
				if (z == 1 && TopNeighbor == NULL || z == -1 && BottomNeighbor == NULL)
				{
					continue;
				}

				FIntVector possibleLocation = Location + FIntVector(x, y, z);
				if (IgnoredLocations.Contains(possibleLocation))
				{
					// Ignoring this location
					continue;
				}

				if (GetRoomFromFloorCoordinates(possibleLocation).RoomClass != NULL)
				{
					// Room already placed
					continue;
				}

				availableLocations.Add(possibleLocation);
			}
		}
	}
	return availableLocations;
}

FFloorRoom UDungeonFloorManager::MakeFloorRoom(UDungeonMissionNode* Node, FIntVector Location, 
	FRandomStream& Rng, int32 TotalSymbolCount)
{
	FFloorRoom room = FFloorRoom();
	room.RoomClass = ((UDungeonMissionSymbol*)Node->Symbol.Symbol)->GetRoomType(Rng);
	room.Location = Location;
	room.Difficulty = Node->Symbol.SymbolID / (float)TotalSymbolCount;
	return room;
}

void UDungeonFloorManager::SetRoom(FFloorRoom Room)
{
	// Verify that the location is valid
	UDungeonFloorManager* manager = FindFloorManagerForLocation(Room.Location);
	if (manager == NULL)
	{
		UE_LOG(LogDungeonGen, Error, TEXT("Could not set room because floor manager was invalid!"));
		return;
	}
	
	manager->DungeonFloor.Set(Room);
}

UDungeonFloorManager* UDungeonFloorManager::FindFloorManagerForLocation(FIntVector Location)
{
	if (Location.X < 0 || Location.Y < 0 || Location.Z < 0)
	{
		// Always false
		return NULL;
	}

	// Check to see if this is on another level
	if ((uint8)Location.Z != DungeonLevel)
	{
		if ((uint8)Location.Z > DungeonLevel)
		{
			if (TopNeighbor == NULL)
			{
				return NULL;
			}
			else
			{
				return TopNeighbor->FindFloorManagerForLocation(Location);
			}
		}
		if ((uint8)Location.Z < DungeonLevel)
		{
			if (BottomNeighbor == NULL)
			{
				return NULL;
			}
			else
			{
				return BottomNeighbor->FindFloorManagerForLocation(Location);
			}
		}
	}
	// DungeonFloor sizes vary from level to level
	if (Location.X > DungeonFloor.XSize())
	{
		return NULL;
	}
	if (Location.Y > DungeonFloor.YSize())
	{
		return NULL;
	}
	return this;
}

void UDungeonFloorManager::AddChild(FIntVector RoomLocation, FIntVector RoomParent)
{
	DungeonFloor.UpdateChildren(RoomLocation, RoomParent);
}

void UDungeonFloorManager::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	TArray<UDungeonMissionNode*> missionNodes = UDungeonMissionNode::GetTopologicalSortedNodes(Head);
	// A map where the key is a list of available locations
	// and the value is the origin location.
	TMap<FIntVector, FIntVector> availableRooms;
	
	// Add our start location as an available room
	availableRooms.Add(StartLocation, FIntVector::ZeroValue);

	// Where rooms are "reserved".
	TMap<FIntVector, FFloorRoom> reservedRooms;
	// A map of nodes to their locations.
	TMap<UDungeonMissionNode*, FIntVector> nodeLocations;
	bool bWasReserved = false;

	while (missionNodes.Num() > 0)
	{
		UDungeonMissionNode* next = missionNodes[0];
		missionNodes.RemoveAt(0);

		FFloorRoom room;

		if (nodeLocations.Contains(next))
		{
			// Already reserved or placed; time to find out which
			FIntVector foundLocation = nodeLocations[next];
			if (reservedRooms.Contains(foundLocation))
			{
				// We're reserving this room; time to make it physical
				// Tell the main algorithm to use this room
				room = reservedRooms[foundLocation];

				// Remove it from the list of reserved rooms
				reservedRooms.Remove(foundLocation);
				bWasReserved = true;
			}
			else
			{
				// Room already processed, but not reserved
				// We don't care about it anymore
				continue;
			}
		}
		else
		{
			checkf(availableRooms.Num() > 0, TEXT("Ran out of available rooms! Either add more levels to the dungeon, make it bigger, or shrink the size of each room."));

			// If we're here, that means we're a new node
			TArray<FIntVector> availableRoomLocations;
			availableRooms.GetKeys(availableRoomLocations);
			// Select a random location
			FIntVector roomLocation = availableRoomLocations[Rng.RandRange(0, availableRoomLocations.Num() - 1)];
			FIntVector roomParent = availableRooms[roomLocation];
			availableRooms.Remove(roomLocation);

			// Create a room from this node
			room = MakeFloorRoom(next, roomLocation, Rng, SymbolCount);

			// Reserve any tightly-coupled child nodes
			// Note that this method may modify the location this room is being placed in
			ProcessTightlyCoupledNodes(next, room, nodeLocations, Rng, availableRooms,
				roomParent, SymbolCount, reservedRooms);
			// The room location may have been modified; update it
			roomLocation = room.Location;
			// The room's parent may have also been modified
			room.IncomingRoom = roomParent;

			// Place the room on this floor
			nodeLocations.Add(next, roomLocation);
		}

		// Update the DungeonFloor with this room
		SetRoom(room);

		// Add new locations to the available rooms array
		TSet<FIntVector> newLocations = GetAvailableLocations(room.Location);
		for (FIntVector location : newLocations)
		{
			availableRooms.Add(location, room.Location);
		}

		// Update our list of child rooms
		// Reserved rooms have already done this, since they have a special
		// list of tightly-coupled neighbors
		if (!bWasReserved)
		{
			AddChild(room.Location, room.IncomingRoom);
		}
	}
}

FIntVector UDungeonFloorManager::ConvertToFloorSpace(FIntVector TileSpaceVector) const
{
	// Floor space is found by dividing by how big each room is, then rounding down
	// As an example, if the room is 24 tiles long and the location is 22x22, it
	// would return the room located at 0, 0 (which stretches from (0,0) to (23, 23)).
	TileSpaceVector.X = FMath::FloorToInt(TileSpaceVector.X / (float)RoomSize);
	TileSpaceVector.Y = FMath::FloorToInt(TileSpaceVector.Y / (float)RoomSize);
	// Z is left alone -- it's assumed that Z in tile space and floor space are the same
	return TileSpaceVector;
}
