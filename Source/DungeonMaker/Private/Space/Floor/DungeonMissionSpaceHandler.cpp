// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionSpaceHandler.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonMissionSymbol.h"

void UDungeonMissionSpaceHandler::InitializeDungeonFloor(UDungeonSpaceGenerator* SpaceGenerator, TArray<int32> LevelSizes)
{
	DungeonSpaceGenerator = SpaceGenerator;

	// All our actual array management is using this pointer to the space generator
	// This is done because Unreal doesn't like having pointers to TArrays, and
	// a reference would need to be initialized to a default value somewhere.

	// Using pointers keeps everything anchored to a single object, so we can change
	// it without needing to worry about updating 50 different references.

	// Additionally, this *should* be a "fire and forget" sort of generation, only ever
	// done once. Because of that, the loss of efficiency that this pointer lookup causes
	// is negligible at best.
	DungeonSpaceGenerator->DungeonSpace = TArray<FDungeonFloor>();
	DungeonSpaceGenerator->DungeonSpace.SetNum(LevelSizes.Num());
	for (int i = 0; i < LevelSizes.Num(); i++)
	{
		DungeonSpaceGenerator->DungeonSpace[i] = FDungeonFloor(LevelSizes[i], LevelSizes[i]);
	}
}

void UDungeonMissionSpaceHandler::CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
	int32 SymbolCount, FRandomStream& Rng)
{
	bool bPathIsValid = false;
	//do
	//{
		// Create space for each room on the DungeonFloor
		GenerateDungeonRooms(Head, StartLocation, Rng, SymbolCount);
		ProcessRoomNeighbors();
		/*bPathIsValid = VerifyPathIsValid(StartLocation);
		if (!bPathIsValid)
		{
			// Invalid path; restart
			TArray<int32> levelSizes;
			for (int i = 0; i < DungeonSpaceGenerator->DungeonSpace.Num(); i++)
			{
				levelSizes.Add(DungeonSpaceGenerator->DungeonSpace[i].XSize());
			}
			InitializeDungeonFloor(DungeonSpaceGenerator, levelSizes);
		}
	} while (!bPathIsValid);*/
}

void UDungeonMissionSpaceHandler::DrawDebugSpace()
{
	for (int i = 0; i < DungeonSpaceGenerator->DungeonSpace.Num(); i++)
	{
		DungeonSpaceGenerator->DungeonSpace[i].DrawDungeonFloor(GetOwner(), RoomSize, i);
	}
}

FFloorRoom UDungeonMissionSpaceHandler::GetRoomFromFloorCoordinates(FIntVector FloorSpaceCoordinates)
{
	if (!IsLocationValid(FloorSpaceCoordinates))
	{
		return FFloorRoom();
	}
	return DungeonSpaceGenerator->DungeonSpace[FloorSpaceCoordinates.Z][FloorSpaceCoordinates.Y][FloorSpaceCoordinates.X];
}

FFloorRoom UDungeonMissionSpaceHandler::GetRoomFromTileSpace(FIntVector TileSpaceLocation)
{
	// Convert to floor space
	FIntVector floorSpaceLocation = ConvertToFloorSpace(TileSpaceLocation);
	return GetRoomFromFloorCoordinates(floorSpaceLocation);
}

bool UDungeonMissionSpaceHandler::IsLocationValid(FIntVector FloorSpaceCoordinates) const
{
	// If it's below 0, it's always invalid
	if (FloorSpaceCoordinates.X < 0 || FloorSpaceCoordinates.Y < 0 || FloorSpaceCoordinates.Z < 0)
	{
		return false;
	}
	// If it's above our total number of floors, it's also invalid
	if (FloorSpaceCoordinates.Z >= DungeonSpaceGenerator->DungeonSpace.Num())
	{
		return false;
	}
	FDungeonFloor floor = DungeonSpaceGenerator->DungeonSpace[FloorSpaceCoordinates.Z];
	return FloorSpaceCoordinates.X < floor.XSize() && FloorSpaceCoordinates.Y < floor.YSize();
}

TArray<FFloorRoom> UDungeonMissionSpaceHandler::GetAllNeighbors(FFloorRoom Room)
{
	TArray<FFloorRoom> neighbors;
	for (FIntVector neighbor : Room.NeighboringRooms)
	{
		if (!IsLocationValid(neighbor))
		{
			continue;
		}
		neighbors.Add(DungeonSpaceGenerator->DungeonSpace[neighbor.Z][neighbor.Y][neighbor.X]);
	}
	for (FIntVector neighbor : Room.NeighboringTightlyCoupledRooms)
	{
		if (!IsLocationValid(neighbor))
		{
			continue;
		}
		neighbors.Add(DungeonSpaceGenerator->DungeonSpace[neighbor.Z][neighbor.Y][neighbor.X]);
	}
	return neighbors;
}

TSet<FIntVector> UDungeonMissionSpaceHandler::GetAvailableLocations(FIntVector Location, 
	TSet<FIntVector> IgnoredLocations /*= TSet<FIntVector>()*/)
{
	TSet<FIntVector> availableLocations;

	if (IsLocationValid(Location) && DungeonSpaceGenerator->DungeonSpace[Location.Z][Location.Y][Location.X].DungeonSymbol.Symbol != NULL)
	{
		UDungeonMissionSymbol* symbol = (UDungeonMissionSymbol*)DungeonSpaceGenerator->DungeonSpace[Location.Z][Location.Y][Location.X].DungeonSymbol.Symbol;
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
				if (z == 1 && Location.Z + 1 < DungeonSpaceGenerator->DungeonSpace.Num() || z == -1 && Location.Z - 1 >= 0)
				{
					continue;
				}

				FIntVector possibleLocation = Location + FIntVector(x, y, z);
				if (IgnoredLocations.Contains(possibleLocation))
				{
					// Ignoring this location
					continue;
				}

				if (!IsLocationValid(possibleLocation))
				{
					// Out of range
					continue;
				}
				if (DungeonSpaceGenerator->DungeonSpace[possibleLocation.Z][possibleLocation.Y][possibleLocation.X].RoomClass != NULL)
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

FFloorRoom UDungeonMissionSpaceHandler::MakeFloorRoom(UDungeonMissionNode* Node, FIntVector Location, 
	FRandomStream& Rng, int32 TotalSymbolCount)
{
	FFloorRoom room = FFloorRoom();
	UDungeonMissionSymbol* symbol = ((UDungeonMissionSymbol*)Node->Symbol.Symbol);
	room.RoomClass = symbol->GetRoomType(Rng);
	room.Location = Location;
	room.Difficulty = Node->Symbol.SymbolID / (float)TotalSymbolCount;
	room.DungeonSymbol = Node->Symbol;
	room.RoomNode = Node;
	return room;
}

void UDungeonMissionSpaceHandler::SetRoom(FFloorRoom Room)
{
	// Verify that the location is valid
	if (!IsLocationValid(Room.Location))
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Could not set room at (%d, %d, %d) because it was an invalid location!"), Room.Location.X, Room.Location.Y, Room.Location.Z);
		return;
	}
	DungeonSpaceGenerator->DungeonSpace[Room.Location.Z].Set(Room);
}

void UDungeonMissionSpaceHandler::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	TMap<FIntVector, FIntVector> availableRooms;
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<FIntVector> processedRooms;
	TMap<FIntVector, FIntVector> openRooms;

	availableRooms.Add(StartLocation, FIntVector(-1, -1, -1));
	openRooms.Add(StartLocation, FIntVector(-1, -1, -1));

	PairNodesToRooms(Head, availableRooms, Rng, processedNodes, processedRooms, StartLocation, openRooms, false, SymbolCount);
}

bool UDungeonMissionSpaceHandler::PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms,
	FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<FIntVector>& ProcessedRooms,
	FIntVector EntranceRoom, TMap<FIntVector, FIntVector>& AllOpenRooms,
	bool bIsTightCoupling, int32 TotalSymbolCount)
{
	if (ProcessedNodes.Contains(Node))
	{
		// Already processed this node
		return true;
	}
	if (Node->Symbol.Symbol == NULL || ((UDungeonMissionSymbol*)Node->Symbol.Symbol)->RoomTypes.Num() == 0)
	{
		// No rooms to pair
		return true;
	}
	if (ProcessedNodes.Intersect(Node->ParentNodes).Num() != Node->ParentNodes.Num())
	{
		// We haven't processed all our parent nodes yet!
		// We should be processed further on down the line, once our next parent node
		// finishes being processed.
		UE_LOG(LogSpaceGen, Log, TEXT("Deferring processing of %s because not all its parents have been processed yet (%d / %d)."), *Node->GetSymbolDescription(), ProcessedNodes.Intersect(Node->ParentNodes).Num(), Node->ParentNodes.Num());
		return true;
	}
	if (AvailableRooms.Num() == 0 && bIsTightCoupling)
	{
		// Out of leaves to process
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is tightly coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}
	if (AllOpenRooms.Num() == 0 && !bIsTightCoupling)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is loosely coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}

	UE_LOG(LogSpaceGen, Log, TEXT("Creating room for %s! Leaves available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableRooms.Num(), Node->NextNodes.Num());
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
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child rooms to match our tightly-coupled rooms."), *Node->GetSymbolDescription());
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
			UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
			return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
		}
	}

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
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
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
		UE_LOG(LogSpaceGen, Log, TEXT("Couldn't generate %d rooms!"), attemptCount.Num());
		for (auto& kvp : attemptCount)
		{
			UDungeonMissionNode* node = kvp.Key;
			FString roomName = node->Symbol.GetSymbolDescription();
			roomName.Append(" (");
			roomName.AppendInt(node->Symbol.SymbolID);
			roomName.AppendChar(')');
			UE_LOG(LogSpaceGen, Log, TEXT("%s is missing:"), *roomName);
			TSet<UDungeonMissionNode*> missingNodes = node->ParentNodes.Difference(ProcessedNodes);

			for (UDungeonMissionNode* parent : missingNodes)
			{
				FString parentRoomName = parent->Symbol.GetSymbolDescription();
				parentRoomName.Append(" (");
				parentRoomName.AppendInt(parent->Symbol.SymbolID);
				parentRoomName.AppendChar(')');
				UE_LOG(LogSpaceGen, Log, TEXT("%s"), *parentRoomName);
			}
		}
	}

	// Make the actual room
	FFloorRoom room = MakeFloorRoom(Node, roomLocation.Key, Rng, TotalSymbolCount);
	SetRoom(room);
	// Don't bother setting neighbors if one of the neighbors would be invalid
	if (IsLocationValid(roomLocation.Key) && IsLocationValid(roomLocation.Value))
	{
		// Link the children
		if (bIsTightCoupling)
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z].DungeonRooms[roomLocation.Key.Y].DungeonRooms[roomLocation.Key.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z].DungeonRooms[roomLocation.Value.Y].DungeonRooms[roomLocation.Value.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Key);
		}
		else
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z].DungeonRooms[roomLocation.Key.Y].DungeonRooms[roomLocation.Key.X].NeighboringRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z].DungeonRooms[roomLocation.Value.Y].DungeonRooms[roomLocation.Value.X].NeighboringRooms.Add(roomLocation.Key);
		}
	}

	return true;
}

FIntVector UDungeonMissionSpaceHandler::ConvertToFloorSpace(FIntVector TileSpaceVector) const
{
	// Floor space is found by dividing by how big each room is, then rounding down
	// As an example, if the room is 24 tiles long and the location is 22x22, it
	// would return the room located at 0, 0 (which stretches from (0,0) to (23, 23)).
	TileSpaceVector.X = FMath::FloorToInt(TileSpaceVector.X / (float)RoomSize);
	TileSpaceVector.Y = FMath::FloorToInt(TileSpaceVector.Y / (float)RoomSize);
	// Z is left alone -- it's assumed that Z in tile space and floor space are the same
	return TileSpaceVector;
}

TKeyValuePair<FIntVector, FIntVector> UDungeonMissionSpaceHandler::GetOpenRoom(UDungeonMissionNode* Node, 
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

bool UDungeonMissionSpaceHandler::VerifyPathIsValid(FIntVector StartLocation)
{
	TSet<UDungeonMissionNode*> seen = TSet<UDungeonMissionNode*>();
	TArray<FFloorRoom> nextToProcess;

	// Make sure the entrance location is valid
	if (!IsLocationValid(StartLocation))
	{
		return false;
	}

	nextToProcess.Add(DungeonSpaceGenerator->DungeonSpace[StartLocation.Z][StartLocation.Y][StartLocation.X]);

	TSet<UDungeonMissionNode*> deferred;
	while (nextToProcess.Num() > 0)
	{
		if (nextToProcess.Num() == deferred.Num())
		{
			// Everything left in this list has already been deferred!
			// Path isn't valid
			UE_LOG(LogSpaceGen, Warning, TEXT("Couldn't process %d nodes because they've been deferred!"), nextToProcess.Num());
			UE_LOG(LogSpaceGen, Warning, TEXT("Nodes left to process:"));
			for (FFloorRoom room : nextToProcess)
			{
				UE_LOG(LogSpaceGen, Warning, TEXT("%s (%d)"), *room.DungeonSymbol.GetSymbolDescription(), room.DungeonSymbol.SymbolID);
			}
			UE_LOG(LogSpaceGen, Warning, TEXT("Deferred nodes"));
			for (UDungeonMissionNode* node : deferred)
			{
				UE_LOG(LogSpaceGen, Warning, TEXT("%s (%d)"), *node->GetSymbolDescription(), node->Symbol.SymbolID);
			}
			return false;
		}
		FFloorRoom next = nextToProcess[0];
		nextToProcess.RemoveAt(0);
		UDungeonMissionNode* node = next.RoomNode;
		if (!IsValid(node) || seen.Contains(node))
		{
			// Already seen this node
			continue;
		}

		UE_LOG(LogSpaceGen, Log, TEXT("Processing %s, with %d parents. We've seen %d nodes so far."), *node->GetSymbolDescription(), node->ParentNodes.Num(), seen.Num());

		if (node->ParentNodes.Num() == 0)
		{
			// All parents have been processed!
			// Since we've made some progress, empty the deferred array (only used to tell if we get stuck)
			deferred.Empty();
			// Mark this node as processed
			seen.Add(node);
			// Append all our children to our next to process array
			nextToProcess.Append(GetAllNeighbors(next));
		}
		else
		{
			TSet<UDungeonMissionNode*> unprocessedParents;
			// There's a strange crash bug right here that happens every once in a while
			// when this Difference function is called
			if (seen.Num() > 0)
			{
				unprocessedParents = node->ParentNodes.Difference(seen);
			}
			else
			{
				unprocessedParents = node->ParentNodes;
			}

			if (unprocessedParents.Num() > 0)
			{
				UE_LOG(LogSpaceGen, Log, TEXT("%s (%d) has %d more parents to process."), *node->GetSymbolDescription(), node->Symbol.SymbolID, unprocessedParents.Num());
				for (UDungeonMissionNode* parent : unprocessedParents)
				{
					UE_LOG(LogSpaceGen, Log, TEXT("Missing: %s (%d)"), *parent->GetSymbolDescription(), parent->Symbol.SymbolID);
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
	}
	return true;
}

void UDungeonMissionSpaceHandler::ProcessRoomNeighbors()
{
	for (int z = 0; z < DungeonSpaceGenerator->DungeonSpace.Num(); z++)
	{
		for (int y = 0; y < DungeonSpaceGenerator->DungeonSpace[z].YSize(); y++)
		{
			for (int x = 0; x < DungeonSpaceGenerator->DungeonSpace[z].XSize(); x++)
			{
				for (FIntVector neighbor : DungeonSpaceGenerator->DungeonSpace[z][y][x].NeighboringRooms)
				{
					DungeonSpaceGenerator->DungeonSpace[neighbor.Z].DungeonRooms[neighbor.Y].DungeonRooms[neighbor.X].NeighboringRooms.Add(FIntVector(x, y, z));
				}
				for (FIntVector neighbor : DungeonSpaceGenerator->DungeonSpace[z][y][x].NeighboringTightlyCoupledRooms)
				{
					DungeonSpaceGenerator->DungeonSpace[neighbor.Z].DungeonRooms[neighbor.Y].DungeonRooms[neighbor.X].NeighboringTightlyCoupledRooms.Add(FIntVector(x, y, z));
				}
			}
		}
	}
}
