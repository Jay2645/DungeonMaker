#include "GroundScatterManager.h"
#include "DungeonRoom.h"


// Sets default values for this component's properties
UGroundScatterManager::UGroundScatterManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UGroundScatterManager::DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations,
	FRandomStream& Rng, ADungeonRoom* Room)
{
	UE_LOG(LogSpaceGen, Log, TEXT("%s is analyzing %d different tiles to determine ground scatter."), *Room->GetName(), TileLocations.Num());
	for (auto& kvp : TileLocations)
	{
		const UDungeonTile* tile = kvp.Key;
		if (!GroundScatter.Pairings.Contains(tile))
		{
			UE_LOG(LogSpaceGen, Log, TEXT("%s had no ground scatter defined for %s."), *Room->GetName(), *tile->TileID.ToString());
			continue;
		}
		FGroundScatterSet scatterSet = GroundScatter.Pairings[tile];
		TArray<FIntVector> tileLocations = kvp.Value;

		for (FGroundScatter scatter : scatterSet.GroundScatter)
		{
			ProcessScatterItem(scatter, tileLocations, Rng, tile, Room);

		}
	}
}

void UGroundScatterManager::ProcessScatterItem(FGroundScatter& Scatter, const TArray<FIntVector>& TileLocations,
	FRandomStream& Rng, const UDungeonTile* Tile, ADungeonRoom* Room)
{
	if (Scatter.ScatterObjects.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Null Scatter object in room %s!"), *Room->GetName());
		return;
	}
	TArray<FIntVector> locations;
	locations.Append(TileLocations);

	uint8 targetScatterCount = (uint8)Rng.RandRange(Scatter.MinCount, Scatter.MaxCount);
	uint8 currentScatterCount = Scatter.SkipTiles;
	uint8 currentSkipCount = 0;
	TSubclassOf<AActor> selectedActor = NULL;
	FScatterTransform selectedObject;

	// Iterate over the locations array until we run out of locations
	// or we hit the maximum count
	while ((!Scatter.bUseRandomCount || currentScatterCount < targetScatterCount) && locations.Num() > 0)
	{
		FIntVector localPosition;
		// Select the location we're going to work on, then make sure
		// that it won't be chosen again
		if (Scatter.bUseRandomLocation)
		{
			int32 locationIndex = Rng.RandRange(0, locations.Num() - 1);
			localPosition = locations[locationIndex];
			locations.RemoveAt(locationIndex);
		}
		else
		{
			localPosition = locations[0];
			locations.RemoveAt(0);
		}

		FIntVector location = localPosition + Room->GetRoomTileSpacePosition();
		ETileDirection direction = Room->GetTileDirection(location);

		// Choose the actual mesh we want to spawn
		if (selectedActor == NULL || !Scatter.bAlwaysUseSameObjectForThisInstance ||
			selectedActor != NULL && !selectedObject.DirectionOffsets.Contains(direction))
		{
			selectedActor = FindScatterActor(Scatter, Rng, selectedObject, Room, Tile, localPosition, direction);
		}
		if (selectedActor == NULL)
		{
			// Could not find relevant actor for whatever reason
			continue;
		}

		// Check adjacency
		if (!IsAdjacencyOkay(direction, Scatter, Room, location))
		{
			continue;
		}


		// Last check -- should we skip this Tile?
		if (currentSkipCount < Scatter.SkipTiles)
		{
			// We need to skip this Tile
			currentSkipCount++;
			continue;
		}
		else
		{
			// Reset skip count
			currentSkipCount = 0;
		}

		// Spawn the actor
		AActor* spawnedActor = CreateScatterObject(Room, location, Scatter, Rng, selectedObject, direction, selectedActor);
		if (spawnedActor != NULL)
		{
			currentScatterCount++;
		}
	}
}

AActor* UGroundScatterManager::CreateScatterObject(ADungeonRoom* Room, FIntVector location, FGroundScatter &Scatter,
	FRandomStream& Rng, FScatterTransform& SelectedObject, ETileDirection Direction,
	TSubclassOf<AActor> SelectedActor)
{
	FTransform tileTransform = Room->GetTileTransformFromTileSpace(location);
	if (!Scatter.bConformToGrid)
	{
		FVector offset = FVector::ZeroVector;
		offset.X += Rng.FRandRange(0.0f, UDungeonTile::TILE_SIZE - (UDungeonTile::TILE_SIZE * 0.25f));
		offset.Y += Rng.FRandRange(0.0f, UDungeonTile::TILE_SIZE - (UDungeonTile::TILE_SIZE * 0.25f));
		tileTransform.AddToTranslation(offset);
	}
	FTransform scatterTransform = SelectedObject.DirectionOffsets[Direction];
	FVector tilePosition = tileTransform.GetLocation();
	FVector scatterPosition = scatterTransform.GetLocation();
	FRotator scatterRotation = FRotator(scatterTransform.GetRotation());
	FVector objectPosition = tilePosition + scatterPosition;
	FRotator objectRotation = FRotator(tileTransform.GetRotation());
	objectRotation.Add(scatterRotation.Pitch, scatterRotation.Yaw, scatterRotation.Roll);
	if (Scatter.bUseRandomLocation)
	{
		int32 randomRotation = Rng.RandRange(0, 3);
		float rotationAmount = randomRotation * 90.0f;
		objectRotation.Add(0.0f, rotationAmount, 0.0f);
	}

	FTransform objectTransform = FTransform(objectRotation, objectPosition, scatterTransform.GetScale3D());
	AActor* scatterActor = GetWorld()->SpawnActorAbsolute(SelectedActor, objectTransform);

#if WITH_EDITOR
	FString folderPath = "Rooms/Scatter Actors/";
	folderPath.Append(SelectedActor->GetName());
	scatterActor->SetFolderPath(FName(*folderPath));
#endif

	SpawnedGroundScatter.Add(scatterActor);
	return scatterActor;
}

bool UGroundScatterManager::IsAdjacencyOkay(ETileDirection Direction, FGroundScatter& Scatter, 
	ADungeonRoom* Room, FIntVector& Location)
{
	if (Direction != ETileDirection::Center)
	{
		if (!Scatter.bPlaceAdjacentToNextRooms)
		{
			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					FFloorRoom nextRoom = Room->DungeonFloor->GetRoomFromTileSpace(Location + FIntVector(x, y, Location.Z));
					if (nextRoom.SpawnedRoom == NULL || nextRoom.SpawnedRoom == Room)
					{
						continue;
					}
					if (Room->RoomMetadata.GetOutgoingRooms().Contains(nextRoom.Location))
					{
						return false;
					}
				}
			}
		}

		if (!Scatter.bPlaceAdjacentToPriorRooms)
		{
			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					FFloorRoom nextRoom = Room->DungeonFloor->GetRoomFromTileSpace(Location + FIntVector(x, y, Location.Z));
					if (nextRoom.SpawnedRoom == NULL || nextRoom.SpawnedRoom == Room)
					{
						continue;
					}
					if (Room->RoomMetadata.IncomingRoom == nextRoom.Location)
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

TSubclassOf<AActor> UGroundScatterManager::FindScatterActor(FGroundScatter& Scatter, FRandomStream& Rng, FScatterTransform& SelectedObject, ADungeonRoom* Room, const UDungeonTile* Tile, FIntVector LocalPosition, ETileDirection Direction)
{
	TArray<FScatterTransform> scatterTransforms = TArray<FScatterTransform>(Scatter.ScatterObjects);
	TSubclassOf<AActor> selectedActor = NULL;
	do
	{
		if (scatterTransforms.Num() == 0)
		{
			break;
		}
		int32 randomObjectIndex = Rng.RandRange(0, scatterTransforms.Num() - 1);
		SelectedObject = Scatter.ScatterObjects[randomObjectIndex];
		scatterTransforms.RemoveAt(randomObjectIndex);

		// Verify we actually have meshes to place here
		if (SelectedObject.ScatterMeshes.Num() == 0)
		{
			UE_LOG(LogSpaceGen, Warning, TEXT("Ground Scatter for room %s has an invalid mesh at Tile %s."), *Room->GetName(), *Tile->TileID.ToString());
			continue;
		}

		// Verify this mesh can work at this location
		FIntVector maxOffset = LocalPosition + SelectedObject.EdgeOffset;
		FIntVector minOffset = LocalPosition - SelectedObject.EdgeOffset;
		if (minOffset.X < 0 || minOffset.Y < 0)
		{
			// Too close to the edge of the room
			continue;
		}
		if (maxOffset.X >= Room->XSize() || maxOffset.Y >= Room->YSize())
		{
			// Too close to the positive edge of the room
			continue;
		};

		if (!SelectedObject.DirectionOffsets.Contains(Direction))
		{
			// Direction not allowed
			continue;
		}

		int32 actorMeshIndex = Rng.RandRange(0, SelectedObject.ScatterMeshes.Num() - 1);
		FScatterObject selectedMesh = SelectedObject.ScatterMeshes[actorMeshIndex];
		if (Rng.GetFraction() <= selectedMesh.SelectionChance + (selectedMesh.DifficultyModifier * Room->GetRoomDifficulty()))
		{
			selectedActor = selectedMesh.ScatterObject;
			if (selectedActor == NULL)
			{
				UE_LOG(LogSpaceGen, Warning, TEXT("Ground Scatter for room %s has an null actor mesh at Tile %s."), *Room->GetName(), *Tile->TileID.ToString());
				SelectedObject.ScatterMeshes.RemoveAt(actorMeshIndex);
				if (SelectedObject.ScatterMeshes.Num() == 0)
				{
					// Out of meshes; try another Scatter object
					UE_LOG(LogSpaceGen, Error, TEXT("A ground Scatter object ran out of Scatter meshes for room %s, processing Tile %s."), *Room->GetName(), *Tile->TileID.ToString());
					Scatter.ScatterObjects.RemoveAt(actorMeshIndex);
				}
			}
		}
	} while (selectedActor == NULL);
	return selectedActor;
}