// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonRoom.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonFloorManager.h"
#include <DrawDebugHelpers.h>
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY(LogSpaceGen);

// Sets default values for this component's properties
ADungeonRoom::ADungeonRoom()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	RoomTiles = FDungeonRoomMetadata();
	Symbol = NULL;
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Room Trigger"));
	RoomTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FCollisionResponseContainer collisonChannels;
	collisonChannels.SetAllChannels(ECR_Ignore);
	collisonChannels.SetResponse(ECollisionChannel::ECC_Pawn, ECR_Overlap);
	RoomTrigger->SetCollisionResponseToChannels(collisonChannels);
	RoomTrigger->SetupAttachment(DummyRoot);
	RoomTrigger->bGenerateOverlapEvents = true;
	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginTriggerOverlap);

	DebugRoomMaxExtents = FIntVector(16, 16, 1);
}


TArray<FIntVector> ADungeonRoom::GetTileLocations(const UDungeonTile* Tile)
{
	TArray<FIntVector> locations;
	for (int x = 0; x < RoomTiles.XSize(); x++)
	{
		for (int y = 0; y < RoomTiles.YSize(); y++)
		{
			if (RoomTiles[y][x] == Tile)
			{
				locations.Add(FIntVector(x, y, 0));
			}
		}
	}
	return locations;
}


void ADungeonRoom::BeginPlay()
{
	Super::BeginPlay();
}

void ADungeonRoom::OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerEnterRoom();
				for (ADungeonRoom* neighbor : AllNeighbors)
				{
					neighbor->OnPlayerEnterNeighborRoom();
				}
			}
		}
	}
}

void ADungeonRoom::InitializeRoom(UDungeonSpaceGenerator* SpaceGenerator, const UDungeonTile* DefaultFloorTile, 
	const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
	UDungeonFloorManager* FloorManager, int32 MaxXSize, int32 MaxYSize,
	int32 XPosition, int32 YPosition, int32 ZPosition, FFloorRoom Room,
	FRandomStream &Rng, bool bUseRandomDimensions, bool bIsDeterminedFromPoints)
{
	const int ROOM_BORDER_SIZE = 1;
	MaxXSize = FMath::Abs(MaxXSize);
	MaxYSize = FMath::Abs(MaxYSize);
	RoomLevel = (uint8)ZPosition;
	RoomMetadata = Room;

	DungeonSpace = SpaceGenerator;
	DungeonFloor = FloorManager;

	DebugDefaultWallTile = DefaultWallTile;
	DebugDefaultFloorTile = DefaultFloorTile;
	DebugDefaultEntranceTile = DefaultEntranceTile;

	DebugSeed = Rng.GetCurrentSeed();
	Symbol = (const UDungeonMissionSymbol*)Room.DungeonSymbol.Symbol;
	
	int32 xSize = MaxXSize;
	int32 ySize = MaxYSize;
	int32 xOffset = XPosition;
	int32 yOffset = YPosition;

	// Initialize the room with the default tiles
	RoomTiles = FDungeonRoomMetadata(xSize, ySize);
	for (int y = 0; y < ySize; y++)
	{
		for (int x = 0; x < xSize; x++)
		{
			if (x == 0 || y == 0 || x == xSize - 1 || y == ySize - 1)
			{
				Set(x, y, DefaultWallTile);
			}
			else
			{
				Set(x, y, DefaultFloorTile);
			}
		}
	}
	DebugRoomMaxExtents = FIntVector(xSize, ySize, 1);

	FVector worldPosition = FVector(
		xOffset * UDungeonTile::TILE_SIZE, 
		yOffset * UDungeonTile::TILE_SIZE, 
		ZPosition * UDungeonTile::TILE_SIZE);

	FVector halfExtents = FVector((xSize * 250.0f), (ySize * 250.0f), 500.0f);
	SetActorLocation(worldPosition);
	RoomTrigger->SetRelativeLocation(halfExtents - FVector(0.0f, 500.0f, 500.0f));
	RoomTrigger->SetBoxExtent(halfExtents);

	OnRoomInitialized();

	UE_LOG(LogSpaceGen, Log, TEXT("Initialized %s to dimensions %d x %d."), *GetName(), RoomTiles.XSize(), RoomTiles.YSize());
}

void ADungeonRoom::DoTileReplacement(FRandomStream &Rng)
{
	OnPreRoomTilesReplaced();
	DoTileReplacementPreprocessing();

	// Replace them based on our replacement rules
	TArray<FRoomReplacements> replacementPhases = RoomReplacementPhases;
	TMap<int32, uint8> replacementCounts;
	for (int i = 0; i < replacementPhases.Num(); i++)
	{
		TArray<URoomReplacementPattern*> replacementPatterns = replacementPhases[i].ReplacementPatterns;
		while (replacementPatterns.Num() > 0)
		{
			int32 rngIndex = Rng.RandRange(0, replacementPatterns.Num() - 1);
			if (!replacementCounts.Contains(rngIndex))
			{
				replacementCounts.Add(rngIndex, (uint8)0);
			}

			// See if we should actually select this pattern
			if (Rng.GetFraction() > replacementPatterns[rngIndex]->GetActualSelectionChance(this))
			{
				continue;
			}

			if (!replacementPatterns[rngIndex]->FindAndReplace(RoomTiles))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
			else
			{
				uint8 maxReplacements = replacementPatterns[rngIndex]->MaxReplacementCount;
				replacementCounts[rngIndex]++;
				if (maxReplacements > 0 && replacementCounts[rngIndex] >= maxReplacements)
				{
					// If we've exceeded our max replacement count, remove us from consideration
					replacementPatterns.RemoveAt(rngIndex);
				}
			}
		}
	}

	OnRoomTilesReplaced();
}

void ADungeonRoom::PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& FloorComponentLookup,
	TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& CeilingComponentLookup,
	FRandomStream& Rng)
{
	TMap<const UDungeonTile*, TArray<FIntVector>> tileLocations;
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			// Cache this tile location
			FIntVector location = FIntVector(x, y, 0);
			const UDungeonTile* tile = GetTile(x, y);
			if (!tileLocations.Contains(tile))
			{
				tileLocations.Add(tile, TArray<FIntVector>());
			}
			tileLocations[tile].Add(location);

			if (tile->GroundMesh.Mesh != NULL)
			{
				PlaceTiles(FloorComponentLookup, tile, tile->GroundMesh.Transform, location);
			}
			if (tile->CeilingMesh.Mesh != NULL)
			{
				PlaceTiles(CeilingComponentLookup, tile, tile->CeilingMesh.Transform, location);
			}
		}
	}

/*#if !UE_BUILD_SHIPPING
	FVector tileStringLocation = GetActorLocation();
	FVector tileStringOffset = FVector(XSize() * UDungeonTile::TILE_SIZE * 0.5f, YSize() * UDungeonTile::TILE_SIZE * 0.5f, 250.0f);
	DrawDebugString(GetWorld(), tileStringLocation + tileStringOffset, *GetName());
#endif*/

	DetermineGroundScatter(tileLocations, Rng);
}

void ADungeonRoom::DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations, FRandomStream& Rng)
{
	UE_LOG(LogSpaceGen, Log, TEXT("%s is analyzing %d different tiles to determine ground scatter."), *GetName(), TileLocations.Num());
	for (auto& kvp : TileLocations)
	{
		const UDungeonTile* tile = kvp.Key;
		if (!GroundScatter.Contains(tile))
		{
			UE_LOG(LogSpaceGen, Log, TEXT("%s had no ground scatter defined for %s."), *GetName(), *tile->TileID.ToString());
			continue;
		}
		FGroundScatterSet scatterSet = GroundScatter[tile];

		for (FGroundScatter scatter : scatterSet.GroundScatter)
		{
			if (scatter.ScatterObjects.Num() == 0)
			{
				UE_LOG(LogSpaceGen, Warning, TEXT("Null scatter object in room %s!"), *GetName());
				continue;
			}
			TArray<FIntVector> locations;
			locations.Append(kvp.Value);

			uint8 targetScatterCount = (uint8)Rng.RandRange(scatter.MinCount, scatter.MaxCount);
			uint8 currentScatterCount = scatter.SkipTiles;
			uint8 currentSkipCount = 0;
			TSubclassOf<AActor> selectedActor = NULL;
			FScatterTransform selectedObject;

			// Iterate over the locations array until we run out of locations
			// or we hit the maximum count
			while ((!scatter.bUseRandomCount || currentScatterCount < targetScatterCount) && locations.Num() > 0)
			{
				FIntVector localPosition;
				if (scatter.bUseRandomLocation)
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

				FIntVector location = localPosition + GetRoomTileSpacePosition();
				ETileDirection direction = GetTileDirection(location);

				// Choose the actual mesh we want to spawn
				if (selectedActor == NULL || !scatter.bAlwaysUseSameObjectForThisInstance || 
					selectedActor != NULL && !selectedObject.DirectionOffsets.Contains(direction))
				{
					TArray<FScatterTransform> scatterTransforms = TArray<FScatterTransform>(scatter.ScatterObjects);
					selectedActor = NULL;
					do
					{
						if (scatterTransforms.Num() == 0)
						{
							break;
						}
						int32 randomObjectIndex = Rng.RandRange(0, scatterTransforms.Num() - 1);
						selectedObject = scatter.ScatterObjects[randomObjectIndex];
						scatterTransforms.RemoveAt(randomObjectIndex);
						
						// Verify we actually have meshes to place here
						if (selectedObject.ScatterMeshes.Num() == 0)
						{
							UE_LOG(LogSpaceGen, Warning, TEXT("Ground scatter for room %s has an invalid mesh at tile %s."), *GetName(), *tile->TileID.ToString());
							continue;
						}

						// Verify this mesh can work at this location
						FIntVector maxOffset = localPosition + selectedObject.EdgeOffset;
						FIntVector minOffset = localPosition - selectedObject.EdgeOffset;
						if (minOffset.X < 0 || minOffset.Y < 0)
						{
							// Too close to the edge of the room
							continue;
						}
						if (maxOffset.X >= XSize() || maxOffset.Y >= YSize())
						{
							// Too close to the positive edge of the room
							continue;
						};

						if (!selectedObject.DirectionOffsets.Contains(direction))
						{
							// Direction not allowed
							continue;
						}

						int32 actorMeshIndex = Rng.RandRange(0, selectedObject.ScatterMeshes.Num() - 1);
						FScatterObject selectedMesh = selectedObject.ScatterMeshes[actorMeshIndex];
						if (Rng.GetFraction() <= selectedMesh.SelectionChance + (selectedMesh.DifficultyModifier * GetRoomDifficulty()))
						{
							selectedActor = selectedMesh.ScatterObject;
							if (selectedActor == NULL)
							{
								UE_LOG(LogSpaceGen, Warning, TEXT("Ground scatter for room %s has an null actor mesh at tile %s."), *GetName(), *tile->TileID.ToString());
								selectedObject.ScatterMeshes.RemoveAt(actorMeshIndex);
								if (selectedObject.ScatterMeshes.Num() == 0)
								{
									// Out of meshes; try another scatter object
									UE_LOG(LogSpaceGen, Error, TEXT("A ground scatter object ran out of scatter meshes for room %s, processing tile %s."), *GetName(), *tile->TileID.ToString());
									scatter.ScatterObjects.RemoveAt(actorMeshIndex);
								}
							}
						}
					} while (selectedActor == NULL);
				}
				if (selectedActor == NULL)
				{
					// Could not find relevant actor for whatever reason
					continue;
				}

				if (direction != ETileDirection::Center)
				{
					if (!scatter.bPlaceAdjacentToNextRooms)
					{
						bool bIsAdjacent = false;
						for (int x = -1; x <= 1; x++)
						{
							for (int y = -1; y <= 1; y++)
							{
								FFloorRoom nextRoom = DungeonFloor->GetRoomFromTileSpace(location + FIntVector(x, y, location.Z));
								if (nextRoom.SpawnedRoom == NULL || nextRoom.SpawnedRoom == this)
								{
									continue;
								}
								if (RoomMetadata.GetOutgoingRooms().Contains(nextRoom.Location))
								{
									bIsAdjacent = true;
									break;
								}
							}
							if (bIsAdjacent)
							{
								break;
							}
						}
						if (bIsAdjacent)
						{
							continue;
						}
					}

					if (!scatter.bPlaceAdjacentToPriorRooms)
					{
						bool bIsAdjacent = false;
						for (int x = -1; x <= 1; x++)
						{
							for (int y = -1; y <= 1; y++)
							{
								FFloorRoom nextRoom = DungeonFloor->GetRoomFromTileSpace(location + FIntVector(x, y, location.Z));
								if (nextRoom.SpawnedRoom == NULL || nextRoom.SpawnedRoom == this)
								{
									continue;
								}
								if (RoomMetadata.IncomingRoom == nextRoom.Location)
								{
									bIsAdjacent = true;
									break;
								}
							}
							if (bIsAdjacent)
							{
								break;
							}
						}
						if (bIsAdjacent)
						{
							continue;
						}
					}
				}

				// Last check -- should we skip this tile?
				if (currentSkipCount < scatter.SkipTiles)
				{
					// We need to skip this tile
					currentSkipCount++;
					continue;
				}
				else
				{
					// Reset skip count
					currentSkipCount = 0;
				}

				FTransform tileTransform = GetTileTransformFromTileSpace(location);
				if (!scatter.bConformToGrid)
				{
					FVector offset = FVector::ZeroVector;
					offset.X += Rng.FRandRange(0.0f, UDungeonTile::TILE_SIZE - (UDungeonTile::TILE_SIZE * 0.25f));
					offset.Y += Rng.FRandRange(0.0f, UDungeonTile::TILE_SIZE - (UDungeonTile::TILE_SIZE * 0.25f));
					tileTransform.AddToTranslation(offset);
				}
				FTransform scatterTransform = selectedObject.DirectionOffsets[direction];
				FVector tilePosition = tileTransform.GetLocation();
				FVector scatterPosition = scatterTransform.GetLocation();
				FRotator scatterRotation = FRotator(scatterTransform.GetRotation());
				FVector objectPosition = tilePosition + scatterPosition;
				FRotator objectRotation = FRotator(tileTransform.GetRotation());
				objectRotation.Add(scatterRotation.Pitch, scatterRotation.Yaw, scatterRotation.Roll);
				if (scatter.bUseRandomLocation)
				{
					int32 randomRotation = Rng.RandRange(0, 3);
					float rotationAmount = randomRotation * 90.0f;
					objectRotation.Add(0.0f, rotationAmount, 0.0f);
				}

				FTransform objectTransform = FTransform(objectRotation, objectPosition, scatterTransform.GetScale3D());
				AActor* scatterActor = GetWorld()->SpawnActorAbsolute(selectedActor, objectTransform);
#if WITH_EDITOR
				FString folderPath = "Rooms/Scatter Actors/";
				folderPath.Append(selectedActor->GetName());
				scatterActor->SetFolderPath(FName(*folderPath));
#endif
				currentScatterCount++;
			}
		}
	}
}

FTransform ADungeonRoom::GetTileTransform(const FIntVector& LocalLocation) const
{
	FIntVector worldLocation = GetRoomTileSpacePosition() + LocalLocation;
	return GetTileTransformFromTileSpace(worldLocation);
}


FTransform ADungeonRoom::GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const
{
	FVector location = FVector(WorldLocation.X * 500.0f, WorldLocation.Y * 500.0f, WorldLocation.Z * 500.0f);
	FRotator rotation = GetActorRotation();
	FVector scale = GetActorScale();

	ETileDirection direction = GetTileDirection(WorldLocation);
	switch (direction)
	{
	case ETileDirection::Center:
		location.X -= 250.0f;
		location.Y -= 250.0f;
		break;
	case ETileDirection::North:
		// Pass
		break;
	case ETileDirection::South:
		location.Y -= 500.0f;
		rotation.Yaw += 180.0f;
		break;
	case ETileDirection::East:
		rotation.Yaw += 90.0f;
		break;
	case ETileDirection::West:
		location.X += 500.0f;
		rotation.Yaw += 270.0f;
		break;
	case ETileDirection::Northeast:
		rotation.Yaw += 45.0f;
		break;
	case ETileDirection::Northwest:
		location.X += 500.0f;
		rotation.Yaw += 315.0f;
		break;
	case ETileDirection::Southeast:
		location.Y -= 500.0f;
		rotation.Yaw += 135.0f;
		break;
	case ETileDirection::Southwest:
		location.X += 500.0f;
		location.Y -= 500.0f;
		rotation.Yaw += 225.0f;
		break;
	default:
		checkNoEntry();
		break;
	}
	return FTransform(rotation, location, scale);
}

TSet<const UDungeonTile*> ADungeonRoom::FindAllTiles()
{
	return RoomTiles.FindAllTiles();
}

void ADungeonRoom::Set(int32 X, int32 Y, const UDungeonTile* Tile)
{
	if (RoomTiles[Y][X] == Tile)
	{
		// Already set
		return;
	}
	RoomTiles.Set(X, Y, Tile);
}

const UDungeonTile* ADungeonRoom::GetTile(int32 X, int32 Y) const
{
	if (!RoomTiles.DungeonRows.IsValidIndex(Y) || !RoomTiles.DungeonRows[Y].DungeonTiles.IsValidIndex(X))
	{
		return NULL;
	}
	return RoomTiles.DungeonRows[Y].DungeonTiles[X];
}

int32 ADungeonRoom::XSize() const
{
	return RoomTiles.XSize();
}

int32 ADungeonRoom::YSize() const
{
	return RoomTiles.YSize();
}


int32 ADungeonRoom::ZSize() const
{
	// TODO: Support multiple levels
	return 1;
}

FString ADungeonRoom::ToString() const
{
	return RoomTiles.ToString();
}

void ADungeonRoom::DrawDebugRoom()
{
	if (XSize() == 0 || YSize() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("%s had no room defined (both X and Y sizes were set to 0)!"), *Symbol->Description.ToString());
	}

	float halfX = XSize() / 2.0f;
	float halfY = YSize() / 2.0f;

	FIntVector position = GetRoomTileSpacePosition();

	FColor randomColor = RoomTiles.DrawRoom(this, position);

	// Draw lines connecting to our neighbors
	int32 xPosition = position.X;
	int32 yPosition = position.Y;
	int32 zPosition = position.Z;
	float midX = (xPosition + halfX) * UDungeonTile::TILE_SIZE;
	float midY = (yPosition + halfY) * UDungeonTile::TILE_SIZE;
	FVector startingLocation = FVector(midX, midY, zPosition);

	for (FIntVector neighborLocation : RoomMetadata.NeighboringRooms)
	{
		FFloorRoom room = DungeonSpace->GetRoomFromFloorCoordinates(neighborLocation);
		if (room.SpawnedRoom == NULL)
		{
			continue;
		}
		ADungeonRoom* neighbor = room.SpawnedRoom;
		float neighborHalfX = neighbor->XSize() / 2.0f;
		float neighborHalfY = neighbor->YSize() / 2.0f;


		FIntVector neighborPosition = neighbor->GetRoomTileSpacePosition();
		int32 neighborXPosition = neighborPosition.X;
		int32 neighborYPosition = neighborPosition.Y;
		int32 neighborZPosition = neighborPosition.Z;

		float neighborMidX = (neighborXPosition + neighborHalfX) * UDungeonTile::TILE_SIZE;
		float neighborMidY = (neighborYPosition + neighborHalfY) * UDungeonTile::TILE_SIZE;
		FVector endingLocation = FVector(neighborMidX, neighborMidY, zPosition * UDungeonTile::TILE_SIZE);
		DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true, -1.0f, (uint8)'\000', 100.0f);
	}

	for (FIntVector neighborLocation : RoomMetadata.NeighboringTightlyCoupledRooms)
	{
		FFloorRoom room = DungeonSpace->GetRoomFromFloorCoordinates(neighborLocation);
		if (room.SpawnedRoom == NULL)
		{
			continue;
		}
		ADungeonRoom* neighbor = room.SpawnedRoom;
		float neighborHalfX = neighbor->XSize() / 2.0f;
		float neighborHalfY = neighbor->YSize() / 2.0f;


		FIntVector neighborPosition = neighbor->GetRoomTileSpacePosition();
		int32 neighborXPosition = neighborPosition.X;
		int32 neighborYPosition = neighborPosition.Y;
		int32 neighborZPosition = neighborPosition.Z;

		float neighborMidX = (neighborXPosition + neighborHalfX) * UDungeonTile::TILE_SIZE;
		float neighborMidY = (neighborYPosition + neighborHalfY) * UDungeonTile::TILE_SIZE;
		FVector endingLocation = FVector(neighborMidX, neighborMidY, zPosition * UDungeonTile::TILE_SIZE);
		DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true, -1.0f, (uint8)'\000', 100.0f);
		DrawDebugLine(GetWorld(), startingLocation + FVector(0.0f, 0.0f, 25.0f), endingLocation + FVector(0.0f, 0.0f, 25.0f), randomColor, true, -1.0f, (uint8)'\000', 100.0f);
	}

	if (Symbol != NULL)
	{
		DrawDebugString(GetWorld(), FVector(midX, midY, (zPosition * UDungeonTile::TILE_SIZE) + 200.0f), GetName());
	}
}

bool ADungeonRoom::IsChangedAtRuntime() const
{
	if (Symbol == NULL)
	{
		return true;
	}
	return Symbol->bChangedAtRuntime;
}

ETileDirection ADungeonRoom::GetTileDirection(FIntVector Location) const
{
	FIntVector tileSpacePosition = GetRoomTileSpacePosition();

	// Top-left is northwest corner
	// Bottom-right is southeast corner
	bool bIsOnLeft = Location.X == tileSpacePosition.X;
	bool bIsOnRight = Location.X == tileSpacePosition.X + XSize() - 1;
	bool bIsOnTop = Location.Y == tileSpacePosition.Y;
	bool bIsOnBottom = Location.Y == tileSpacePosition.Y + YSize() - 1;
	
	if (bIsOnLeft && bIsOnTop)
	{
		// Top-Left corner
		return ETileDirection::Northwest;
	}
	else if (bIsOnRight && bIsOnTop)
	{
		// Top-right corner
		return ETileDirection::Northeast;
	}
	else if (bIsOnLeft && bIsOnBottom)
	{
		// Bottom-left corner
		return ETileDirection::Southwest;
	}
	else if (bIsOnRight && bIsOnBottom)
	{
		// Bottom-right corner
		return ETileDirection::Southeast;
	}
	else if (bIsOnTop)
	{
		// Top edge
		return ETileDirection::North;
	}
	else if (bIsOnBottom)
	{
		// Bottom edge
		return ETileDirection::South;
	}
	else if (bIsOnLeft)
	{
		// Left edge
		return ETileDirection::West;
	}
	else if (bIsOnRight)
	{
		// Right edge
		return ETileDirection::East;
	}
	else
	{
		// Not on edge at all
		return ETileDirection::Center;
	}
}


FIntVector ADungeonRoom::GetRoomTileSpacePosition() const
{
	FIntVector tileSpacePosition;
	FVector position = GetActorLocation();
	tileSpacePosition.X = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	tileSpacePosition.Y = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	tileSpacePosition.Z = FMath::RoundToInt(position.Z / UDungeonTile::TILE_SIZE);
	return tileSpacePosition;
}

void ADungeonRoom::SetTileGridCoordinates(FIntVector CurrentLocation, const UDungeonTile* Tile)
{
	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	Set(CurrentLocation.X - xPosition, CurrentLocation.Y - yPosition, Tile);
}

float ADungeonRoom::GetRoomDifficulty() const
{
	return RoomMetadata.Difficulty;
}

void ADungeonRoom::TryToPlaceEntrances(const UDungeonTile* EntranceTile, FRandomStream& Rng)
{
	TSet<FIntVector> neighbors = RoomMetadata.NeighboringRooms;
	for (FIntVector neighbor : neighbors)
	{
		AddNeighborEntrances(neighbor, Rng, EntranceTile);
	}
	// Now process any tightly-coupled neighbors
	neighbors = RoomMetadata.NeighboringTightlyCoupledRooms;
	for (FIntVector neighbor : neighbors)
	{
		ADungeonRoom* roomNeighbor = AddNeighborEntrances(neighbor, Rng, EntranceTile);
		if (roomNeighbor != NULL)
		{
			TightlyCoupledNeighbors.Add(roomNeighbor);
			roomNeighbor->TightlyCoupledNeighbors.Add(this);
		}
	}
}

void ADungeonRoom::DoTileReplacementPreprocessing()
{
	/* Empty */
}

ADungeonRoom* ADungeonRoom::AddNeighborEntrances(const FIntVector& Neighbor, FRandomStream& Rng, 
	const UDungeonTile* EntranceTile)
{
	// We handle spawning the entrance to the room above or to the right of us
	// The other room will spawn any other entrances
	ADungeonRoom* roomNeighbor = NULL;
	FIntVector ourLocation = FIntVector::ZeroValue;
	FIntVector neighborLocation = FIntVector::ZeroValue;
	if (Neighbor.X > RoomMetadata.Location.X)
	{
		int entranceLocation = Rng.RandRange(1, YSize() - 2);
		ourLocation.X = XSize() - 1;
		ourLocation.Y = entranceLocation;
		
		roomNeighbor = DungeonSpace->GetRoomFromFloorCoordinates(Neighbor).SpawnedRoom;
		neighborLocation.X = 0;
		neighborLocation.Y = entranceLocation;
	}
	else if (Neighbor.Y > RoomMetadata.Location.Y)
	{
		int entranceLocation = Rng.RandRange(1, XSize() - 2);
		ourLocation.X = entranceLocation;
		ourLocation.Y = YSize() - 1;

		roomNeighbor = DungeonSpace->GetRoomFromFloorCoordinates(Neighbor).SpawnedRoom;
		neighborLocation.X = entranceLocation;
		neighborLocation.Y = 0;
	}

	if (roomNeighbor != NULL)
	{
		Set(ourLocation.X, ourLocation.Y, EntranceTile);
		roomNeighbor->Set(neighborLocation.X, neighborLocation.Y, EntranceTile);
		AllNeighbors.Add(roomNeighbor);
		EntranceLocations.Add(ourLocation);
		roomNeighbor->AllNeighbors.Add(this);
		roomNeighbor->EntranceLocations.Add(neighborLocation);
	}
	return roomNeighbor;
}

void ADungeonRoom::PlaceTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& ComponentLookup, 
	const UDungeonTile* Tile, const FTransform& TileTransform, const FIntVector& Location)
{
	if (!ComponentLookup.Contains(Tile))
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Missing Instanced Static Mesh Component for tile %s!"), *Tile->TileID.ToString());
		return;
	}
	// Fetch Dungeon transform
	FTransform actorTransform = GetActorTransform();
	FVector actorPosition = actorTransform.GetLocation();
	// Add the local tile position, rotation, and the dungeon offset
	FVector tilePosition = TileTransform.GetLocation();
	FVector offset = FVector(Location.X * UDungeonTile::TILE_SIZE, Location.Y * UDungeonTile::TILE_SIZE, Location.Z * UDungeonTile::TILE_SIZE);
	FVector finalPosition = actorPosition + tilePosition + offset;
	// Add the rotations
	FRotator tileRotation = FRotator(TileTransform.GetRotation());
	FRotator finalRotation = FRotator(actorTransform.GetRotation());
	finalRotation.Add(tileRotation.Pitch, tileRotation.Yaw, tileRotation.Roll);

	// Create the tile
	FTransform objectTransform = FTransform(finalRotation, finalPosition, TileTransform.GetScale3D());
	ComponentLookup[Tile]->AddInstance(objectTransform);
}