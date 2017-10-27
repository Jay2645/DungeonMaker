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
	InitializeRoom(DebugDefaultTile, DebugRoomMaxExtents.X, DebugRoomMaxExtents.Y,
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

void ADungeonRoom::InitializeRoomFromPoints(
	const UDungeonTile* DefaultRoomTile, const UDungeonMissionSymbol* RoomSymbol,
	FIntVector StartLocation, FIntVector EndLocation, int32 Width)
{
	FRandomStream rng;
	if (StartLocation.X == EndLocation.X)
	{
		InitializeRoom(DefaultRoomTile, Width, FMath::Abs(EndLocation.Y - StartLocation.Y),
			StartLocation.X, StartLocation.Y, StartLocation.Z,
			RoomSymbol, rng, false, true);
	}
	else if (StartLocation.Y == EndLocation.Y)
	{
		InitializeRoom(DefaultRoomTile, FMath::Abs(EndLocation.X - StartLocation.X), Width,
			StartLocation.X, StartLocation.Y, StartLocation.Z,
			RoomSymbol, rng, false, true);
	}
	else
	{
		checkNoEntry();
	}
}

void ADungeonRoom::InitializeRoom(const UDungeonTile* DefaultRoomTile,
	int32 MaxXSize, int32 MaxYSize,
	int32 XPosition, int32 YPosition, int32 ZPosition,
	const UDungeonMissionSymbol* RoomSymbol, FRandomStream &Rng,
	bool bUseRandomDimensions, bool bIsDeterminedFromPoints)
{
	const int ROOM_BORDER_SIZE = 1;
	MaxXSize = FMath::Abs(MaxXSize);
	MaxYSize = FMath::Abs(MaxYSize);

	DebugDefaultTile = DefaultRoomTile;
	DebugSeed = Rng.GetCurrentSeed();
	Symbol = RoomSymbol;
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
		UE_LOG(LogDungeonGen, Error, TEXT("%s had no room defined!"), *Symbol->Description.ToString());
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

TSet<ADungeonRoom*> ADungeonRoom::ConnectRooms(ADungeonRoom* A, ADungeonRoom* B, FRandomStream& Rng,
	const UDungeonMissionSymbol* HallwaySymbol, const UDungeonTile* DefaultTile, FDungeonFloor& DungeonFloor)
{
	const int32 HALLWAY_WIDTH = 3;
	const int32 HALLWAY_EDGE_OFFSET = 1;

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

	if (bMaxLocation.X >= aMinLocation.X && bMaxLocation.X <= aMaxLocation.X || aMaxLocation.X >= bMinLocation.X && aMaxLocation.X <= bMaxLocation.X)
	{
		// Create vertical hallway along midpoint
		if (midpoint.X < aMinLocation.X + HALLWAY_EDGE_OFFSET || midpoint.X < bMinLocation.X + HALLWAY_EDGE_OFFSET)
		{
			midpoint.X += HALLWAY_EDGE_OFFSET;
		}
		else if (midpoint.X > aMaxLocation.X - HALLWAY_EDGE_OFFSET - HALLWAY_WIDTH || midpoint.X > bMaxLocation.X - HALLWAY_EDGE_OFFSET - HALLWAY_WIDTH)
		{
			midpoint.X -= HALLWAY_EDGE_OFFSET;
		}

		FIntVector hallwayStart = FIntVector(FMath::RoundToInt(midpoint.X), intersectionMinLocation.Y, intersectionMinLocation.Z);
		FIntVector hallwayEnd = FIntVector(FMath::RoundToInt(midpoint.X), intersectionMaxLocation.Y, intersectionMaxLocation.Z);

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
	else if (bMaxLocation.Y >= aMinLocation.Y && bMaxLocation.Y <= aMaxLocation.Y || aMaxLocation.Y >= bMinLocation.Y && aMaxLocation.Y <= bMaxLocation.Y)
	{
		// Create horizontal hallway along midpoint
		if (midpoint.Y < aMinLocation.Y + HALLWAY_EDGE_OFFSET || midpoint.Y < bMinLocation.Y + HALLWAY_EDGE_OFFSET)
		{
			midpoint.Y += HALLWAY_EDGE_OFFSET;
		}
		else if (midpoint.Y > aMaxLocation.Y - HALLWAY_EDGE_OFFSET - HALLWAY_WIDTH || midpoint.Y > bMaxLocation.Y - HALLWAY_EDGE_OFFSET - HALLWAY_WIDTH)
		{
			midpoint.Y -= HALLWAY_EDGE_OFFSET;
		}
		FIntVector hallwayStart = FIntVector(intersectionMinLocation.X, FMath::RoundToInt(midpoint.Y), intersectionMinLocation.Z);
		FIntVector hallwayEnd = FIntVector(intersectionMaxLocation.X, FMath::RoundToInt(midpoint.Y), intersectionMaxLocation.Z);
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
		// No overlap at all
		// Generate L-shaped hallway
		FIntVector aMidpoint = FIntVector(FMath::RoundToInt((aMinLocation.X + aMaxLocation.X) / 2.0f), FMath::RoundToInt((aMinLocation.Y + aMinLocation.Y) / 2.0f), FMath::RoundToInt((aMinLocation.Z + aMaxLocation.Z) / 2.0f));
		FIntVector bMidpoint = FIntVector(FMath::RoundToInt((bMinLocation.X + bMaxLocation.X) / 2.0f), FMath::RoundToInt((bMinLocation.Y + bMinLocation.Y) / 2.0f), FMath::RoundToInt((bMinLocation.Z + bMaxLocation.Z) / 2.0f));

		/*FIntVector intersection1 = FIntVector(FMath::RoundToInt(aMidpoint.X), FMath::RoundToInt(bMidpoint.Y), FMath::RoundToInt(midpoint.Z));
		FIntVector hallwayAStartLocation1 = FIntVector(FMath::RoundToInt(aMidpoint.X), FMath::Min(aMaxLocation.Y, bMaxLocation.Y), FMath::Min(aMaxLocation.Z, bMaxLocation.Z));
		FIntVector hallwayBStartLocation1 = FIntVector(FMath::Min(aMaxLocation.X, bMaxLocation.X), FMath::RoundToInt(bMidpoint.Y), FMath::Max(aMaxLocation.Z, bMaxLocation.Z));

		FIntVector intersection2 = FIntVector(FMath::RoundToInt(bMidpoint.X), FMath::RoundToInt(aMidpoint.Y), FMath::RoundToInt(midpoint.Z));
		FIntVector hallwayAStartLocation2 = FIntVector(FMath::RoundToInt(bMidpoint.X), FMath::Max(aMinLocation.Y, bMinLocation.Y), FMath::Min(aMaxLocation.Z, bMaxLocation.Z));
		FIntVector hallwayBStartLocation2 = FIntVector(FMath::Min(aMaxLocation.X, bMaxLocation.X), FMath::RoundToInt(aMidpoint.Y), FMath::Max(aMaxLocation.Z, bMaxLocation.Z));


		// TODO: Make this work
		//bool bCanPlaceIntersectionA = DungeonFloor.HallwayIsClear(hallwayAStartLocation1, intersection1) && DungeonFloor.HallwayIsClear(hallwayBStartLocation1, intersection1);
		//bool bCanPlaceIntersectionB = DungeonFloor.HallwayIsClear(, intersection2);*/
		FIntVector intersection = FIntVector(aMidpoint.X, bMidpoint.Y, midpoint.Z);
		FIntVector hallway1Start;
		FIntVector hallway2Start;
		if (aMinLocation.Y > bMaxLocation.Y)
		{
			// A is on top of B
			hallway1Start = FIntVector(aMidpoint.X, aMinLocation.Y, aMinLocation.Z);
			hallway2Start = FIntVector(bMinLocation.X, bMidpoint.Y, bMinLocation.Z);
		}
		else
		{
			// B is on top of A
			// We know there's no overlap
			hallway1Start = FIntVector(aMidpoint.X, aMaxLocation.Y, aMaxLocation.Z);
			hallway2Start = FIntVector(bMaxLocation.X, bMidpoint.Y, bMinLocation.Z);
		}

		ADungeonRoom* hallway1 = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		ADungeonRoom* hallway2 = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		
		FString hallway1Name = hallwayName + " 1";
		FString hallway2Name = hallwayName + " 2";
		hallway1->Rename(*hallway1Name);
		hallway2->Rename(*hallway2Name);

		hallway1->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallway1Start, intersection, HALLWAY_WIDTH);
		hallway2->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallway2Start, intersection, HALLWAY_WIDTH);

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
	/*// Each location corresponds to the top-left of the room
	// a|bLocation ______
	//            |  A|B |
	//            |______|
	//
	//    b|aLocation _________
	//               |   B|A   |
	//               |_________|

	// Sort them into top and bottom
	if (aLocation.Y > bLocation.Y)
	{
		// B is above A, reverse them
		FIntVector bottom = aLocation;
		aLocation = bLocation;
		bLocation = bottom;

		// Reverse the pointers as well
		ADungeonRoom* bottomRoom = A;
		A = B;
		B = bottomRoom;
	}
	// New:
	// aLocation ______
	//          |  A   |
	//          |______|
	//
	//    bLocation _________
	//             |    B    |
	//             |_________|

	// Edge case:
	// Rooms aligned on Y axis
	// bLocation _________     aLocation ______
	//          |    B    |             |   A  |
	//          |         |             |______|
	//          |_________|

	if (aLocation.Y == bLocation.Y)
	{
		// Set A to be the shorter of the two rooms
		if (B->YSize() < A->YSize())
		{
			FIntVector tempVector = aLocation;
			aLocation = bLocation;
			bLocation = tempVector;

			ADungeonRoom* tempRoom = A;
			A = B;
			B = tempRoom;
		}

		// New:
		// aLocation _________     bLocation ______
		//          |    A    |             |   B  |
		//          |         |             |______|
		//          |_________|
	}

	// If they share a border, we don't need to do any processing at all
	if (bLocation.X == aLocation.X + A->XSize())
	{
		// Share border along Y axis
		//  _________ ______
		// |    A    |   B  |
		// |         |______|
		// |_________|

		// Ensure that there is actually overlap between their edges
		// A is guaranteed to be above or on the same level as B
		if (bLocation.Y > aLocation.Y + A->YSize())
		{
			return hallways;
		}
	}
	if (bLocation.Y == aLocation.Y + A->YSize())
	{
		//  _________ 
		// |    A    |
		// |         |
		// |_________|
		// |     |
		// |  B  |
		// |_____|
		if (bLocation.X + B->XSize() > aLocation.X && bLocation.X < aLocation.X + A->XSize())
		{
			return hallways;
		}
	}

	FString hallwayName = A->GetName() + " - " + B->GetName();

	// We can select any point between aLocation.X to bLocation.X + b width (XSize())
	// This int is an X coordinate in world space
	int32 randomLocation = -1;
	// If there's an overlap between rooms, limit it to that overlap
	// This prevents overly-winding hallways
	if (bLocation.X > aLocation.X && bLocation.X < aLocation.X + A->XSize())
	{
		// B starts somewhere between start of A and end of A
		int32 endRange = FMath::Min(bLocation.X + B->XSize(), aLocation.X + A->XSize());
		if (endRange - bLocation.X - (HALLWAY_EDGE_OFFSET * 2) >= HALLWAY_WIDTH)
		{
			randomLocation = Rng.RandRange(bLocation.X + HALLWAY_EDGE_OFFSET, endRange - HALLWAY_EDGE_OFFSET - 1 - HALLWAY_WIDTH);
		}
	}
	else if (aLocation.X > bLocation.X && aLocation.X < bLocation.X + B->XSize())
	{
		// A starts somewhere between start of B and end of B
		int32 endRange = FMath::Min(bLocation.X + B->XSize(), aLocation.X + A->XSize());
		if (endRange - bLocation.X - (HALLWAY_EDGE_OFFSET * 2) >= HALLWAY_WIDTH)
		{
			randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, endRange - HALLWAY_EDGE_OFFSET - 1 - HALLWAY_WIDTH);
		}
	}
	if(randomLocation == -1)
	{
		// Select it to be either completely in A OR completely in B, but not touching both
		/*if (bLocation.X > aLocation.X)
		{
			// A is to the left of B
			if (Rng.GetFraction() > 0.5f)
			{
				// Use A for our point
			}
			else
			{
				// Use B for our point
			}
		}
		else
		{
			// A is to the right of B
			if (Rng.GetFraction() > 0.5f)
			{
				// Use A for our point
			}
			else
			{
				// Use B for our point
			}
		}*/
		/*randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, bLocation.X + B->XSize() - HALLWAY_EDGE_OFFSET - 1 - HALLWAY_WIDTH);
	}
	// Edge cases:
	// Point not in either room
	// aLocation ______
	//          |  A   |
	//          |______|
	//                     ^
	//                     Point
	//
	//                    bLocation _________
	//                             |    B    |
	//                             |_________|
	if (randomLocation > aLocation.X + A->XSize() && randomLocation < bLocation.X ||
		randomLocation > bLocation.X + B->XSize() && randomLocation < aLocation.X)
	{
		// Limit it to be somewhere along only room A
		randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, aLocation.X + A->XSize() - HALLWAY_EDGE_OFFSET - 1);
		// New:
		// aLocation ______
		//          |  A   |
		//          |______|
		//              ^
		//              Point
		//
		//                    bLocation _________
		//                             |    B    |
		//                             |_________|
	}

	// Edge case:
	// Rooms aligned on Y axis (no top or bottom)
	// aLocation ______     bLocation _________
	//          |  A   |             |    B    |
	//          |______|             |_________|
	//           ^
	//           Point
	if (aLocation.Y == bLocation.Y)
	{
		hallwayName.Append(" (Y Coordinates are the Same)");
		// A is guaranteed to be the same size or smaller on the Y axis
		randomLocation = Rng.RandRange(aLocation.Y + HALLWAY_EDGE_OFFSET, aLocation.Y + A->YSize() - HALLWAY_EDGE_OFFSET - 1);

		// aLocation ______                   bLocation _________
		//          |  A   |<-Point                    |    B    |
		//          |______|                           |_________|
		FIntVector hallwayStart; 
		FIntVector hallwayEnd;
		if (aLocation.X < bLocation.X)
		{
			// A is to the left of B
			hallwayStart = FIntVector(aLocation.X + A->XSize(), randomLocation, aLocation.Z);
			hallwayEnd = FIntVector(bLocation.X, randomLocation, bLocation.Z);
		}
		else
		{
			// B is to the left of A
			hallwayStart = FIntVector(bLocation.X + B->XSize(), randomLocation, aLocation.Z);
			hallwayEnd = FIntVector(aLocation.X, randomLocation, bLocation.Z);
		}
		// aLocation ______                   bLocation _________
		//          |  A   |<-hallwayStart hallwayEnd->|    B    |
		//          |______|                           |_________|
		
		// Goal:
		//           ______                             _________
		//          |  A   |___________________________|    B    |
		//          |______|                           |_________|

		//ADungeonRoom* hallway = NewObject<ADungeonRoom>(A->GetOwner(), FName(*hallwayName));
		ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		hallway->Rename(*hallwayName);
		hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallwayStart, hallwayEnd, HALLWAY_WIDTH);
		//hallway->InitializeRoom(
		//	DefaultTile, hallwayEnd.X - hallwayStart.X, HALLWAY_WIDTH, 
		//	hallwayStart.X, hallwayStart.Y, hallwayStart.Z, 
		//	HallwaySymbol, Rng, false);
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
	else
	{
		// There are now 3 different possibilities:
		// A (Point only touching room A):
		// aLocation ______
		//          |  A   |
		//          |______|
		//           ^
		//           Point
		//
		//    bLocation _________
		//             |    B    |
		//             |_________|
		//
		// B (Point in the intersection of both rooms:
		// aLocation ______
		//          |  A   |
		//          |______|
		//               ^
		//               Point
		//
		//    bLocation _________
		//             |    B    |
		//             |_________|
		//
		// C (Point only touching room B):
		// aLocation ______
		//          |  A   |
		//          |______|
		//           
		//                   Point
		//                   v
		//    bLocation _________
		//             |    B    |
		//             |_________|

		// Solutions are as follows:
		// A:
		// aLocation ______
		//          |  A   |
		//          |______|
		//           |
		//           |
		//           |
		//    bLocation _________
		//           |_|    B    |
		//             |_________|
		//
		// B:
		// aLocation ______
		//          |  A   |
		//          |______|
		//               |
		//               |
		//               |
		//    bLocation _|_______
		//             |    B    |
		//             |_________|
		//
		// C:
		// aLocation ______
		//          |  A   |_
		//          |______| |
		//                   |
		//                   |
		//                   |
		//    bLocation _____|___
		//             |    B    |
		//             |_________|

		if (randomLocation < bLocation.X || 
			randomLocation > aLocation.X && randomLocation > bLocation.X + B->XSize())
		{
			// Case A (Point only touching room A)
			// aLocation ______
			//          |  A   |
			//          |______|
			//           ^
			//           Point
			//
			//    bLocation _________
			//             |    B    |
			//             |_________|
			FString hallwayAName = hallwayName + " (Case A) A";
			FString hallwayBName = hallwayName + " (Case A) B";

			// Select a random location along the Y direction of room B
			//    bLocation _________
			//   Point B ->|    B    |
			//             |_________|
			int32 pointB = Rng.RandRange(bLocation.Y - HALLWAY_EDGE_OFFSET, bLocation.Y + B->YSize() - HALLWAY_EDGE_OFFSET - 1);
			// Define points
			FIntVector aHallwayStart = FIntVector(randomLocation, aLocation.Y + A->YSize(), aLocation.Z);
			FIntVector hallwayIntersection = FIntVector(randomLocation, pointB, bLocation.Z);

			// Create hallway A
			ADungeonRoom* hallwayA = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayA->Rename(*hallwayAName);

			hallwayA->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, aHallwayStart, hallwayIntersection, HALLWAY_WIDTH);
			hallways.Add(hallwayA);

			FIntVector bHallwayStart;

			if (aLocation.X > bLocation.X)
			{
				// Edge case: B is to the left of A
				//                            aLocation ______
				//                                     |  A   |
				//                                     |______|
				//                                aHallway->|
				//                                          |
				//                                          |
				//        bLocation _________               |
				//                 |    B    |______________|<-intersection
				//                 |_________|^ Point B
				hallwayBName += " Edge Case";
				bHallwayStart = FIntVector(bLocation.X + B->XSize() - HALLWAY_WIDTH, pointB, bLocation.Z);
			}
			else
			{
				//     aLocation ______
				//              |  A   |
				//              |______|
				//     aHallway->|
				//               |
				//               |
				//        bLocation _________
				// intersection->|_|    B    |
				//                ^|_________|
				//         bHallway

				bHallwayStart = FIntVector(bLocation.X, pointB, bLocation.Z);
			}

			ADungeonRoom* hallwayB = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayB->Rename(*hallwayBName);
			hallwayB->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, bHallwayStart, hallwayIntersection, HALLWAY_WIDTH);
			hallways.Add(hallwayB);

			// Fix the mission neighbors array
			if (A->MissionNeighbors.Contains(B))
			{
				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				B->MissionNeighbors.Remove(A);
				B->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(A);
			}
		}
		else if (	randomLocation > bLocation.X && randomLocation < bLocation.X + B->XSize() &&
					randomLocation < aLocation.X + A->XSize())
		{
			// Case B (Point intersects both rooms)
			// aLocation ______
			//          |  A   |
			//          |______|
			//               ^
			//               Point
			//
			//    bLocation _________
			//             |    B    |
			//             |_________|
			hallwayName.Append(" (Case B)");

			FIntVector hallwayStart = FIntVector(randomLocation, aLocation.Y + A->YSize(), aLocation.Z);
			FIntVector hallwayEnd = FIntVector(randomLocation, bLocation.Y, bLocation.Z);

			// aLocation ______
			//          |  A   |
			//          |______|
			//               |
			//               |
			//               |
			//    bLocation _|_______
			//             |    B    |
			//             |_________|

			ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallway->Rename(*hallwayName);
			//hallway->InitializeRoom(
			//	DefaultTile, hallwayEnd.X - hallwayStart.X, HALLWAY_WIDTH,
			//	hallwayStart.X, hallwayStart.Y, hallwayStart.Z,
			//	HallwaySymbol, Rng, false);
			hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				hallwayStart, hallwayEnd, HALLWAY_WIDTH);
			hallways.Add(hallway);

			// Fix the mission neighbors
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
			// Case C (Point only touching room B)
			// aLocation ______
			//          |  A   |
			//          |______|
			//           
			//                   
			//                   Point
			//    bLocation _____v___
			//             |    B    |
			//             |_________|
			FString hallwayAName = hallwayName + " (Case C) A";
			FString hallwayBName = hallwayName + " (Case C) B";

			// Select a random position along the Y direction of room A
			// aLocation ______
			//          |  A   |<-Point A
			//          |______|
			int32 pointA = Rng.RandRange(aLocation.Y - HALLWAY_EDGE_OFFSET, aLocation.Y + A->YSize() - HALLWAY_EDGE_OFFSET - 1);

			// Create a vector pointing to the top of room B, at the random location we selected
			FIntVector bHallwayStart = FIntVector(randomLocation, bLocation.Y, bLocation.Z);
			FIntVector intersection = FIntVector(randomLocation, pointA, aLocation.Z);

			// Create hallway A
			ADungeonRoom* hallwayB = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayB->Rename(*hallwayBName);
			hallwayB->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, intersection, bHallwayStart, HALLWAY_WIDTH);
			hallways.Add(hallwayB);

			FIntVector aHallwayStart;
			if (aLocation.X < bLocation.X)
			{
				// Edge case: B is to the left of A
				//                            aLocation ______
				//                            Point A->|  A   |
				//                                     |______|
				//
				//
				//                      Point
				//        bLocation ____v____
				//                 |    B    |
				//                 |_________|
				aHallwayStart = FIntVector(aLocation.X, pointA, aLocation.Z);
			}
			else
			{
				// aLocation ______
				//          |  A   |_
				//          |______| |
				//                   |
				//                   |
				//                   |
				//    bLocation _____|___
				//             |    B    |
				//             |_________|

				aHallwayStart = FIntVector(aLocation.X + A->XSize(), pointA, bLocation.Z);
			}
			
			ADungeonRoom* hallwayA = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayA->Rename(*hallwayAName);
			
			hallwayA->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, aHallwayStart, intersection, HALLWAY_WIDTH);
			hallways.Add(hallwayA);

			// Fix the mission neighbors array
			if (A->MissionNeighbors.Contains(B))
			{
				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				B->MissionNeighbors.Remove(A);
				B->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(A);
			}
		}
	}*/

	return hallways;
}

void ADungeonRoom::SetTileGridCoordinates(FIntVector CurrentLocation, const UDungeonTile* Tile)
{

	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	Set(CurrentLocation.X - xPosition, CurrentLocation.Y - yPosition, Tile);
}
