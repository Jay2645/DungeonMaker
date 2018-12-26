// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloorManager.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonMissionSymbol.h"
#include "Components/RoomMeshComponent.h"

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
	DefaultExitTile = DungeonSpaceGenerator->DefaultExitTile;

	UnresolvedHooks.Empty();
}

void UDungeonFloorManager::CreateRoomTiles(FRandomStream& Rng, const FGroundScatterPairing& GlobalGroundScatter)
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
	floor.DrawDungeonFloor(GetOwner(), DungeonLevel);
}

const UDungeonTile* UDungeonFloorManager::GetTileFromTileSpace(FIntVector TileSpaceLocation)
{
	return DungeonSpaceGenerator->GetTile(TileSpaceLocation);
}

void UDungeonFloorManager::UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile)
{
	DungeonSpaceGenerator->SetTile(TileSpaceLocation, NewTile);
}

void UDungeonFloorManager::SpawnRoomMeshes(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
	TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
	FRandomStream& Rng)
{
	FLowResDungeonFloor& floor = DungeonSpaceGenerator->DungeonSpace.GetLowRes(DungeonLevel);
#if WITH_EDITOR
	int32 roomCount = 0;
#endif
	for (int x = 0; x < floor.XSize(); x++)
	{
		for (int y = 0; y < floor.YSize(); y++)
		{
			if (floor[y][x].SpawnedRoom == NULL)
			{
				// This room is empty
				continue;
			}
			floor[y][x].SpawnedRoom->GetMeshComponent()->PlaceRoomTiles(FloorComponentLookup, CeilingComponentLookup, Rng);
			floor[y][x].SpawnedRoom->OnRoomGenerationComplete();
#if WITH_EDITOR
			roomCount++;
			UE_LOG(LogSpaceGen, Log, TEXT("Generated %s (room %d of %d)"), *floor[y][x].DungeonSymbol.GetSymbolDescription(), roomCount, DungeonSpaceGenerator->MissionRooms.Num());
#endif
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
	return DungeonSpaceGenerator->DungeonSpace.GetTileLocations(Type);
}

FFloorRoom UDungeonFloorManager::GetRoomFromTileSpace(const FIntVector& TileSpaceLocation)
{
	return DungeonSpaceGenerator->GetRoomFromTileSpace(TileSpaceLocation);
}

ADungeonRoom* UDungeonFloorManager::CreateRoom(const FFloorRoom& Room, FRandomStream& Rng, 
	const FGroundScatterPairing& GlobalGroundScatter)
{
	UDungeonMissionSymbol* symbol = Cast<UDungeonMissionSymbol>(Room.DungeonSymbol.Symbol);
	if (symbol == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Null symbol passed to create room! Room class: %s"), *Room.RoomClass->GetName());
		return NULL;
	}
	FString roomName = Room.DungeonSymbol.GetSymbolDescription();
	roomName.Append(" (");
	roomName.AppendInt(Room.DungeonSymbol.SymbolID);
	roomName.AppendChar(')');

#if WITH_EDITOR
	// If we're in a debug build, validate our data
	int roomTypeCount = symbol->RoomTypes.Num();
	if (roomTypeCount == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("%s had no room types defined!"), *roomName);
		return NULL;
	}
	for (int i = 0; i < symbol->RoomTypes.Num(); i++)
	{
		if (symbol->RoomTypes[i] == NULL)
		{
			UE_LOG(LogSpaceGen, Error, TEXT("%s had a null room type at index %d!"), *roomName, i);
		}
	}
#endif

	ADungeonRoom* room = (ADungeonRoom*)GetWorld()->SpawnActor(symbol->GetRoomType(Rng));
	if (room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Could not spawn %s!"), *roomName);
		return NULL;
	}
#if WITH_EDITOR
	room->SetFolderPath("Rooms");
#endif
	room->Rename(*roomName);

	room->GetGroundScatter()->GroundScatter.CombinePairings(GlobalGroundScatter);

	FIntVector roomLocation = Room.Location * RoomSize;
	roomLocation.Z = Room.Location.Z;

	UE_LOG(LogSpaceGen, Log, TEXT("Spawned in room for %s."), *roomName);

	room->InitializeRoom(DungeonSpaceGenerator, this, DefaultFloorTile, DefaultWallTile, DefaultEntranceTile, DefaultExitTile,
		FIntVector(RoomSize, RoomSize, 1), roomLocation, Room, Rng);

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
	Room->CreateEntranceToNeighbors(Rng);
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
			if (!replacementPatterns[rngIndex]->FindAndReplaceFloor(DungeonSpaceGenerator->DungeonSpace, DungeonLevel, Rng))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}
}