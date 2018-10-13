// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonSpaceGenerator.h"
#include "MissionSpaceHandlers/NeighboringMissionSpaceHandler.h"

// Sets default values for this component's properties
UDungeonSpaceGenerator::UDungeonSpaceGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MaxGeneratedRooms = -1;
	MissionToSpaceHandlerClass = UNeighboringMissionSpaceHandler::StaticClass();
}

bool UDungeonSpaceGenerator::CreateDungeonSpace(UDungeonMissionNode* Head, int32 SymbolCount, FRandomStream& Rng)
{
	TotalSymbolCount = SymbolCount;

	// Create floors
	int32 floorSideSize = FMath::CeilToInt(FMath::Sqrt((float)DungeonSize / (float)RoomSize));
	int32 symbolsPerFloor = floorSideSize * floorSideSize;
	int32 floorCount = FMath::CeilToInt(SymbolCount / (float)symbolsPerFloor);

	// By default, all levels will have the same number of rooms
	// We can probably get fancy with this by making like spherical dungeons and such if wanted
	TArray<int32> dungeonLevelSizes;
	dungeonLevelSizes.SetNum(floorCount);
	for (int i = 0; i < dungeonLevelSizes.Num(); i++)
	{
		dungeonLevelSizes[i] = floorSideSize;
	}
	
	MissionSpaceHandler = NewObject<UDungeonMissionSpaceHandler>(GetOuter(), MissionToSpaceHandlerClass, TEXT("Mission Space Manager"));
	MissionSpaceHandler->RoomSize = RoomSize;
	MissionSpaceHandler->InitializeDungeonFloor(this, dungeonLevelSizes);
	// Map the mission to the space
	bool bMadeSpace = MissionSpaceHandler->CreateDungeonSpace(Head, FIntVector(0, 0, 0), TotalSymbolCount, Rng);

	if (!bMadeSpace)
	{
		MissionSpaceHandler->DestroyComponent();
		return false;
	}

	for (int i = 0; i < DungeonSpace.Num(); i++)
	{
		FString floorName = "Floor ";
		floorName.AppendInt(i);
		UDungeonFloorManager* floor = NewObject<UDungeonFloorManager>(GetOuter(), FName(*floorName));
		floor->InitializeFloorManager(this, i);
		Floors.Add(floor);
		floor->SpawnRooms(Rng, GlobalGroundScatter);
	}


	if (bDebugDungeon)
	{
		DrawDebugSpace();
	}
	else
	{
		for (ADungeonRoom* room : MissionRooms)
		{
			TSet<const UDungeonTile*> roomTiles = room->FindAllTiles();
			for (const UDungeonTile* tile : roomTiles)
			{
				if (!FloorComponentLookup.Contains(tile) && tile->GroundMesh.Num() > 0)
				{
					// Otherwise, create a new InstancedStaticMeshComponent
					FString componentName = tile->TileID.ToString() + " Floor";
					UE_LOG(LogSpaceGen, Log, TEXT("Generating new dungeon space actor %s."), *componentName);

					ASpaceMeshActor* floorMeshComponent = (ASpaceMeshActor*)GetWorld()->SpawnActor(ASpaceMeshActor::StaticClass());
					floorMeshComponent->Rename(*componentName);
					floorMeshComponent->SetStaticMesh(tile, tile->GroundMesh);
					FloorComponentLookup.Add(tile, floorMeshComponent);
				}
				if (!CeilingComponentLookup.Contains(tile) && tile->CeilingMesh.Num() > 0)
				{
					// Otherwise, create a new InstancedStaticMeshComponent
					FString componentName = tile->TileID.ToString() + " Ceiling";
					UE_LOG(LogSpaceGen, Log, TEXT("Generating new dungeon space actor %s."), *componentName);

					ASpaceMeshActor* ceilingMeshComponent = (ASpaceMeshActor*)GetWorld()->SpawnActor(ASpaceMeshActor::StaticClass());
					ceilingMeshComponent->Rename(*componentName);
					ceilingMeshComponent->SetStaticMesh(tile, tile->CeilingMesh);
					CeilingComponentLookup.Add(tile, ceilingMeshComponent);
				}
			}
		}
		UE_LOG(LogSpaceGen, Log, TEXT("Generated %d ceiling meshes and %d floor meshes for %d rooms."), CeilingComponentLookup.Num(), FloorComponentLookup.Num(), MissionRooms.Num());

#if WITH_EDITOR
		for (auto& kvp : CeilingComponentLookup)
		{
			FString folderPath = "Rooms/Meshes/Ceiling";
			kvp.Value->SetFolderPath(FName(*folderPath));
		}
		for (auto& kvp : FloorComponentLookup)
		{
			FString folderPath = "Rooms/Meshes/Floor";
			kvp.Value->SetFolderPath(FName(*folderPath));
		}
#endif

		for (UDungeonFloorManager* floor : Floors)
		{
			floor->SpawnRoomMeshes(FloorComponentLookup, CeilingComponentLookup, Rng);
		}
	}

	return true;
}

bool UDungeonSpaceGenerator::IsLocationValid(FIntVector FloorSpaceCoordinates)
{
	// If it's below 0, it's always invalid
	if (FloorSpaceCoordinates.X < 0 || FloorSpaceCoordinates.Y < 0 || FloorSpaceCoordinates.Z < 0)
	{
		return false;
	}
	// If it's above our total number of floors, it's also invalid
	if (FloorSpaceCoordinates.Z >= DungeonSpace.Num())
	{
		return false;
	}
	FLowResDungeonFloor floor = DungeonSpace.GetLowRes(FloorSpaceCoordinates.Z);
	return FloorSpaceCoordinates.X < floor.XSize() && FloorSpaceCoordinates.Y < floor.YSize();
}


TArray<FFloorRoom> UDungeonSpaceGenerator::GetAllNeighbors(FFloorRoom Room)
{
	TArray<FFloorRoom> neighbors;
	for (FIntVector neighbor : Room.NeighboringRooms)
	{
		if (!IsLocationValid(neighbor))
		{
			continue;
		}
		neighbors.Add(DungeonSpace.GetLowRes(neighbor.Z)[neighbor.Y][neighbor.X]);
	}
	for (FIntVector neighbor : Room.NeighboringTightlyCoupledRooms)
	{
		if (!IsLocationValid(neighbor))
		{
			continue;
		}
		neighbors.Add(DungeonSpace.GetLowRes(neighbor.Z)[neighbor.Y][neighbor.X]);
	}
	return neighbors;
}

void UDungeonSpaceGenerator::SetRoom(FFloorRoom Room)
{
	// Verify that the location is valid
	if (!IsLocationValid(Room.Location))
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Could not set room at (%d, %d, %d) because it was an invalid location!"), Room.Location.X, Room.Location.Y, Room.Location.Z);
		return;
	}
	DungeonSpace.Set(Room, DefaultFloorTile);
}

FFloorRoom UDungeonSpaceGenerator::GetRoomFromFloorCoordinates(const FIntVector& FloorSpaceCoordinates)
{
	if (!IsLocationValid(FloorSpaceCoordinates))
	{
		return FFloorRoom();
	}
	return DungeonSpace.GetLowRes(FloorSpaceCoordinates.Z)[FloorSpaceCoordinates.Y][FloorSpaceCoordinates.X];
}

FFloorRoom UDungeonSpaceGenerator::GetRoomFromTileSpace(const FIntVector& TileSpaceLocation)
{
	// Convert to floor space
	FIntVector floorSpaceLocation = ConvertToFloorSpace(TileSpaceLocation);
	return GetRoomFromFloorCoordinates(floorSpaceLocation);
}

FIntVector UDungeonSpaceGenerator::ConvertToFloorSpace(const FIntVector& TileSpaceVector) const
{
	// Floor space is found by dividing by how big each room is, then rounding down
	// As an example, if the room is 24 tiles long and the location is 22x22, it
	// would return the room located at 0, 0 (which stretches from (0,0) to (23, 23)).
	FIntVector floorSpaceVector = TileSpaceVector;
	floorSpaceVector.X = FMath::FloorToInt(TileSpaceVector.X / (float)RoomSize);
	floorSpaceVector.Y = FMath::FloorToInt(TileSpaceVector.Y / (float)RoomSize);
	// Z is left alone -- it's assumed that Z in tile space and floor space are the same
	return floorSpaceVector;
}

void UDungeonSpaceGenerator::DrawDebugSpace()
{
	for (int i = 0; i < DungeonSpace.Num(); i++)
	{
		DungeonSpace.GetHighRes(i).DrawDungeonFloor(GetOwner(), i);
	}
}