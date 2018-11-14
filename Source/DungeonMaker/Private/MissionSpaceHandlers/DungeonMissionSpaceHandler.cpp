// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionSpaceHandler.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonMissionSymbol.h"

#define INVALID_LOCATION FIntVector(-1, -1, -1)

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
	bool bMadeDungeonSuccessfully = false;
	do
	{
		currentAttempts++;

		// Create space for each room on the DungeonFloor
		RoomCount = 0;
		GenerateDungeonRooms(Head, StartLocation, Rng, SymbolCount);
		bMadeDungeonSuccessfully = RoomCount == SymbolCount;
		if (bMadeDungeonSuccessfully)
		{
			ProcessRoomNeighbors();
		}
		else
		{
			UE_LOG(LogSpaceGen, Warning, TEXT("Some symbols did not have rooms! Rooms: %d, Symbols: %d. Attempt number %d."), RoomCount, SymbolCount, currentAttempts);

			TArray<int32> levelSizes;
			for (int i = 0; i < DungeonSpaceGenerator->DungeonSpace.Num(); i++)
			{
				levelSizes.Add(DungeonSpaceGenerator->DungeonSpace.GetLowRes(i).XSize());
			}
			InitializeDungeonFloor(DungeonSpaceGenerator, levelSizes);
		}
	} while (!bMadeDungeonSuccessfully && currentAttempts < MAX_ATTEMPTS);

	if (!bMadeDungeonSuccessfully)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Aborting this dungeon after trying to make rooms %d times!"), currentAttempts);
	}
	return bMadeDungeonSuccessfully;
}

bool UDungeonMissionSpaceHandler::CheckInputIsValid(TMap<FIntVector, FIntVector> &AvailableRooms, bool bIsTightCoupling, UDungeonMissionNode* Node, FMissionSpaceHelper &SpaceHelper)
{
	if (AvailableRooms.Num() == 0 && bIsTightCoupling)
	{
		// Out of leaves to process
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is tightly coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}
	if (!SpaceHelper.HasOpenRooms() && !bIsTightCoupling)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is loosely coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}
	return true;
}

bool UDungeonMissionSpaceHandler::CheckCanSkipProcessing(UDungeonMissionNode* Node, FMissionSpaceHelper &SpaceHelper)
{
	if (Node == NULL)
	{
		// No rooms to pair
		UE_LOG(LogSpaceGen, Error, TEXT("Null node was provided to the Mission Space Handler!"));
		return true;
	}
	if (SpaceHelper.HasProcessed(Node))
	{
		// Already processed this node
		return true;
	}

	if (((UDungeonMissionSymbol*)Node->NodeType)->RoomTypes.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mission Space Handler tried handling %s, which had no room types defined!"), *Node->GetNodeTitle());
		return true;
	}

	for (int i = 0; i < Node->ParentNodes.Num(); i++)
	{
		if (!SpaceHelper.HasProcessed((UDungeonMissionNode*)Node->ParentNodes[i]))
		{
			// We haven't processed all our parent nodes yet!
			// We should be processed further on down the line, once our next parent node
			// finishes being processed.
			UE_LOG(LogSpaceGen, Log, TEXT("Deferring processing of %s because not all its parents have been processed yet."), *Node->GetSymbolDescription());
			return true;
		}
	}
	return false;
}

void UDungeonMissionSpaceHandler::DrawDebugSpace()
{
	for (int i = 0; i < DungeonSpaceGenerator->DungeonSpace.Num(); i++)
	{
		DungeonSpaceGenerator->DungeonSpace.GetLowRes(i).DrawDungeonFloor(GetOwner(), i);
	}
}

TSet<FIntVector> UDungeonMissionSpaceHandler::GetAvailableLocations(FIntVector Location, 
	TSet<FIntVector> IgnoredLocations /*= TSet<FIntVector>()*/)
{
	TSet<FIntVector> availableLocations;

	if (DungeonSpaceGenerator->IsLocationValid(Location) && DungeonSpaceGenerator->DungeonSpace.GetLowRes(Location.Z)[Location.Y][Location.X].DungeonSymbol.Symbol != NULL)
	{
		UDungeonMissionSymbol* symbol = (UDungeonMissionSymbol*)DungeonSpaceGenerator->DungeonSpace.GetLowRes(Location.Z)[Location.Y][Location.X].DungeonSymbol.Symbol;
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

				if (!DungeonSpaceGenerator->IsLocationValid(possibleLocation))
				{
					// Out of range
					continue;
				}
				if (DungeonSpaceGenerator->DungeonSpace.GetLowRes(possibleLocation.Z)[possibleLocation.Y][possibleLocation.X].RoomClass != NULL)
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
	room.MaxRoomSize = RoomSize;

	return room;
}

void UDungeonMissionSpaceHandler::SetRoom(FFloorRoom Room, bool bShouldIncrementRoomCount)
{
	DungeonSpaceGenerator->SetRoom(Room);
	if (bShouldIncrementRoomCount)
	{
		RoomCount++;
	}
}

void UDungeonMissionSpaceHandler::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	// Empty
}

void UDungeonMissionSpaceHandler::ProcessRoomNeighbors()
{
	for (int z = 0; z < DungeonSpaceGenerator->DungeonSpace.Num(); z++)
	{
		for (int y = 0; y < DungeonSpaceGenerator->DungeonSpace.GetLowRes(z).YSize(); y++)
		{
			for (int x = 0; x < DungeonSpaceGenerator->DungeonSpace.GetLowRes(z).XSize(); x++)
			{
				for (FIntVector neighbor : DungeonSpaceGenerator->DungeonSpace.GetLowRes(z)[y][x].NeighboringRooms)
				{
					DungeonSpaceGenerator->DungeonSpace.GetLowRes(neighbor.Z)[neighbor.Y][neighbor.X].NeighboringRooms.Add(FIntVector(x, y, z));
				}
				for (FIntVector neighbor : DungeonSpaceGenerator->DungeonSpace.GetLowRes(z)[y][x].NeighboringTightlyCoupledRooms)
				{
					DungeonSpaceGenerator->DungeonSpace.GetLowRes(neighbor.Z)[neighbor.Y][neighbor.X].NeighboringTightlyCoupledRooms.Add(FIntVector(x, y, z));
				}
			}
		}
	}
}

FRoomPairing UDungeonMissionSpaceHandler::GetOpenRoom(UDungeonMissionNode* Node,
	TMap<FIntVector, FIntVector>& AvailableRooms, FMissionSpaceHelper& SpaceHelper)
{
	TSet<UDungeonMissionNode*> nodesToCheck;
	// If this node has a tightly-coupled child, ensure that there's room to place the child as well
	for (UDungeonMakerNode* neighborNode : Node->ChildrenNodes)
	{
		if (neighborNode->bTightlyCoupledToParent)
		{
			nodesToCheck.Add((UDungeonMissionNode*)neighborNode);
		}
	}

	// Initialize our starting location to an invalid location
	FIntVector roomLocation = INVALID_LOCATION;
	FIntVector parentLocation = INVALID_LOCATION;
	do
	{
		if (AvailableRooms.Num() == 0)
		{
			// Out of rooms; return an invalid input
			UE_LOG(LogSpaceGen, Warning, TEXT("Ran out of rooms when trying to place %s"), *Node->ToString(0, false));
			return FRoomPairing();
		}
		int32 leafIndex = SpaceHelper.Rng.RandRange(0, AvailableRooms.Num() - 1);
		TArray<FIntVector> allAvailableRooms;
		AvailableRooms.GetKeys(allAvailableRooms);
		roomLocation = allAvailableRooms[leafIndex];
		parentLocation = AvailableRooms[roomLocation];
		AvailableRooms.Remove(roomLocation);

		if (SpaceHelper.HasProcessed(roomLocation))
		{
			// Already processed this leaf
			roomLocation = INVALID_LOCATION;
			parentLocation = INVALID_LOCATION;
			continue;
		}

		TSet<FIntVector> neighbors = GetAvailableLocations(roomLocation, SpaceHelper.GetProcessedRooms());

		if (neighbors.Num() < nodesToCheck.Num())
		{
			// This leaf wouldn't have enough neighbors to attach all our tightly-coupled nodes
			roomLocation = INVALID_LOCATION;
			parentLocation = INVALID_LOCATION;
			continue;
		}

		if (parentLocation != INVALID_LOCATION)
		{
			const FFloorRoom& room = DungeonSpaceGenerator->GetRoomFromFloorCoordinates(parentLocation);
			if (Node->NodeType != NULL && room.DungeonSymbol.Symbol != NULL)
			{
				UDungeonMissionSymbol* symbol = Cast<UDungeonMissionSymbol>(Node->NodeType);
				if (symbol != NULL)
				{
					if (symbol->SymbolSkipChances.Contains(room.DungeonSymbol.Symbol))
					{
						float skipChance = symbol->SymbolSkipChances[room.DungeonSymbol.Symbol];
						if (SpaceHelper.Rng.GetFraction() <= skipChance)
						{
							// Skip
							// This room is technically still valid, so we re-insert it into the map
							AvailableRooms.Add(roomLocation, parentLocation);
							roomLocation = INVALID_LOCATION;
							parentLocation = INVALID_LOCATION;
							UE_LOG(LogSpaceGen, Log, TEXT("Skipping room as it has the same symbol as our parent."));
							continue;
						}
					}
				}
				UE_LOG(LogSpaceGen, Log, TEXT("Placing %s next to %s."), *Node->GetSymbolDescription(), *room.DungeonSymbol.GetSymbolDescription());
			}
		}
	} while (roomLocation == INVALID_LOCATION);
	return FRoomPairing(roomLocation, parentLocation);
}

void UDungeonMissionSpaceHandler::UpdateNeighbors(const FRoomPairing& RoomPairing, bool bIsTightCoupling)
{
	FIntVector childRoom = RoomPairing.ChildRoom;
	FIntVector parentRoom = RoomPairing.ParentRoom;

	// Don't bother setting neighbors if one of the neighbors would be invalid
	if (DungeonSpaceGenerator->IsLocationValid(childRoom) && DungeonSpaceGenerator->IsLocationValid(parentRoom))
	{
		// Link the children
		if (bIsTightCoupling)
		{
			DungeonSpaceGenerator->DungeonSpace.GetLowRes(childRoom.Z)[childRoom.Y][childRoom.X].NeighboringTightlyCoupledRooms.Add(parentRoom);
			DungeonSpaceGenerator->DungeonSpace.GetLowRes(parentRoom.Z)[parentRoom.Y][parentRoom.X].NeighboringTightlyCoupledRooms.Add(childRoom);
		}
		else
		{
			DungeonSpaceGenerator->DungeonSpace.GetLowRes(childRoom.Z)[childRoom.Y][childRoom.X].NeighboringRooms.Add(parentRoom);
			DungeonSpaceGenerator->DungeonSpace.GetLowRes(parentRoom.Z)[parentRoom.Y][parentRoom.X].NeighboringRooms.Add(childRoom);
		}
	}
}