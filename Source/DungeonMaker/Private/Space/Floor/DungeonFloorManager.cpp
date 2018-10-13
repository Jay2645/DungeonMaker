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

void UDungeonFloorManager::SpawnRooms(FRandomStream& Rng, const FGroundScatterPairing& GlobalGroundScatter)
{
	FLowResDungeonFloor& floor = DungeonSpaceGenerator->DungeonSpace.GetLowRes(DungeonLevel);
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].RoomClass == NULL)
			{
				// This room is empty
				continue;
			}
			floor[y][x].SpawnedRoom = CreateRoom(floor[y][x], Rng, GlobalGroundScatter);
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
	FLowResDungeonFloor floor = GetDungeonFloor();
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

void UDungeonFloorManager::SpawnRoomMeshes(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
	TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
	FRandomStream& Rng)
{
	FLowResDungeonFloor& floor = DungeonSpaceGenerator->DungeonSpace.GetLowRes(DungeonLevel);
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				// This room is empty
				continue;
			}
			floor[y][x].SpawnedRoom->PlaceRoomTiles(FloorComponentLookup, CeilingComponentLookup, Rng);
			floor[y][x].SpawnedRoom->OnRoomGenerationComplete();
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

TSet<FIntVector> UDungeonFloorManager::GetAllTilesOfType(ETileType Type)
{
	TSet<FIntVector> tileTypes;
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			FFloorRoom room = GetRoomFromTileSpace(FIntVector(x, y, DungeonLevel));
			if (room.SpawnedRoom != NULL)
			{
				tileTypes.Append(room.SpawnedRoom->GetAllTilesOfType(Type));
			}
		}
	}
	return tileTypes;
}

FFloorRoom UDungeonFloorManager::GetRoomFromTileSpace(const FIntVector& TileSpaceLocation)
{
	return DungeonSpaceGenerator->GetRoomFromTileSpace(TileSpaceLocation);
}

ADungeonRoom* UDungeonFloorManager::CreateRoom(const FFloorRoom& Room, FRandomStream& Rng, 
	const FGroundScatterPairing& GlobalGroundScatter)
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

	room->GetGroundScatter()->GroundScatter.CombinePairings(GlobalGroundScatter);

	FIntVector roomLocation = Room.Location * RoomSize;
	roomLocation.Z = Room.Location.Z;

	UE_LOG(LogSpaceGen, Log, TEXT("Spawned in room for %s."), *roomName);
	room->InitializeRoom(DungeonSpaceGenerator, DefaultFloorTile, DefaultWallTile, DefaultEntranceTile,
		this, RoomSize, RoomSize, roomLocation.X, roomLocation.Y, roomLocation.Z,
		Room, Rng);

	if (room->IsChangedAtRuntime())
	{
		UnresolvedHooks.Add(room);
	}
	DungeonSpaceGenerator->MissionRooms.Add(room);
	return room;
}

FLowResDungeonFloor UDungeonFloorManager::GetDungeonFloor() const
{
	return DungeonSpaceGenerator->DungeonSpace.GetLowRes(DungeonLevel);
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
			if (!replacementPatterns[rngIndex]->FindAndReplaceFloor(this, Rng))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}
}