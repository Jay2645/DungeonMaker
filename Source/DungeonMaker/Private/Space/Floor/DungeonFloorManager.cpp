// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloorManager.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonMissionSymbol.h"

void UDungeonFloorManager::InitializeFloorManager(UDungeonSpaceGenerator* SpaceGenerator, int32 Level)
{
	DungeonSpaceGenerator = SpaceGenerator;
	DungeonLevel = Level;

	RoomSize = DungeonSpaceGenerator->RoomSize;
	PreGenerationRoomReplacementPhases = DungeonSpaceGenerator->PreGenerationRoomReplacementPhases;
	PostGenerationRoomReplacementPhases = DungeonSpaceGenerator->PostGenerationRoomReplacementPhases;

	DefaultFloorTile = DungeonSpaceGenerator->DefaultFloorTile;
	DefaultWallTile = DungeonSpaceGenerator->DefaultWallTile;
	DefaultEntranceTile = DungeonSpaceGenerator->DefaultEntranceTile;

	UnresolvedHooks.Empty();
}

void UDungeonFloorManager::SpawnRooms(FRandomStream& Rng)
{
	FDungeonFloor& floor = DungeonSpaceGenerator->DungeonSpace[DungeonLevel];
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].RoomClass == NULL)
			{
				// This room is empty
				continue;
			}
			floor.DungeonRooms[y].DungeonRooms[x].SpawnedRoom = CreateRoom(floor[y][x], Rng);
		}
	}

	// Replace tiles
	// Handle entrances first
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				continue;
			}
			CreateEntrances(floor[y][x].SpawnedRoom, Rng);
		}
	}

	DoFloorWideTileReplacement(PreGenerationRoomReplacementPhases, Rng);
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				continue;
			}
			DoTileReplacement(floor[y][x].SpawnedRoom, Rng);
		}
	}
	DoFloorWideTileReplacement(PostGenerationRoomReplacementPhases, Rng);
}

void UDungeonFloorManager::DrawDebugSpace()
{
	FDungeonFloor floor = GetDungeonFloor();
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				continue;
			}
			floor[y][x].SpawnedRoom->DrawDebugRoom();
		}
	}
}

FFloorRoom UDungeonFloorManager::GetRoomFromTileSpace(FIntVector TileSpaceLocation)
{
	// Convert to floor space
	FIntVector floorSpaceLocation = DungeonSpaceGenerator->ConvertToFloorSpace(TileSpaceLocation);
	return DungeonSpaceGenerator->GetRoomFromFloorCoordinates(floorSpaceLocation);
}

const UDungeonTile* UDungeonFloorManager::GetTileFromTileSpace(FIntVector TileSpaceLocation)
{
	FIntVector floorSpaceLocation = DungeonSpaceGenerator->ConvertToFloorSpace(TileSpaceLocation);
	FFloorRoom room = DungeonSpaceGenerator->GetRoomFromFloorCoordinates(floorSpaceLocation);
	if (room.SpawnedRoom == NULL)
	{
		return NULL;
	}
	FIntVector localTileOffset = TileSpaceLocation - (floorSpaceLocation * RoomSize);
	return room.SpawnedRoom->GetTile(localTileOffset.X, localTileOffset.Y);
}

void UDungeonFloorManager::UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile)
{
	FIntVector floorSpaceLocation = DungeonSpaceGenerator->ConvertToFloorSpace(TileSpaceLocation);
	FFloorRoom room = DungeonSpaceGenerator->GetRoomFromFloorCoordinates(floorSpaceLocation);
	if (room.SpawnedRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Tile has not been placed yet at (%d, %d, %d)."), TileSpaceLocation.X, TileSpaceLocation.Y, TileSpaceLocation.Z);
		return;
	}
	FIntVector localTileOffset = TileSpaceLocation - floorSpaceLocation;
	room.SpawnedRoom->SetTileGridCoordinates(localTileOffset, NewTile);
}

void UDungeonFloorManager::SpawnRoomMeshes(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& ComponentLookup,
	FRandomStream& Rng)
{
	FDungeonFloor& floor = DungeonSpaceGenerator->DungeonSpace[DungeonLevel];
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				// This room is empty
				continue;
			}
			floor.DungeonRooms[y].DungeonRooms[x].SpawnedRoom->PlaceRoomTiles(ComponentLookup, Rng);
			floor.DungeonRooms[y].DungeonRooms[x].SpawnedRoom->OnRoomGenerationComplete();
		}
	}
}

int UDungeonFloorManager::XSize() const
{
	return GetDungeonFloor().XSize() * RoomSize;
}

int UDungeonFloorManager::YSize() const
{
	return GetDungeonFloor().YSize() * RoomSize;
}

ADungeonRoom* UDungeonFloorManager::CreateRoom(const FFloorRoom& Room, FRandomStream& Rng)
{
	FString roomName = Room.DungeonSymbol.GetSymbolDescription();
	roomName.Append(" (");
	roomName.AppendInt(Room.DungeonSymbol.SymbolID);
	roomName.AppendChar(')');

	ADungeonRoom* room = (ADungeonRoom*)GetWorld()->SpawnActor(((UDungeonMissionSymbol*)Room.DungeonSymbol.Symbol)->GetRoomType(Rng));
#if WITH_EDITOR
	room->SetFolderPath("Rooms");
#endif
	room->Rename(*roomName);

	FIntVector roomLocation = Room.Location * RoomSize;
	roomLocation.Z = Room.Location.Z;

	UE_LOG(LogSpaceGen, Log, TEXT("Spawned in room for %s."), *roomName);
	room->InitializeRoom(DungeonSpaceGenerator, DefaultFloorTile, DefaultWallTile, DefaultEntranceTile,
		this, RoomSize, RoomSize, roomLocation.X, roomLocation.Y, roomLocation.Z,
		Room, Rng);

	// Exploit the fact that our for loop runs from 0 upward to check to see if we can delete a wall
	// These rooms are guaranteed to have been spawned before us
	// We avoid the corners, however
	for (int x = roomLocation.X + 1; x < roomLocation.X + RoomSize - 1; x++)
	{
		const UDungeonTile* tile = GetTileFromTileSpace(FIntVector(x, roomLocation.Y - 1, roomLocation.Z));
		if (tile != NULL && tile->TileType == ETileType::Wall)
		{
			room->SetTileGridCoordinates(FIntVector(x, roomLocation.Y, roomLocation.Z), DefaultFloorTile);
		}
	}
	for (int y = roomLocation.Y + 1; y < roomLocation.Y + RoomSize - 1; y++)
	{
		const UDungeonTile* tile = GetTileFromTileSpace(FIntVector(roomLocation.X - 1, y, roomLocation.Z));
		if (tile != NULL && tile->TileType == ETileType::Wall)
		{
			room->SetTileGridCoordinates(FIntVector(roomLocation.X, y, roomLocation.Z), DefaultFloorTile);
		}
	}

	if (room->IsChangedAtRuntime())
	{
		UnresolvedHooks.Add(room);
	}
	DungeonSpaceGenerator->MissionRooms.Add(room);
	return room;
}

FDungeonFloor UDungeonFloorManager::GetDungeonFloor() const
{
	return DungeonSpaceGenerator->DungeonSpace[DungeonLevel];
}

void UDungeonFloorManager::CreateEntrances(ADungeonRoom* Room, FRandomStream& Rng)
{
	Room->TryToPlaceEntrances(DefaultEntranceTile, Rng);
}

void UDungeonFloorManager::DoTileReplacement(ADungeonRoom* Room, FRandomStream& Rng)
{
	Room->DoTileReplacement(Rng);
}

void UDungeonFloorManager::DoFloorWideTileReplacement(TArray<FRoomReplacements> ReplacementPhases, FRandomStream &Rng)
{
	// Replace them based on our replacement rules
	for (int i = 0; i < ReplacementPhases.Num(); i++)
	{
		TArray<URoomReplacementPattern*> replacementPatterns = ReplacementPhases[i].ReplacementPatterns;
		while (replacementPatterns.Num() > 0)
		{
			int32 rngIndex = Rng.RandRange(0, replacementPatterns.Num() - 1);
			if (!replacementPatterns[rngIndex]->FindAndReplaceFloor(this))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}
}