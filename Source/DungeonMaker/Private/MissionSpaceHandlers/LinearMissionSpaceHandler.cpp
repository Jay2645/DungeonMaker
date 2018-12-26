

#include "LinearMissionSpaceHandler.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonFloor.h"
#include "DungeonFloorHelpers.h"

#define INVALID_LOCATION FIntVector(-1, -1, -1)

ULinearMissionSpaceHandler::ULinearMissionSpaceHandler()
{
	MaxDistanceBetweenRooms = 1;
}

void ULinearMissionSpaceHandler::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	FMissionSpaceHelper spaceHelper = FMissionSpaceHelper(Rng, StartLocation);
	TMap<UDungeonMakerNode*, FFloorRoom> processed;
	FFloorRoom parent = MakeFloorRoom(Head, StartLocation, Rng, SymbolCount);
	SetRoom(parent);

	TSet<FIntVector> availableRooms;
	for (int x = 0; x < DungeonSpaceGenerator->DungeonSpace.LowResXSize(); x++)
	{
		for (int y = 0; y < DungeonSpaceGenerator->DungeonSpace.LowResYSize(); y++)
		{
			for (int z = 0; z < DungeonSpaceGenerator->DungeonSpace.ZSize(); z++)
			{
				availableRooms.Add(FIntVector(x, y, z));
			}
		}
	}
	availableRooms.Remove(StartLocation);

	PairNodesToRooms(parent, processed, StartLocation, availableRooms, Rng, SymbolCount);
}

void ULinearMissionSpaceHandler::PairNodesToRooms(FFloorRoom Parent, TMap<UDungeonMakerNode*, FFloorRoom>& Processed, FIntVector StartLocation, TSet<FIntVector>& AvailableRooms, FRandomStream &Rng, int32 SymbolCount)
{
	TArray<UDungeonMakerNode*> toProcess;
	TArray<FFloorRoom> rooms;
	toProcess.Append(Parent.RoomNode->ChildrenNodes);
	while (toProcess.Num() > 0)
	{
		// Process depth-first
		UDungeonMissionNode* node = Cast<UDungeonMissionNode>(toProcess.Pop());
		if (node == NULL)
		{
			UE_LOG(LogSpaceGen, Warning, TEXT("Could not pair a null node!"));
			continue;
		}
		if (Processed.Contains(node))
		{
			// We've already made this child
			// Make sure that the parent has a connection to it
			FRoomPairing pairing = FRoomPairing(Processed[node].Location, Parent.Location);
			UpdateNeighbors(pairing, node->bTightlyCoupledToParent);
			continue;
		}

		UE_LOG(LogSpaceGen, Verbose, TEXT("Trying to place %s with %d available rooms."), *node->ToGraphSymbol().GetSymbolDescription(), AvailableRooms.Num());

		TArray<FIntVector> possibleLocations;
		for (int x = -((int32)MaxDistanceBetweenRooms); x <= ((int32)MaxDistanceBetweenRooms); x++)
		{
			for (int y = -((int32)MaxDistanceBetweenRooms); y <= ((int32)MaxDistanceBetweenRooms); y++)
			{
				if (x == 0 && y == 0)
				{
					// Parent location
					continue;
				}
				FIntVector location = StartLocation + FIntVector(x, y, 0);
				if (AvailableRooms.Contains(location))
				{
					possibleLocations.Add(location);
				}
			}
		}
		if (possibleLocations.Num() == 0)
		{
			UE_LOG(LogSpaceGen, Error, TEXT("Linear Mission Space Handler ran out of locations to process! Remaining node count: %d"), toProcess.Num() + 1);
			return;
		}

		FIntVector location = possibleLocations[Rng.RandRange(0, possibleLocations.Num() - 1)];
		AvailableRooms.Remove(location);
		FFloorRoom createdRoom = MakeFloorRoom(node, location, Rng, SymbolCount);
		rooms.Add(createdRoom);
		SetRoom(createdRoom);
		FRoomPairing pairing = FRoomPairing(createdRoom.Location, Parent.Location);
		UpdateNeighbors(pairing, node->bTightlyCoupledToParent);
		Processed.Add(node, createdRoom);
	}

	while (rooms.Num() > 0)
	{
		FFloorRoom room = rooms.Pop();
		PairNodesToRooms(room, Processed, room.Location, AvailableRooms, Rng, SymbolCount);
	}
}