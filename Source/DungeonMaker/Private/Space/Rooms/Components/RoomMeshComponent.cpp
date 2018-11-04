#include "RoomMeshComponent.h"


// Sets default values for this component's properties
URoomMeshComponent::URoomMeshComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	bDoGroundScatter = true;
	bSpawnInteractions = true;
	bPlaceTileMeshes = true;

	bHasPlacedMeshes = false;
}


void URoomMeshComponent::ClearTileBacklog()
{
	for (TPair<const UDungeonTile*, FTransform> kvp : TileBacklog)
	{
		CreateNewTileMesh(kvp.Key, kvp.Value);
	}
	TileBacklog.Empty();
}

FTransform URoomMeshComponent::CreateMeshTransform(const FTransform& MeshTransformOffset, const FIntVector &Location) const
{
	if (ParentRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mesh component did not have parent room defined!"));
		return FTransform();
	}
	// Fetch Dungeon transform
	FTransform actorTransform = ParentRoom->GetActorTransform();
	// Add the local tile position, rotation, and the dungeon offset
	FVector meshPosition = MeshTransformOffset.GetLocation();
	FVector offset = FVector(Location.X * UDungeonTile::TILE_SIZE, Location.Y * UDungeonTile::TILE_SIZE, Location.Z * UDungeonTile::TILE_SIZE);
	FVector finalPosition = meshPosition + offset;
	// Add the rotations
	FRotator meshRotation = FRotator(MeshTransformOffset.GetRotation());
	FRotator finalRotation = FRotator(actorTransform.GetRotation());
	finalRotation.Add(meshRotation.Pitch, meshRotation.Yaw, meshRotation.Roll);

	// Create the tile
	return FTransform(finalRotation, finalPosition, MeshTransformOffset.GetScale3D());
}

void URoomMeshComponent::PlaceTile(TMap<const UDungeonTile*, ASpaceMeshActor*>& ComponentLookup, const UDungeonTile* Tile, int32 MeshID, const FTransform& MeshTransformOffset, const FIntVector& Location)
{
	if (!ComponentLookup.Contains(Tile))
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Missing Instanced Static Mesh Component for tile %s!"), *Tile->TileID.ToString());
		return;
	}

	bHasPlacedMeshes = true;

	FTransform objectTransform = CreateMeshTransform(MeshTransformOffset, Location);
	ComponentLookup[Tile]->AddInstance(MeshID, objectTransform);
}

void URoomMeshComponent::CreateAllRoomTiles(TMap<const UDungeonTile*, TArray<FIntVector>>& TileLocations, TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup, TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup, FRandomStream& Rng)
{
	if (ParentRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mesh component did not have parent room defined!"));
		return;
	}

	FDungeonSpace dungeon = GetDungeon();
	TSet<FIntVector> entranceLocations = dungeon.GetTileLocations(ETileType::Entrance, ParentRoom);
	FIntVector roomSize = ParentRoom->GetRoomSize();

	// Place tiles
	for (auto& kvp : TileLocations)
	{
		const UDungeonTile* tile = kvp.Key;
		TArray<FIntVector> locations = kvp.Value;

		if (FloorComponentLookup.Contains(tile))
		{
			for (int i = 0; i < locations.Num(); i++)
			{
				int32 meshSelection;
				if (FloorTileMeshSelections.Contains(tile))
				{
					meshSelection = FloorTileMeshSelections[tile];
				}
				else
				{
					do
					{
						meshSelection = Rng.RandRange(0, tile->GroundMesh.Num() - 1);
					} while (tile->GroundMesh[meshSelection].SelectionChance < Rng.GetFraction());
				}
				PlaceTile(FloorComponentLookup, tile, meshSelection, tile->GroundMesh[meshSelection].Transform, locations[i]);
			}
		}
		if (CeilingComponentLookup.Contains(tile))
		{
			for (int i = 0; i < locations.Num(); i++)
			{
				int32 meshSelection;
				FIntVector currentLocation = locations[i];
				if (CeilingTileMeshSelections.Contains(tile))
				{
					meshSelection = CeilingTileMeshSelections[tile];
				}
				else
				{
					do
					{
						meshSelection = Rng.RandRange(0, tile->CeilingMesh.Num() - 1);
					} while (tile->CeilingMesh[meshSelection].SelectionChance < Rng.GetFraction());
				}
				FTransform meshTransform = tile->CeilingMesh[meshSelection].Transform;

				// On any tile that's not part of the entrance, set the ceiling up high
				// Entrances should have their ceiling match the door
				if (!entranceLocations.Contains(currentLocation))
				{
					meshTransform.AddToTranslation(FVector(0.0f, 0.0f, (roomSize.Z - 1) * 500.0f));
				}
				PlaceTile(CeilingComponentLookup, tile, meshSelection, meshTransform, currentLocation);
			}
		}
	}

	// Spawn walls up to the ceiling height
	if (roomSize.Z > 1)
	{
		TSet<FIntVector> walls = dungeon.GetTileLocations(ETileType::Wall, ParentRoom);
		walls.Append(dungeon.GetTileLocations(ETileType::Entrance, ParentRoom));
		if (walls.Num() > 0)
		{
			// If we find more than 1 wall, grab the first wall we can
			const UDungeonTile* wallTile = dungeon.GetTile(walls.Array()[0]);
			if (FloorComponentLookup.Contains(wallTile))
			{
				// Select the mesh associated with this wall
				int32 meshSelection = FloorTileMeshSelections[wallTile];
				TSet<FIntVector> wallLocations = dungeon.GetTileLocations(wallTile, ParentRoom);
				wallLocations.Append(entranceLocations.Array());
				// Make walls go up to the ceiling
				for (int i = 1; i < roomSize.Z; i++)
				{
					for (FIntVector wallLocation : wallLocations)
					{
						FIntVector location = wallLocation;
						location.Z = i;
						FTransform offset;
						PlaceTile(FloorComponentLookup, wallTile, FloorTileMeshSelections[wallTile], offset, location);
					}
				}
			}
		}
	}

	// Update anything that we tried to make too early (usually in Blueprint)
	ClearTileBacklog();
}

void URoomMeshComponent::SpawnInteractions(TMap<const UDungeonTile*, TArray<FIntVector>> &TileLocations, FRandomStream& Rng)
{
#if !UE_BUILD_SHIPPING
	if (bSpawnInteractions)
	{
#endif
		// Place tile interactions
		for (auto& kvp : InteractionOptions)
		{
			for (int i = 0; i < TileLocations[kvp.Key].Num(); i++)
			{
				SpawnInteraction(kvp.Key, kvp.Value, TileLocations[kvp.Key][i], Rng);
			}
		}
#if !UE_BUILD_SHIPPING
	}
#endif
}

void URoomMeshComponent::InitializeMeshComponent(ADungeonRoom* Room, UGroundScatterManager* GroundScatterManager, UDungeonSpaceGenerator* Space)
{
	ParentRoom = Room;
	GroundScatter = GroundScatterManager;
	DungeonSpace = Space;
}

void URoomMeshComponent::PlaceRoomTiles(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup, 
	TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup, FRandomStream& Rng)
{
	if (ParentRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mesh component did not have parent room defined!"));
		return;
	}

	FDungeonSpace& dungeon = ParentRoom->GetDungeon();
	FIntVector roomLocation = ParentRoom->GetRoomLocation();
	FIntVector roomSize = ParentRoom->GetRoomSize();

	TMap<const UDungeonTile*, TArray<FIntVector>> tileLocations;
	for (int x = roomLocation.X; x < roomLocation.X + roomSize.X; x++)
	{
		for (int y = roomLocation.Y; y < roomLocation.Y + roomSize.Y; y++)
		{
			// Cache this tile location
			FIntVector location = FIntVector(x, y, roomLocation.Z);
			const UDungeonTile* tile = dungeon.GetTile(location);
			if (tile == NULL)
			{
				continue;
			}
			if (!tileLocations.Contains(tile))
			{
				tileLocations.Add(tile, TArray<FIntVector>());
			}
			tileLocations[tile].Add(location);

			if (tile->bGroundMeshShouldAlwaysBeTheSame)
			{
				// Determine what we should spawn on this tile later
				if (tile->GroundMesh.Num() > 0 && !FloorTileMeshSelections.Contains(tile))
				{
					int32 randomIndex;
					do
					{
						randomIndex = Rng.RandRange(0, tile->GroundMesh.Num() - 1);
					} while (tile->GroundMesh[randomIndex].SelectionChance < Rng.GetFraction());

					FloorTileMeshSelections.Add(tile, randomIndex);
				}
			}

			if (tile->bCeilingMeshShouldAlwaysBeTheSame)
			{
				if (tile->CeilingMesh.Num() > 0 && !CeilingTileMeshSelections.Contains(tile))
				{
					int32 randomIndex;
					do
					{
						randomIndex = Rng.RandRange(0, tile->CeilingMesh.Num() - 1);
					} while (tile->CeilingMesh[randomIndex].SelectionChance < Rng.GetFraction());

					CeilingTileMeshSelections.Add(tile, randomIndex);
				}
			}

			if (tile->Interactions.Num() > 0 && !InteractionOptions.Contains(tile))
			{
				int32 randomIndex = Rng.RandRange(0, tile->Interactions.Num() - 1);
				InteractionOptions.Add(tile, tile->Interactions[randomIndex]);
			}
		}
	}

	CreateAllRoomTiles(tileLocations, FloorComponentLookup, CeilingComponentLookup, Rng);

	SpawnInteractions(tileLocations, Rng);

	// Determine ground scatter
	DetermineGroundScatter(tileLocations, Rng);
}

void URoomMeshComponent::DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations, FRandomStream& Rng)
{
#if !UE_BUILD_SHIPPING
	if (bDoGroundScatter)
	{
#endif
		if (GroundScatter == NULL)
		{
			UE_LOG(LogSpaceGen, Error, TEXT("No ground scatter defined!"));
			return;
		}
		GroundScatter->DetermineGroundScatter(TileLocations, Rng, ParentRoom);
#if !UE_BUILD_SHIPPING
	}
#endif
}

void URoomMeshComponent::CreateNewTileMesh(const UDungeonTile* Tile, const FTransform& Location)
{
	if (Tile == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Tile mesh lookup was passed a null tile at (%f, %f, %f)!"), Location.GetLocation().X, Location.GetLocation().Y, Location.GetLocation().Z);
		return;
	}
	else if (!bHasPlacedMeshes)
	{
		TileBacklog.Add(TPair<const UDungeonTile*, FTransform>(Tile, Location));
		return;
	}
	else if (DungeonSpace->FloorComponentLookup.Contains(Tile))
	{
		DungeonSpace->FloorComponentLookup[Tile]->AddInstance(FloorTileMeshSelections[Tile], Location);
	}
	else if (DungeonSpace->CeilingComponentLookup.Contains(Tile))
	{
		DungeonSpace->CeilingComponentLookup[Tile]->AddInstance(CeilingTileMeshSelections[Tile], Location);
	}
	else
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Neither floor nor ceiling lookup contained %s at (%f, %f, %f)!"), *Tile->GetName(), Location.GetLocation().X, Location.GetLocation().Y, Location.GetLocation().Z);
	}
}

AActor* URoomMeshComponent::SpawnInteraction(const UDungeonTile* Tile, FDungeonTileInteractionOptions TileInteractionOptions,
	const FIntVector& Location, FRandomStream& Rng)
{
	if (TileInteractionOptions.Options.Num() == 0)
	{
		// No interactions possible
		return NULL;
	}
	TSubclassOf<AActor> interactionActor = NULL;
	FDungeonTileInteraction interaction;
	TArray<FDungeonTileInteraction> allInteractions = TileInteractionOptions.Options;

	// Select an interaction actor
	do
	{
		int32 randomIndex = Rng.RandRange(0, TileInteractionOptions.Options.Num() - 1);
		interaction = TileInteractionOptions.Options[randomIndex];
		if (interaction.InteractionActor == NULL || interaction.SelectionChance == 0.000f)
		{
			// Remove the offending actor
			allInteractions.RemoveAt(randomIndex);
			continue;
		}

		if (interaction.SelectionChance < Rng.GetFraction())
		{
			// Selection chance too low, try again
			continue;
		}
		interactionActor = interaction.InteractionActor;
	} while (interactionActor == NULL && allInteractions.Num() > 0);

	if (interactionActor == NULL)
	{
		return NULL;
	}

	// Create transform
	FTransform transform = interaction.BaseTransform;
	ETileDirection direction = ParentRoom->GetTileComponent()->GetTileDirection(Location);
	if (TileInteractionOptions.DirectionOffsets.Contains(direction))
	{
		FTransform offset = TileInteractionOptions.DirectionOffsets[direction];
		transform.SetLocation(transform.GetLocation() + offset.GetLocation());
		transform.SetRotation(transform.GetRotation() + offset.GetRotation());
		transform.SetScale3D(transform.GetScale3D() * offset.GetScale3D());
	}

	// Spawn actor
	return GetWorld()->SpawnActorAbsolute(interactionActor, CreateMeshTransform(transform, Location));
}

FDungeonSpace& URoomMeshComponent::GetDungeon() const
{
	return ParentRoom->GetDungeon();
}