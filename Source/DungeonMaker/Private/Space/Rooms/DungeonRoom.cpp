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

	/*MinimumRoomSize.WallSize = 4;
	MinimumRoomSize.CeilingHeight = 1;
	MaximumRoomSize.WallSize = 16;
	MaximumRoomSize.CeilingHeight = 1;*/
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
	/*FRandomStream rng(DebugSeed);
	FIntVector position = GetRoomTileSpacePosition();
	int32 xPosition = position.X;
	int32 yPosition = position.Y;
	int32 zPosition = position.Z;

	// Initialize this room
	InitializeRoom(DebugDefaultFloorTile, DebugDefaultWallTile, DebugDefaultEntranceTile, 
		RoomDifficulty, DebugRoomMaxExtents.X, DebugRoomMaxExtents.Y,
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
	}*/
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
				/*for (ADungeonRoom* neighbor : MissionNeighbors)
				{
					neighbor->OnPlayerEnterNeighborRoom();
				}*/
			}
		}
	}
}

/*bool ADungeonRoom::PathIsClear(FIntVector StartLocation, FIntVector EndLocation, int32 SweepWidth, FDungeonFloor& DungeonFloor)
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

void ADungeonRoom::InitializeRoomFromPoints(const UDungeonTile* DefaultFloorTile,
	const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile, 
	const UDungeonMissionSymbol* RoomSymbol, FIntVector StartLocation, FIntVector EndLocation, 
	int32 Width, bool bIsJoinedToHallway)
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

	InitializeRoom(DefaultFloorTile, DefaultWallTile, DefaultEntranceTile, 
		RoomDifficulty, width, height,
		startingLocation.X, startingLocation.Y, 0,
		RoomSymbol, rng, false, true);

}*/

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
	
	int32 xSize;
	int32 ySize;
	/*if (bIsDeterminedFromPoints)
	{*/
		xSize = MaxXSize;
		ySize = MaxYSize;
	/*}
	else
	{
		check(MaxXSize > ROOM_BORDER_SIZE * 2);
		check(MaxYSize > ROOM_BORDER_SIZE * 2);

		xSize = FMath::Min(MaxXSize - (ROOM_BORDER_SIZE * 2), MaximumRoomSize.WallSize);
		ySize = FMath::Min(MaxYSize - (ROOM_BORDER_SIZE * 2), MaximumRoomSize.WallSize);
	}*/

	int32 xOffset;
	int32 yOffset;
	/*if (bUseRandomDimensions)
	{
		xSize = Rng.RandRange(MinimumRoomSize.WallSize, xSize);
		ySize = Rng.RandRange(MinimumRoomSize.WallSize, ySize);
		// X Offset can be anywhere from our current X position to the start of the room
		// That way we have enough space to place the room
		xOffset = Rng.RandRange(XPosition + ROOM_BORDER_SIZE, MaxXSize - xSize - ROOM_BORDER_SIZE);
		yOffset = Rng.RandRange(YPosition + ROOM_BORDER_SIZE, MaxYSize - ySize - ROOM_BORDER_SIZE);
	}
	else
	{*/
		xOffset = XPosition;
		yOffset = YPosition;
	/*}*/

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

void ADungeonRoom::PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& ComponentLookup,
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
	TSet<FIntVector> neighbors = RoomMetadata.NeighboringRooms.Union(RoomMetadata.NeighboringTightlyCoupledRooms);
	for (FIntVector neighbor : neighbors)
	{
		// We handle spawning the entrance to the room above or to the right of us
		// The other room will spawn any other entrances
		if (neighbor.X > RoomMetadata.Location.X)
		{
			int entranceLocation = Rng.RandRange(1, YSize() - 2);
			Set(XSize() - 1, entranceLocation, EntranceTile);
		}
		if (neighbor.Y > RoomMetadata.Location.Y)
		{
			int entranceLocation = Rng.RandRange(1, XSize() - 2);
			Set(entranceLocation, YSize() - 1, EntranceTile);
		}
	}
}
