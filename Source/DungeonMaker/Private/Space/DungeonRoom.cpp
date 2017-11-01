// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonRoom.h"
#include <DrawDebugHelpers.h>
#include "../../Public/Space/BSP/BSPLeaf.h"
#include "GameFramework/Character.h"


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

	MinimumRoomSize.WallSize = 4;
	MinimumRoomSize.CeilingHeight = 1;
	MaximumRoomSize.WallSize = 16;
	MaximumRoomSize.CeilingHeight = 1;
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

	if (!bIsStandaloneRoomForDebug)
	{
		// We only care about this if we're a standalone room and we're trying to be debugged
		return;
	}
	FRandomStream rng(DebugSeed);
	FIntVector position = GetRoomTileSpacePosition();
	int32 xPosition = position.X;
	int32 yPosition = position.Y;
	int32 zPosition = position.Z;

	// Initialize this room
	InitializeRoom(DebugDefaultTile, RoomDifficulty, DebugRoomMaxExtents.X, DebugRoomMaxExtents.Y,
		xPosition, yPosition, zPosition, Symbol, rng, false);
	// Create hallways
	TSet<ADungeonRoom*> newRooms;// = MakeHallways(rng, DebugDefaultTile, DebugHallwaySymbol);
	FDungeonFloor newFloor;

	// Add us to the list of hallways
	// This ensures we all get processed together
	newRooms.Add(this);
	TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> componentLookup;
	for (ADungeonRoom* room : newRooms)
	{
		// Replace the tile symbols
		// This doesn't place meshes, but states which mesh goes where
		room->DoTileReplacement(newFloor, rng);

		// Find our final tile set
		TSet<const UDungeonTile*> roomTiles = room->FindAllTiles();
		for (const UDungeonTile* tile : roomTiles)
		{
			if (componentLookup.Contains(tile) || tile->TileMesh == NULL)
			{
				continue;
			}
			// Otherwise, create a new InstancedStaticMeshComponent
			UHierarchicalInstancedStaticMeshComponent* tileMesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, tile->TileID);
			tileMesh->RegisterComponent();
			tileMesh->SetStaticMesh(tile->TileMesh);
			componentLookup.Add(tile, tileMesh);
		}
	}

	// Done with pre-processing the tiles; time to place the actual meshes!
	if (!bDrawDebugTiles)
	{
		for (ADungeonRoom* room : newRooms)
		{
			room->PlaceRoomTiles(componentLookup, rng, newFloor);
		}
	}
	else
	{
		for (ADungeonRoom* room : newRooms)
		{
			room->DrawDebugRoom();
		}
	}

	for (ADungeonRoom* room : newRooms)
	{
		room->OnRoomGenerationComplete();
	}
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
				for (ADungeonRoom* neighbor : MissionNeighbors)
				{
					neighbor->OnPlayerEnterNeighborRoom();
				}
			}
		}
	}
}

bool ADungeonRoom::PathIsClear(FIntVector StartLocation, FIntVector EndLocation, int32 SweepWidth, FDungeonFloor& DungeonFloor)
{
	int32 xSize;
	int32 ySize;
	if (StartLocation.X == EndLocation.X)
	{
		xSize = FMath::Abs(EndLocation.Y - StartLocation.Y);
		ySize = SweepWidth;
	}
	else if (StartLocation.Y == EndLocation.Y)
	{
		xSize = SweepWidth;
		ySize = FMath::Abs(EndLocation.X - StartLocation.X);

	}
	else
	{
		checkNoEntry();
		xSize = 0;
		ySize = 0;
	}

	FDungeonRoomMetadata room = DungeonFloor.ToRoom();
	UE_LOG(LogDungeonGen, Log, TEXT("Dungeon Size: %d x %d; layout: %s"), room.XSize(), room.YSize(), *room.ToString());

	for (int y = StartLocation.Y; y < StartLocation.Y + ySize; y++)
	{
		for (int x = StartLocation.X; x < StartLocation.X + xSize; x++)
		{
			const UDungeonTile* tile = DungeonFloor.GetTileAt(FIntVector(x, y, 0));
			if (tile != NULL)
			{
				return false;
			}
		}
	}
	return true;
}

void ADungeonRoom::InitializeRoomFromPoints(
	const UDungeonTile* DefaultRoomTile, const UDungeonMissionSymbol* RoomSymbol,
	FIntVector StartLocation, FIntVector EndLocation, int32 Width, bool bIsJoinedToHallway)
{
	FRandomStream rng;
	int32 width;
	int32 height;
	FIntVector startingLocation = StartLocation;
	if (StartLocation.X == EndLocation.X)
	{
		width = Width;
		height = EndLocation.Y - StartLocation.Y;

		if (height < 0)
		{
			startingLocation = EndLocation;
		}

		height = FMath::Abs(height);
		if (bIsJoinedToHallway)
		{
			height += Width;
		}
	}
	else if (StartLocation.Y == EndLocation.Y)
	{
		width = EndLocation.X - StartLocation.X;
		height = Width;

		if (width < 0)
		{
			startingLocation = EndLocation;
		}

		width = FMath::Abs(width);
	}
	else
	{
		checkNoEntry();
		return;
	}

	InitializeRoom(DefaultRoomTile, RoomDifficulty, width, height,
		startingLocation.X, startingLocation.Y, 0,
		RoomSymbol, rng, false, true);

}

void ADungeonRoom::InitializeRoom(const UDungeonTile* DefaultRoomTile,
	float Difficulty, int32 MaxXSize, int32 MaxYSize,
	int32 XPosition, int32 YPosition, int32 ZPosition,
	const UDungeonMissionSymbol* RoomSymbol, FRandomStream &Rng,
	bool bUseRandomDimensions, bool bIsDeterminedFromPoints)
{
	const int ROOM_BORDER_SIZE = 1;
	MaxXSize = FMath::Abs(MaxXSize);
	MaxYSize = FMath::Abs(MaxYSize);
	RoomLevel = (uint8)ZPosition;

	DebugDefaultTile = DefaultRoomTile;
	DebugSeed = Rng.GetCurrentSeed();
	Symbol = RoomSymbol;
	RoomDifficulty = Difficulty;

	int32 xSize;
	int32 ySize;
	if (bIsDeterminedFromPoints)
	{
		xSize = MaxXSize;
		ySize = MaxYSize;
	}
	else
	{
		check(MaxXSize > ROOM_BORDER_SIZE * 2);
		check(MaxYSize > ROOM_BORDER_SIZE * 2);

		xSize = FMath::Min(MaxXSize - (ROOM_BORDER_SIZE * 2), MaximumRoomSize.WallSize);
		ySize = FMath::Min(MaxYSize - (ROOM_BORDER_SIZE * 2), MaximumRoomSize.WallSize);
	}

	int32 xOffset;
	int32 yOffset;
	if (bUseRandomDimensions)
	{
		xSize = Rng.RandRange(MinimumRoomSize.WallSize, xSize);
		ySize = Rng.RandRange(MinimumRoomSize.WallSize, ySize);
		// X Offset can be anywhere from our current X position to the start of the room
		// That way we have enough space to place the room
		xOffset = Rng.RandRange(XPosition + ROOM_BORDER_SIZE, MaxXSize - xSize - ROOM_BORDER_SIZE);
		yOffset = Rng.RandRange(YPosition + ROOM_BORDER_SIZE, MaxYSize - ySize - ROOM_BORDER_SIZE);
	}
	else
	{
		xOffset = XPosition;
		yOffset = YPosition;
	}

	if (XPosition == 0 && YPosition == 0)
	{
		// First room; place the room under the spawn point
		xOffset = 0;
		yOffset = 0;
	}


	// Initialize the room with the default tiles
	RoomTiles = FDungeonRoomMetadata(xSize, ySize);
	for (int y = 0; y < ySize; y++)
	{
		for (int x = 0; x < xSize; x++)
		{
			Set(x, y, DefaultRoomTile);
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
}

void ADungeonRoom::DoTileReplacement(FDungeonFloor& DungeonFloor, FRandomStream &Rng)
{
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

	UpdateDungeonFloor(DungeonFloor);

	OnRoomTilesReplaced();
}

void ADungeonRoom::UpdateDungeonFloor(FDungeonFloor& DungeonFloor)
{
	FIntVector position = GetRoomTileSpacePosition();

	for (int y = 0; y < YSize(); y++)
	{
		for (int x = 0; x < XSize(); x++)
		{
			const UDungeonTile* tile = GetTile(x, y);
			FIntVector currentLocation = FIntVector(x + position.X, y + position.Y, 0);
			if (DungeonFloor.TileIsWall(currentLocation))
			{
				ADungeonRoom* otherRoom = DungeonFloor.GetRoom(currentLocation);
				if (otherRoom != NULL)
				{
					otherRoom->SetTileGridCoordinates(currentLocation, tile);
				}
				DungeonFloor.PlaceNewTile(currentLocation, this, tile);
			}
			else
			{
				Set(x, y, DungeonFloor.GetTileAt(currentLocation));
			}
		}
	}
}

TSet<ADungeonRoom*> ADungeonRoom::MakeHallways(FRandomStream& Rng, const UDungeonTile* DefaultTile, 
	const UDungeonMissionSymbol* HallwaySymbol, FDungeonFloor& DungeonFloor)
{
	TSet<ADungeonRoom*> newHallways;
	TSet<ADungeonRoom*> allNeighbors = MissionNeighbors;
	for (ADungeonRoom* neighbor : allNeighbors)
	{
		newHallways.Append(ConnectRooms(this, neighbor, Rng, HallwaySymbol, DefaultTile, DungeonFloor));
	}
	return newHallways;
}

void ADungeonRoom::PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& ComponentLookup,
	FRandomStream& Rng, FDungeonFloor& DungeonFloor)
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

			if (tile->TileMesh == NULL)
			{
				continue;
			}
			if (!ComponentLookup.Contains(tile))
			{
				UHierarchicalInstancedStaticMeshComponent* tileMesh = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, tile->TileID);
				tileMesh->RegisterComponent();
				tileMesh->SetStaticMesh(tile->TileMesh);
				ComponentLookup.Add(tile, tileMesh);
			}
			FTransform tileTfm = GetActorTransform();
			FVector offset = FVector(x * UDungeonTile::TILE_SIZE, y * UDungeonTile::TILE_SIZE, 0.0f);
			tileTfm.AddToTranslation(offset);
			UHierarchicalInstancedStaticMeshComponent* meshComponent = ComponentLookup[tile];
			meshComponent->AddInstance(tileTfm);
		}
	}

#if !UE_BUILD_SHIPPING
	FVector tileStringLocation = GetActorLocation();
	FVector tileStringOffset = FVector(XSize() * UDungeonTile::TILE_SIZE * 0.5f, YSize() * UDungeonTile::TILE_SIZE * 0.5f, 250.0f);
	DrawDebugString(GetWorld(), tileStringLocation + tileStringOffset, *GetName());
#endif

	DetermineGroundScatter(tileLocations, Rng, DungeonFloor);
}

void ADungeonRoom::DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations, FRandomStream& Rng, FDungeonFloor& DungeonFloor)
{
	UE_LOG(LogDungeonGen, Log, TEXT("%s is analyzing %d different tiles to determine ground scatter."), *GetName(), TileLocations.Num());
	for (auto& kvp : TileLocations)
	{
		const UDungeonTile* tile = kvp.Key;
		if (!GroundScatter.Contains(tile))
		{
			UE_LOG(LogDungeonGen, Log, TEXT("%s had no ground scatter defined for %s."), *GetName(), *tile->TileID.ToString());
			continue;
		}
		FGroundScatterSet scatterSet = GroundScatter[tile];

		for (FGroundScatter scatter : scatterSet.GroundScatter)
		{
			if (scatter.ScatterObjects.Num() == 0)
			{
				UE_LOG(LogDungeonGen, Warning, TEXT("Null scatter object in room %s!"), *GetName());
				continue;
			}
			TArray<FIntVector> locations;
			locations.Append(kvp.Value);

			uint8 targetScatterCount = (uint8)Rng.RandRange(scatter.MinCount, scatter.MaxCount);
			uint8 currentScatterCount = scatter.SkipTiles;
			uint8 currentSkipCount = 0;

			while ((!scatter.bUseRandomCount || currentScatterCount < targetScatterCount) && locations.Num() > 0)
			{
				FIntVector location;
				if (scatter.bUseRandomLocation)
				{
					int32 locationIndex = Rng.RandRange(0, locations.Num() - 1);
					location = locations[locationIndex];
					locations.RemoveAt(locationIndex);
				}
				else
				{
					location = locations[0];
					locations.RemoveAt(0);
				}
				FIntVector maxOffset = location + scatter.EdgeOffset;
				FIntVector minOffset = location - scatter.EdgeOffset;
				if (minOffset.X < 0 || minOffset.Y < 0)
				{
					// Too close to the edge of the room
					continue;
				}
				if (maxOffset.X >= XSize() || maxOffset.Y >= YSize())
				{
					// Too close to the positive edge of the room
					continue;
				}

				location += GetRoomTileSpacePosition();


				ETileDirection direction = GetTileDirection(location);

				if (!scatter.AllowedDirections.Contains(direction))
				{
					// Direction not allowed
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
								ADungeonRoom* next = DungeonFloor.GetRoom(location + FIntVector(x, y, location.Z));
								if (next == NULL || next == this)
								{
									continue;
								}
								if (!MissionNeighbors.Contains(next))
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
								ADungeonRoom* next = DungeonFloor.GetRoom(location + FIntVector(x, y, location.Z));
								if (next == NULL || next == this)
								{
									continue;
								}
								if (MissionNeighbors.Contains(next))
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
				FTransform scatterTransform = scatter.ObjectOffset;
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

				// Spawn the ground scatter object
				TSubclassOf<AActor> selectedActor = NULL;
				do
				{
					FScatterObject selectedObject = scatter.ScatterObjects[Rng.RandRange(0, scatter.ScatterObjects.Num() - 1)];
					if (Rng.GetFraction() <= selectedObject.SelectionChance)
					{
						selectedActor = selectedObject.ScatterObject;
					}
				} while (selectedActor == NULL);

				AActor* scatterActor = GetWorld()->SpawnActorAbsolute(selectedActor, objectTransform);
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
		UE_LOG(LogDungeonGen, Error, TEXT("%s had no room defined (both X and Y sizes were set to 0)!"), *Symbol->Description.ToString());
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

	for (ADungeonRoom* neighbor : MissionNeighbors)
	{
		if (neighbor->Symbol == NULL)
		{
			continue;
		}
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

FIntVector ADungeonRoom::FindClosestVertex(const FIntVector& Source1, const FIntVector& Source2, const FIntVector& Destination)
{
	float distance1 = FVector::DistSquaredXY((FVector)Source1, (FVector)Destination);
	float distance2 = FVector::DistSquaredXY((FVector)Source2, (FVector)Destination);
	if (distance1 < distance2)
	{
		return Source1;
	}
	else
	{
		return Source2;
	}
}

TSet<ADungeonRoom*> ADungeonRoom::ConnectRooms(ADungeonRoom* A, ADungeonRoom* B, FRandomStream& Rng,
	const UDungeonMissionSymbol* HallwaySymbol, const UDungeonTile* DefaultTile, FDungeonFloor& DungeonFloor)
{
	const int32 HALLWAY_WIDTH = 3;
	const int32 HALLWAY_EDGE_OFFSET = 1;
	if (A == NULL || B == NULL)
	{
		UE_LOG(LogDungeonGen, Fatal, TEXT("One of the supplied rooms to the hallway generator was null!"));
		return TSet<ADungeonRoom*>();
	}

	FString hallwayName = A->GetName() + " - " + B->GetName();

	TSet<ADungeonRoom*> hallways;
	// Convert room's world-space coordinates to grid coordinates
	FIntVector aMinLocation = A->GetRoomTileSpacePosition();
	FIntVector bMinLocation = B->GetRoomTileSpacePosition();
	FIntVector aMaxLocation = aMinLocation + FIntVector(A->XSize(), A->YSize(), A->ZSize());
	FIntVector bMaxLocation = bMinLocation + FIntVector(B->XSize(), B->YSize(), B->ZSize());
	
	FIntVector intersectionMinLocation = FIntVector(FMath::Max(aMinLocation.X, bMinLocation.X), FMath::Min(aMaxLocation.Y, bMaxLocation.Y), 0);
	FIntVector intersectionMaxLocation = FIntVector(FMath::Min(aMaxLocation.X, bMaxLocation.X), FMath::Max(aMinLocation.Y, bMinLocation.Y), 0);

	FVector midpoint = FVector((intersectionMinLocation.X + intersectionMaxLocation.X) / 2.0f, (intersectionMinLocation.Y + intersectionMaxLocation.Y) / 2.0f, (intersectionMinLocation.Z + intersectionMaxLocation.Z) / 2.0f);

	bool bOverlapX = bMaxLocation.X >= aMinLocation.X && bMaxLocation.X <= aMaxLocation.X ||
		aMaxLocation.X >= bMinLocation.X && aMaxLocation.X <= bMaxLocation.X;
	bool bOverlapY = bMaxLocation.Y >= aMinLocation.Y && bMaxLocation.Y <= aMaxLocation.Y ||
		aMaxLocation.Y >= bMinLocation.Y && aMaxLocation.Y <= bMaxLocation.Y;

	bool bMustBeLHallway = true;
	if (bOverlapX)
	{
		// We overlap on the X axis
		// Check to see if the overlap amount is at least our minimum hallway width
		// Otherwise, we wind up in a situation where the hallway doesn't fit properly
		int32 xDiff = FMath::Abs(intersectionMaxLocation.X - intersectionMinLocation.X);
		bMustBeLHallway = xDiff < HALLWAY_WIDTH;
	}
	if (bOverlapY)
	{
		// The same, but for the Y axis
		int32 yDiff = FMath::Abs(intersectionMaxLocation.Y - intersectionMinLocation.Y);
		bMustBeLHallway = yDiff < HALLWAY_WIDTH;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogDungeonGen, Log, TEXT("Connecting %s (%d, %d, %d) - (%d, %d, %d) to %s (%d, %d, %d) - (%d, %d, %d)."),
		*A->GetName(), aMinLocation.X, aMinLocation.Y, aMinLocation.Z, aMaxLocation.X, aMaxLocation.Y, aMaxLocation.Z,
		*B->GetName(), bMinLocation.X, bMinLocation.Y, bMinLocation.Z, bMaxLocation.X, bMaxLocation.Y, bMaxLocation.Z);
	UE_LOG(LogDungeonGen, Log, TEXT("Hallways intersect from (%d, %d, %d) (min) to (%d, %d, %d) (max). Midpoint is (%f, %f, %f)."),
		intersectionMinLocation.X, intersectionMinLocation.Y, intersectionMinLocation.Z, intersectionMaxLocation.X, intersectionMaxLocation.Y, intersectionMaxLocation.Z,
		midpoint.X, midpoint.Y, midpoint.Z);
	if (bMustBeLHallway)
	{
		UE_LOG(LogDungeonGen, Log, TEXT("There's not enough space between our rooms to generate a normal hallway; it will have a corner."));
	}
#endif

	// Check X overlap
	if (!bMustBeLHallway && bOverlapX)
	{
		int32 midpointX = FMath::RoundToInt(midpoint.X);
		UE_LOG(LogDungeonGen, Log, TEXT("Current:%d. Intersection min location: %d, Max location: %d"), midpointX, intersectionMinLocation.X, intersectionMaxLocation.X);
		while (midpointX + HALLWAY_WIDTH > intersectionMaxLocation.X && midpointX > intersectionMinLocation.X)
		{
			midpointX--;
		}
		UE_LOG(LogDungeonGen, Log, TEXT("New midpoint: %d"), midpointX);

		// Create vertical hallway along midpoint
		FIntVector hallwayStart = FIntVector(midpointX, intersectionMinLocation.Y, intersectionMinLocation.Z);
		FIntVector hallwayEnd = FIntVector(midpointX, intersectionMaxLocation.Y, intersectionMaxLocation.Z);

		ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		hallwayName += " X";
		hallway->Rename(*hallwayName);
		hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallwayStart, hallwayEnd, HALLWAY_WIDTH);
		hallways.Add(hallway);

		if (A->MissionNeighbors.Contains(B))
		{
			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
		else
		{
			B->MissionNeighbors.Remove(A);
			B->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(A);
		}
	}
	else if (!bMustBeLHallway && bOverlapY)
	{
		int32 midpointY = FMath::RoundToInt(midpoint.Y);
		while (midpointY + HALLWAY_WIDTH > intersectionMinLocation.Y && midpointY >= intersectionMaxLocation.Y)
		{
			midpointY--;
		}

		// Create horizontal hallway along midpoint
		FIntVector hallwayStart = FIntVector(intersectionMinLocation.X, midpointY, intersectionMinLocation.Z);
		FIntVector hallwayEnd = FIntVector(intersectionMaxLocation.X, midpointY, intersectionMaxLocation.Z);
		ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		hallwayName += " Y";
		hallway->Rename(*hallwayName);
		hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallwayEnd, hallwayStart, HALLWAY_WIDTH);
		hallways.Add(hallway);

		// Update the mission neighbors
		if (A->MissionNeighbors.Contains(B))
		{
			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
		else
		{
			B->MissionNeighbors.Remove(A);
			B->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(A);
		}
	}
	else
	{
		// No overlap at all, or not enough to count
		ADungeonRoom* hallway1 = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		ADungeonRoom* hallway2 = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));

		FString hallway1Name = hallwayName + " 1";
		FString hallway2Name = hallwayName + " 2";
		hallway1->Rename(*hallway1Name);
		hallway2->Rename(*hallway2Name);

		// Generate L-shaped hallway
		FIntVector aMidpoint = FIntVector(FMath::RoundToInt((aMinLocation.X + aMaxLocation.X) / 2.0f), FMath::RoundToInt((aMinLocation.Y + aMaxLocation.Y) / 2.0f), FMath::RoundToInt((aMinLocation.Z + aMaxLocation.Z) / 2.0f));
		FIntVector bMidpoint = FIntVector(FMath::RoundToInt((bMinLocation.X + bMaxLocation.X) / 2.0f), FMath::RoundToInt((bMinLocation.Y + bMaxLocation.Y) / 2.0f), FMath::RoundToInt((bMinLocation.Z + bMaxLocation.Z) / 2.0f));
		
		UE_LOG(LogDungeonGen, Log, TEXT("First midpoint: (%d, %d, %d); second midpoint: (%d, %d, %d)."),
			aMidpoint.X, aMidpoint.Y, aMidpoint.Z, bMidpoint.X, bMidpoint.Y, bMidpoint.Z);

		// Intersection 1: Uses the X midpoint of A and the Y midpoint of B
		FIntVector intersection1 = FIntVector(aMidpoint.X, bMidpoint.Y, midpoint.Z);
		// Move to the closest edge
		// For A this will be on the Y axis
		// For B this will be on the X axis
		FIntVector hallwayAStart1 = FindClosestVertex(FIntVector(aMidpoint.X, aMinLocation.Y, 0), 
			FIntVector(aMidpoint.X, aMaxLocation.Y, 0), 
			intersection1);
		FIntVector hallwayBStart1 = FindClosestVertex(FIntVector(bMinLocation.X, bMidpoint.Y, 0),
			FIntVector(bMaxLocation.X, bMidpoint.Y, 0),
			intersection1);
		UE_LOG(LogDungeonGen, Log, TEXT("Intersection 1: (%d, %d, %d) -> (%d, %d, %d) -> (%d, %d, %d)."),
			hallwayAStart1.X, hallwayAStart1.Y, hallwayAStart1.Z,
			intersection1.X, intersection1.Y, intersection1.Z, 
			hallwayBStart1.X, hallwayBStart1.Y, hallwayBStart1.Z);


		// Intersection 2: Uses the X midpoint of B and the Y midpoint of A
		FIntVector intersection2 = FIntVector(bMidpoint.X, aMidpoint.Y, midpoint.Z);
		// Move to the closest edge
		// For A this will be on the X axis
		// For B this will be on the Y axis
		FIntVector hallwayAStart2 = FindClosestVertex(FIntVector(aMinLocation.X, aMidpoint.Y, 0),
			FIntVector(aMaxLocation.X, aMidpoint.Y, 0),
			intersection2);
		FIntVector hallwayBStart2 = FindClosestVertex(FIntVector(bMidpoint.X, bMinLocation.Y, 0),
			FIntVector(bMidpoint.X, bMaxLocation.Y, 0),
			intersection2);
		UE_LOG(LogDungeonGen, Log, TEXT("Intersection 2: (%d, %d, %d) -> (%d, %d, %d) -> (%d, %d, %d)."),
			hallwayAStart2.X, hallwayAStart2.Y, hallwayAStart2.Z,
			intersection2.X, intersection2.Y, intersection2.Z,
			hallwayBStart2.X, hallwayBStart2.Y, hallwayBStart2.Z);

		bool bCanPlaceHallway1 = PathIsClear(intersection1, hallwayAStart1, HALLWAY_WIDTH, DungeonFloor) &&
			PathIsClear(intersection1, hallwayBStart1, HALLWAY_WIDTH, DungeonFloor);
		bool bCanPlaceHallway2 = PathIsClear(intersection2, hallwayAStart2, HALLWAY_WIDTH, DungeonFloor) &&
			PathIsClear(intersection2, hallwayBStart2, HALLWAY_WIDTH, DungeonFloor);

		if (bCanPlaceHallway1)
		{
			UE_LOG(LogDungeonGen, Log, TEXT("Intersection 1 can be placed."));
		}
		if (bCanPlaceHallway2)
		{
			UE_LOG(LogDungeonGen, Log, TEXT("Intersection 2 can be placed."));
		}

		if (bCanPlaceHallway1 && bCanPlaceHallway2 && Rng.GetFraction() > 0.5f || bCanPlaceHallway1 && !bCanPlaceHallway2)
		{
			hallway1->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				intersection1, hallwayAStart1, HALLWAY_WIDTH, true);
			hallway2->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				intersection1, hallwayBStart1, HALLWAY_WIDTH, true);
		}
		else if (bCanPlaceHallway2)
		{
			hallway1->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				intersection2, hallwayAStart2, HALLWAY_WIDTH, true);
			hallway2->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				intersection2, hallwayBStart2, HALLWAY_WIDTH, true);
		}
		else
		{
			UE_LOG(LogDungeonGen, Error, TEXT("No valid way to connect %s and %s!"), *A->GetName(), *B->GetName());
			
			// Place them anyway; they'll have to intersect
			if (Rng.GetFraction() > 0.5f)
			{
				hallway1->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
					intersection1, hallwayAStart1, HALLWAY_WIDTH, true);
				hallway2->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
					intersection1, hallwayBStart1, HALLWAY_WIDTH, true);
			}
			else
			{
				hallway1->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
					intersection2, hallwayAStart2, HALLWAY_WIDTH, true);
				hallway2->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
					intersection2, hallwayBStart2, HALLWAY_WIDTH, true);
			}
		}
		

		hallways.Add(hallway1);
		hallways.Add(hallway2);


		if (A->MissionNeighbors.Contains(B))
		{
			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway1);
			hallway1->MissionNeighbors.Add(hallway2);
			hallway2->MissionNeighbors.Add(B);
		}
		else
		{
			B->MissionNeighbors.Remove(A);
			B->MissionNeighbors.Add(hallway2);
			hallway2->MissionNeighbors.Add(hallway1);
			hallway1->MissionNeighbors.Add(A);
		}
	}
	return hallways;
}

void ADungeonRoom::SetTileGridCoordinates(FIntVector CurrentLocation, const UDungeonTile* Tile)
{

	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	Set(CurrentLocation.X - xPosition, CurrentLocation.Y - yPosition, Tile);
}
