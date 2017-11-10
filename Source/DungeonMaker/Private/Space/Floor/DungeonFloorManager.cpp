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

TArray<FFloorRoom> UDungeonFloorManager::GetAllNeighbors(FFloorRoom Room)
{
	TArray<FFloorRoom> neighbors;
	for (FIntVector neighbor : Room.NeighboringRooms)
	{
		UDungeonFloorManager* manager = FindFloorManagerForLocation(neighbor);
		if (manager == NULL)
		{
			continue;
		}
		neighbors.Add(manager->DungeonFloor[neighbor.Y][neighbor.X]);
	}
	for (FIntVector neighbor : Room.NeighboringTightlyCoupledRooms)
	{
		UDungeonFloorManager* manager = FindFloorManagerForLocation(neighbor);
		if (manager == NULL)
		{
			continue;
		}
		neighbors.Add(manager->DungeonFloor[neighbor.Y][neighbor.X]);
	}
	return neighbors;
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
}*/

bool UDungeonFloorManager::PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms,
	FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<FIntVector>& ProcessedRooms,
	FIntVector EntranceRoom, TMap<FIntVector, FIntVector>& AllOpenRooms,
	bool bIsTightCoupling, int32 TotalSymbolCount)
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
	if (AvailableRooms.Num() == 0 && bIsTightCoupling)
	{
		// Out of leaves to process
		UE_LOG(LogDungeonGen, Warning, TEXT("%s is tightly coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}
	if (AllOpenRooms.Num() == 0 && !bIsTightCoupling)
	{
		UE_LOG(LogDungeonGen, Warning, TEXT("%s is loosely coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}

	UE_LOG(LogDungeonGen, Log, TEXT("Creating room for %s! Leaves available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableRooms.Num(), Node->NextNodes.Num());
	// Find an open room to add this to
	TKeyValuePair<FIntVector, FIntVector> roomLocation;
	if (bIsTightCoupling)
	{
		roomLocation = GetOpenRoom(Node, AvailableRooms, Rng, ProcessedRooms);
	}
	else
	{
		roomLocation = GetOpenRoom(Node, AllOpenRooms, Rng, ProcessedRooms);
	}
	
	ProcessedRooms.Add(roomLocation.Key);
	ProcessedNodes.Add(Node);

	// Grab all our neighbor rooms, excluding those which have already been processed
	TSet<FIntVector> neighboringRooms = GetAvailableLocations(roomLocation.Key, ProcessedRooms);
	TMap<FIntVector, FIntVector> roomNeighborMap;
	// Map us to be the neighbor to all our neighbors
	for (FIntVector neighbor : neighboringRooms)
	{
		roomNeighborMap.Add(neighbor, roomLocation.Key);
	}

	// Find all the tightly coupled nodes attached to our current node
	TArray<UDungeonMissionNode*> nextToProcess;
	for (FMissionNodeData& neighborNode : Node->NextNodes)
	{
		if (neighborNode.bTightlyCoupledToParent)
		{
			// If we're tightly coupled to our parent, ensure we get added to a neighboring leaf
				bool bSuccesfullyPairedChild = PairNodesToRooms(neighborNode.Node, roomNeighborMap, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, true, TotalSymbolCount);
				if (!bSuccesfullyPairedChild)
				{
					// Failed to find a child leaf; back out
					ProcessedNodes.Remove(Node);
					// Restart -- next time, we'll select a different leaf
					UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child rooms to match our tightly-coupled rooms."), *Node->GetSymbolDescription());
					return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
				}
		}
		else
		{
			nextToProcess.Add(neighborNode.Node);
		}
	}

	if (((UDungeonMissionSymbol*)Node->Symbol.Symbol)->bAllowedToHaveChildren)
	{
		AvailableRooms.Append(roomNeighborMap);
	}
	AllOpenRooms.Append(AvailableRooms);
	TArray<UDungeonMissionNode*> deferredNodes;
	// Now we process all non-tightly coupled nodes
	for (int i = 0; i < nextToProcess.Num(); i++)
	{
			// If we're not tightly coupled, ensure that we have all our required parents generated
			if (ProcessedNodes.Intersect(nextToProcess[i]->ParentNodes).Num() != nextToProcess[i]->ParentNodes.Num())
			{
				// Defer processing a bit
				deferredNodes.Add(nextToProcess[i]);
				continue;
			}
			bool bSuccesfullyPairedChild = PairNodesToRooms(nextToProcess[i], AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, false, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
			}
	}


	/*// All done making the leaf!
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
	MissionLeaves.Add(leaf);*/

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
			bool bSuccesfullyPairedChild = PairNodesToRooms(currentNode, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, false, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogDungeonGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
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

	// Make the actual room
	FFloorRoom room = MakeFloorRoom(Node, roomLocation.Key, Rng, TotalSymbolCount);
	SetRoom(room);
	UDungeonFloorManager* ourManager = FindFloorManagerForLocation(roomLocation.Key);
	UDungeonFloorManager* otherManager = FindFloorManagerForLocation(roomLocation.Value);
	// Don't bother setting neighbors if one of the neighbors would be invalid
	if (ourManager != NULL && otherManager != NULL)
	{
		// Link the children
		if (bIsTightCoupling)
		{
			ourManager->DungeonFloor.DungeonRooms[roomLocation.Key.Y].DungeonRooms[roomLocation.Key.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Value);
			otherManager->DungeonFloor.DungeonRooms[roomLocation.Value.Y].DungeonRooms[roomLocation.Value.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Key);
		}
		else
		{
			ourManager->DungeonFloor.DungeonRooms[roomLocation.Key.Y].DungeonRooms[roomLocation.Key.X].NeighboringRooms.Add(roomLocation.Value);
			otherManager->DungeonFloor.DungeonRooms[roomLocation.Value.Y].DungeonRooms[roomLocation.Value.X].NeighboringRooms.Add(roomLocation.Key);
		}
	}

	return true;
}

void UDungeonFloorManager::CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
	int32 SymbolCount, FRandomStream& Rng)
{
	bool bPathIsValid = false;
	//do
	//{
		// Create space for each room on the DungeonFloor
		GenerateDungeonRooms(Head, StartLocation, Rng, SymbolCount);
		bPathIsValid = VerifyPathIsValid(StartLocation);
		/*if (!bPathIsValid)
		{
			// Invalid path; restart
			UDungeonFloorManager* next = this;
			do
			{
				next->InitializeDungeonFloor();
				next = next->TopNeighbor;
			} while (next != NULL);
		}
	} while (!bPathIsValid);*/
	if (!bPathIsValid)
	{
		UE_LOG(LogDungeonGen, Error, TEXT("Invalid path!"));
	}
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

TSet<FIntVector> UDungeonFloorManager::GetAvailableLocations(FIntVector Location, 
	TSet<FIntVector> IgnoredLocations /*= TSet<FIntVector>()*/)
{
	TSet<FIntVector> availableLocations;

	UDungeonFloorManager* ourManager = FindFloorManagerForLocation(Location);
	if (ourManager != NULL && ourManager->DungeonFloor[Location.Y][Location.X].DungeonSymbol.Symbol != NULL)
	{
		UDungeonMissionSymbol* symbol = (UDungeonMissionSymbol*)ourManager->DungeonFloor[Location.Y][Location.X].DungeonSymbol.Symbol;
		if (!symbol->bAllowedToHaveChildren)
		{
			// Not allowed to have children; return empty set
			return availableLocations;
		}
	}

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

				UDungeonFloorManager* manager = FindFloorManagerForLocation(possibleLocation);
				if (manager == NULL)
				{
					// Out of range
					continue;
				}
				if (manager->DungeonFloor[possibleLocation.Y][possibleLocation.X].RoomClass != NULL)
				{
					// Already placed
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
	room.DungeonSymbol = Node->Symbol;
	room.RoomNode = Node;
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
	uint8 zLocation = (uint8)Location.Z;
	if (zLocation != DungeonLevel)
	{
		if (zLocation > DungeonLevel)
		{
			if (TopNeighbor == NULL)
			{
				return NULL;
			}
			else
			{
				check(TopNeighbor->DungeonLevel > DungeonLevel);
				return TopNeighbor->FindFloorManagerForLocation(Location);
			}
		}
		if (zLocation < DungeonLevel)
		{
			if (BottomNeighbor == NULL)
			{
				return NULL;
			}
			else
			{
				check(BottomNeighbor->DungeonLevel < DungeonLevel);
				return BottomNeighbor->FindFloorManagerForLocation(Location);
			}
		}
	}
	// DungeonFloor sizes can vary from level to level
	if (Location.X >= DungeonFloor.XSize())
	{
		return NULL;
	}
	if (Location.Y >= DungeonFloor.YSize())
	{
		return NULL;
	}
	return this;
}

void UDungeonFloorManager::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	TMap<FIntVector, FIntVector> availableRooms;
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<FIntVector> processedRooms;
	TMap<FIntVector, FIntVector> openRooms;

	availableRooms.Add(StartLocation, FIntVector(-1, -1, -1));
	openRooms.Add(StartLocation, FIntVector(-1, -1, -1));

	PairNodesToRooms(Head, availableRooms, Rng, processedNodes, processedRooms, StartLocation, openRooms, false, SymbolCount);
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

TKeyValuePair<FIntVector, FIntVector> UDungeonFloorManager::GetOpenRoom(UDungeonMissionNode* Node, 
	TMap<FIntVector, FIntVector>& AvailableRooms, FRandomStream& Rng, TSet<FIntVector>& ProcessedRooms)
{
	TSet<UDungeonMissionNode*> nodesToCheck;
	for (FMissionNodeData& neighborNode : Node->NextNodes)
	{
		if (neighborNode.bTightlyCoupledToParent)
		{
			nodesToCheck.Add(neighborNode.Node);
		}
	}

	FIntVector roomLocation = FIntVector(-1, -1, -1);
	FIntVector parentLocation = FIntVector(-1, -1, -1);

	do
	{
		if (AvailableRooms.Num() == 0)
		{
			return TKeyValuePair<FIntVector, FIntVector>(roomLocation, parentLocation);
		}
		int32 leafIndex = Rng.RandRange(0, AvailableRooms.Num() - 1);
		TArray<FIntVector> allAvailableRooms;
		AvailableRooms.GetKeys(allAvailableRooms);
		roomLocation = allAvailableRooms[leafIndex];
		parentLocation = AvailableRooms[roomLocation];
		AvailableRooms.Remove(roomLocation);

		if (ProcessedRooms.Contains(roomLocation))
		{
			// Already processed this leaf
			roomLocation = FIntVector(-1, -1, -1);
			parentLocation = FIntVector(-1, -1, -1);
			continue;
		}

		TSet<FIntVector> neighbors = GetAvailableLocations(roomLocation, ProcessedRooms);

		if (neighbors.Num() < nodesToCheck.Num())
		{
			// This leaf wouldn't have enough neighbors to attach all our tightly-coupled nodes
			roomLocation = FIntVector(-1, -1, -1);
			parentLocation = FIntVector(-1, -1, -1);
			continue;
		}
	} while (roomLocation.X == -1 && roomLocation.Y == -1 && roomLocation.Z == -1);
	return TKeyValuePair<FIntVector, FIntVector>(roomLocation, parentLocation);
}

bool UDungeonFloorManager::VerifyPathIsValid(FIntVector StartLocation)
{
	TSet<UDungeonMissionNode*> seen;
	TArray<FFloorRoom> nextToProcess;

	UDungeonFloorManager* manager = FindFloorManagerForLocation(StartLocation);
	nextToProcess.Add(manager->DungeonFloor[StartLocation.Y][StartLocation.X]);

	TSet<UDungeonMissionNode*> deferred;
	while (nextToProcess.Num() > 0)
	{
		if (nextToProcess.Num() == deferred.Num())
		{
			// Everything left in this list has already been deferred!
			// Path isn't valid
			UE_LOG(LogDungeonGen, Warning, TEXT("Couldn't process %d nodes because they've been deferred!"), nextToProcess.Num());
			UE_LOG(LogDungeonGen, Warning, TEXT("Nodes left to process:"));
			for (FFloorRoom room : nextToProcess)
			{
				UE_LOG(LogDungeonGen, Warning, TEXT("%s (%d)"), *room.DungeonSymbol.GetSymbolDescription(), room.DungeonSymbol.SymbolID);
			}
			UE_LOG(LogDungeonGen, Warning, TEXT("Deferred nodes"));
			for (UDungeonMissionNode* node : deferred)
			{
				UE_LOG(LogDungeonGen, Warning, TEXT("%s (%d)"), *node->GetSymbolDescription(), node->Symbol.SymbolID);
			}
			return false;
		}
		FFloorRoom next = nextToProcess[0];
		nextToProcess.RemoveAt(0);
		UDungeonMissionNode* node = next.RoomNode;
		if (seen.Contains(node))
		{
			// Already seen this node
			continue;
		}

		// Get a list of all parents not yet processed
		TSet<UDungeonMissionNode*> unprocessedParents = node->ParentNodes.Difference(seen);
		if (unprocessedParents.Num() > 0)
		{
			UE_LOG(LogDungeonGen, Log, TEXT("%s (%d) has %d more parents to process."), *node->GetSymbolDescription(), node->Symbol.SymbolID, unprocessedParents.Num());
			for (UDungeonMissionNode* parent : unprocessedParents)
			{
				UE_LOG(LogDungeonGen, Log, TEXT("Missing: %s (%d)"), *parent->GetSymbolDescription(), parent->Symbol.SymbolID);
			}
			// Defer this node
			nextToProcess.Add(next);
			deferred.Add(node);
		}
		else
		{
			// All parents have been processed!
			// Since we've made some progress, empty the deferred array (only used to tell if we get stuck)
			deferred.Empty();
			// Mark this node as processed
			seen.Add(node);
			// Append all our children to our next to process array
			nextToProcess.Append(GetAllNeighbors(next));
		}
	}
	return true;
}
