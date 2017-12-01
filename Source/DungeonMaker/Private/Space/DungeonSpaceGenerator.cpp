// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonSpaceGenerator.h"

// Sets default values for this component's properties
UDungeonSpaceGenerator::UDungeonSpaceGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MaxGeneratedRooms = -1;
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
	
	MissionSpaceHandler = NewObject<UDungeonMissionSpaceHandler>(GetOuter(), TEXT("Mission Space Manager"));
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

void UDungeonSpaceGenerator::DrawDebugSpace()
{
	MissionSpaceHandler->DrawDebugSpace();
	for (int i = 0; i < Floors.Num(); i++)
	{
		Floors[i]->DrawDebugSpace();
	}
}

FIntVector UDungeonSpaceGenerator::ConvertToFloorSpace(FIntVector TileSpaceLocation)
{
	return MissionSpaceHandler->ConvertToFloorSpace(TileSpaceLocation);
}

FFloorRoom UDungeonSpaceGenerator::GetRoomFromFloorCoordinates(FIntVector FloorSpaceLocation)
{
	return MissionSpaceHandler->GetRoomFromFloorCoordinates(FloorSpaceLocation);
}