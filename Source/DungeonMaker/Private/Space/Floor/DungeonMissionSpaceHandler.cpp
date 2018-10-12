// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionSpaceHandler.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonMissionSymbol.h"

void UDungeonMissionSpaceHandler::InitializeDungeonFloor(UDungeonSpaceGenerator* SpaceGenerator, TArray<int32> LevelSizes)
{
	DungeonSpaceGenerator = SpaceGenerator;
	DungeonSpaceGenerator->DungeonSpace = FDungeonSpace(LevelSizes, RoomSize);
}

bool UDungeonMissionSpaceHandler::CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
	int32 SymbolCount, FRandomStream& Rng)
{
	const int32 MAX_ATTEMPTS = 20;
	int32 currentAttempts = 0;
	do
	{

		// Create space for each room on the DungeonFloor
		RoomCount = 0;
		GenerateDungeonRooms(Head, StartLocation, Rng, SymbolCount);
		ProcessRoomNeighbors();

		if (RoomCount != SymbolCount - 1)
		{
			UE_LOG(LogSpaceGen, Warning, TEXT("Room count didn't match symbol count! Rooms: %d, Symbols: %d"), RoomCount, SymbolCount);

			TArray<int32> levelSizes;
			for (int i = 0; i < DungeonSpaceGenerator->DungeonSpace.Num(); i++)
			{
				levelSizes.Add(DungeonSpaceGenerator->DungeonSpace[i].XSize());
			}
			InitializeDungeonFloor(DungeonSpaceGenerator, levelSizes);

			if (currentAttempts < MAX_ATTEMPTS)
			{
				UE_LOG(LogSpaceGen, Error, TEXT("Ran out of attempts to make room count match symbol count! Rooms: %d, Symbols: %d"), RoomCount, SymbolCount);
				return false;
			}
		}

		currentAttempts++;
	} while (RoomCount != SymbolCount - 1);

	return true;
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
	UDungeonMissionSymbol* symbol = (UDungeonMissionSymbol*)Node->NodeType;
	room.RoomClass = symbol->GetRoomType(Rng);
	room.Location = Location;
	room.Difficulty = Node->NodeID / (float)TotalSymbolCount;
	room.DungeonSymbol = Node->ToGraphSymbol();
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
	RoomCount++;
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
	if (Node == NULL)
	{
		// No rooms to pair
		UE_LOG(LogSpaceGen, Error, TEXT("Null node was provided to the Mission Space Handler!"));
		return true;
	}
	if (((UDungeonMissionSymbol*)Node->NodeType)->RoomTypes.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mission Space Handler tried handling %s, which had no room types defined!"), *Node->GetNodeTitle());
		return true;
	}
	
	for (int i = 0; i < Node->ParentNodes.Num(); i++)
	{
		if (!ProcessedNodes.Contains((UDungeonMissionNode*)Node->ParentNodes[i]))
		{
			// We haven't processed all our parent nodes yet!
			// We should be processed further on down the line, once our next parent node
			// finishes being processed.
			UE_LOG(LogSpaceGen, Log, TEXT("Deferring processing of %s because not all its parents have been processed yet."), *Node->GetSymbolDescription());
			return true;
		}
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

	UE_LOG(LogSpaceGen, Log, TEXT("Creating room for %s! Rooms available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableRooms.Num(), Node->ChildrenNodes.Num());
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
	for (UDungeonMakerNode* neighborNode : Node->ChildrenNodes)
	{
		if (neighborNode->bTightlyCoupledToParent)
		{
			// If we're tightly coupled to our parent, ensure we get added to a neighboring leaf
			UE_LOG(LogSpaceGen, Log, TEXT("Placing tightly-coupled room %s next to parent %s."), *neighborNode->GetNodeTitle(), *Node->GetNodeTitle());
			bool bSuccesfullyPairedChild = PairNodesToRooms((UDungeonMissionNode*)neighborNode, roomNeighborMap, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, true, TotalSymbolCount);
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
			nextToProcess.Add((UDungeonMissionNode*)neighborNode);
		}
	}

	if (((UDungeonMissionSymbol*)Node->NodeType)->bAllowedToHaveChildren)
	{
		AvailableRooms.Append(roomNeighborMap);
	}
	AllOpenRooms.Append(AvailableRooms);
	TArray<UDungeonMissionNode*> deferredNodes;

	// Now we process all non-tightly coupled nodes
	UE_LOG(LogSpaceGen, Log, TEXT("%s has %d children to process."), *Node->GetNodeTitle(), nextToProcess.Num());
	for (int i = 0; i < nextToProcess.Num(); i++)
	{
		// If we're not tightly coupled, ensure that we have all our required parents generated
		bool bDeferProcessing = false;
		for (int j = 0; j < nextToProcess[i]->ParentNodes.Num(); j++)
		{
			if (!ProcessedNodes.Contains((UDungeonMissionNode*)nextToProcess[i]->ParentNodes[j]))
			{
				bDeferProcessing = true;
				break;
			}
		}
		if (bDeferProcessing)
		{
			// Defer processing a bit
			deferredNodes.Add(nextToProcess[i]);
			continue;
		}

		UE_LOG(LogSpaceGen, Log, TEXT("Placing loosely-coupled room %s. Parent: %s."), *nextToProcess[i]->GetNodeTitle(), *Node->GetNodeTitle());
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

		bool bDeferProcessing = false;
		for (int j = 0; j < currentNode->ParentNodes.Num(); j++)
		{
			if (!ProcessedNodes.Contains((UDungeonMissionNode*)currentNode->ParentNodes[j]))
			{
				bDeferProcessing = true;
				break;
			}
		}
		if (!bDeferProcessing)
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

	// Make the actual room
	FFloorRoom room = MakeFloorRoom(Node, roomLocation.Key, Rng, TotalSymbolCount);
	SetRoom(room);
	// Don't bother setting neighbors if one of the neighbors would be invalid
	if (IsLocationValid(roomLocation.Key) && IsLocationValid(roomLocation.Value))
	{
		// Link the children
		if (bIsTightCoupling)
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z][roomLocation.Key.Y][roomLocation.Key.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z][roomLocation.Value.Y][roomLocation.Value.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Key);
		}
		else
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z][roomLocation.Key.Y][roomLocation.Key.X].NeighboringRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z][roomLocation.Value.Y][roomLocation.Value.X].NeighboringRooms.Add(roomLocation.Key);
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
	for (UDungeonMakerNode* neighborNode : Node->ChildrenNodes)
	{
		if (neighborNode->bTightlyCoupledToParent)
		{
			nodesToCheck.Add((UDungeonMissionNode*)neighborNode);
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
					DungeonSpaceGenerator->DungeonSpace[neighbor.Z][neighbor.Y][neighbor.X].NeighboringRooms.Add(FIntVector(x, y, z));
				}
				for (FIntVector neighbor : DungeonSpaceGenerator->DungeonSpace[z][y][x].NeighboringTightlyCoupledRooms)
				{
					DungeonSpaceGenerator->DungeonSpace[neighbor.Z][neighbor.Y][neighbor.X].NeighboringTightlyCoupledRooms.Add(FIntVector(x, y, z));
				}
			}
		}
	}
}
