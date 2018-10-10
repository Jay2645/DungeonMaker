// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonRoom.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonFloorManager.h"
#include "GroundScatterManager.h"
#include <DrawDebugHelpers.h>
#include "GameFramework/Character.h"
#include "Trials/TrialRoom.h"
#include "LockedRoom.h"
#include "KeyRoom.h"

// Sets default values for this component's properties
ADungeonRoom::ADungeonRoom()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	RoomTiles = FDungeonRoomMetadata();
	Symbol = NULL;
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	GroundScatter = CreateDefaultSubobject<UGroundScatterManager>(TEXT("Ground Scatter"));
	
	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Room Trigger"));
	RoomTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	NorthEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("North Entrance Trigger"));
	NorthEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	SouthEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("South Entrance Trigger"));
	SouthEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	WestEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("West Entrance Trigger"));
	WestEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	EastEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("East Entrance Trigger"));
	EastEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FCollisionResponseContainer collisonChannels;
	collisonChannels.SetAllChannels(ECR_Ignore);
	collisonChannels.SetResponse(ECollisionChannel::ECC_Pawn, ECR_Overlap);

	RoomTrigger->SetCollisionResponseToChannels(collisonChannels);
	RoomTrigger->SetupAttachment(DummyRoot);
	RoomTrigger->SetGenerateOverlapEvents(true);
	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginTriggerOverlap);
	RoomTrigger->OnComponentEndOverlap.AddDynamic(this, &ADungeonRoom::OnEndTriggerOverlap);

	NorthEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	NorthEntranceTrigger->SetupAttachment(DummyRoot);
	NorthEntranceTrigger->SetGenerateOverlapEvents(true);
	NorthEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	SouthEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	SouthEntranceTrigger->SetupAttachment(DummyRoot);
	SouthEntranceTrigger->SetGenerateOverlapEvents(true);
	SouthEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	WestEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	WestEntranceTrigger->SetupAttachment(DummyRoot);
	WestEntranceTrigger->SetGenerateOverlapEvents(true);
	WestEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	EastEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	EastEntranceTrigger->SetupAttachment(DummyRoot);
	EastEntranceTrigger->SetGenerateOverlapEvents(true);
	EastEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	DebugRoomMaxExtents = FIntVector(16, 16, 1);
	MaxRoomHeight = 1;
	MinRoomSize = FIntVector(7, 7, 1);
	RoomDifficultyModifier = 1.0f;
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

void ADungeonRoom::OnEndTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerLeaveRoom();
			}
		}
	}
}

void ADungeonRoom::OnBeginEntranceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerEnterRoomEntrance();
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

	SetActorLocation(worldPosition);

	float tileSize = UDungeonTile::TILE_SIZE;
	float halfTileSize = tileSize * 0.5f;

	FVector halfExtents = FVector((xSize * halfTileSize), (ySize * halfTileSize), tileSize);
	RoomTrigger->SetRelativeLocation(halfExtents - FVector(0.0f, tileSize, tileSize));
	RoomTrigger->SetBoxExtent(halfExtents - FVector(tileSize, tileSize, 0.0f));

	NorthEntranceTrigger->SetRelativeLocation(FVector(xSize * halfTileSize, (ySize - 1) * tileSize - halfTileSize, tileSize));
	NorthEntranceTrigger->SetBoxExtent(FVector(xSize * halfTileSize, halfTileSize, tileSize));

	SouthEntranceTrigger->SetRelativeLocation(FVector(xSize * halfTileSize, -halfTileSize, tileSize));
	SouthEntranceTrigger->SetBoxExtent(FVector(xSize * halfTileSize, halfTileSize, tileSize));

	EastEntranceTrigger->SetRelativeLocation(FVector(halfTileSize, (ySize - 1) * halfTileSize - halfTileSize, tileSize));
	EastEntranceTrigger->SetBoxExtent(FVector(halfTileSize, ySize * halfTileSize, tileSize));

	WestEntranceTrigger->SetRelativeLocation(FVector(xSize * tileSize - halfTileSize, (ySize - 1) * halfTileSize - halfTileSize, tileSize));
	WestEntranceTrigger->SetBoxExtent(FVector(halfTileSize, ySize * halfTileSize, tileSize));

	OnRoomInitialized();

	UE_LOG(LogSpaceGen, Log, TEXT("Initialized %s to dimensions %d x %d."), *GetName(), RoomTiles.XSize(), RoomTiles.YSize());
}

void ADungeonRoom::DoTileReplacement(FRandomStream &Rng)
{
	OnPreRoomTilesReplaced();
	DoTileReplacementPreprocessing(Rng);

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

			if (!replacementPatterns[rngIndex]->FindAndReplace(RoomTiles, Rng))
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

void ADungeonRoom::PlaceRoomTiles(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
	TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
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
}

void ADungeonRoom::DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations, FRandomStream& Rng)
{
	GroundScatter->DetermineGroundScatter(TileLocations, Rng, this);
}

FTransform ADungeonRoom::GetTileTransform(const FIntVector& LocalLocation) const
{
	FIntVector worldLocation = GetRoomTileSpacePosition() + LocalLocation;
	return GetTileTransformFromTileSpace(worldLocation);
}


FTransform ADungeonRoom::GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const
{
	float tileSize = UDungeonTile::TILE_SIZE;
	float halfTileSize = tileSize * 0.5f;

	FVector location = FVector(WorldLocation.X * tileSize, WorldLocation.Y * tileSize, WorldLocation.Z * tileSize);
	FRotator rotation = GetActorRotation();
	FVector scale = GetActorScale();

	ETileDirection direction = GetTileDirection(WorldLocation);
	switch (direction)
	{
	case ETileDirection::Center:
		location.X -= halfTileSize;
		location.Y -= halfTileSize;
		break;
	case ETileDirection::North:
		// Pass
		break;
	case ETileDirection::South:
		location.Y -= tileSize;
		rotation.Yaw += 180.0f;
		break;
	case ETileDirection::East:
		rotation.Yaw += 90.0f;
		break;
	case ETileDirection::West:
		location.X += tileSize;
		rotation.Yaw += 270.0f;
		break;
	case ETileDirection::Northeast:
		rotation.Yaw += 45.0f;
		break;
	case ETileDirection::Northwest:
		location.X += tileSize;
		rotation.Yaw += 315.0f;
		break;
	case ETileDirection::Southeast:
		location.Y -= tileSize;
		rotation.Yaw += 135.0f;
		break;
	case ETileDirection::Southwest:
		location.X += tileSize;
		location.Y -= tileSize;
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

TSet<FIntVector> ADungeonRoom::GetAllTilesOfType(ETileType Type) const
{
	TSet<FIntVector> locations;
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			const UDungeonTile* tile = GetTile(x, y);
			if (tile != NULL && tile->TileType == Type)
			{
				locations.Add(FIntVector(x, y, RoomLevel));
			}
		}
	}
	return locations;
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
	return (int32)ActualRoomHeight;
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

ETileDirection ADungeonRoom::GetTileDirectionLocalSpace(FIntVector Location) const
{
	// Top-left is northwest corner
	// Bottom-right is southeast corner
	bool bIsOnLeft = Location.X <= 0;
	bool bIsOnRight = Location.X >= XSize() - 1;
	bool bIsOnTop = Location.Y <= 0;
	bool bIsOnBottom = Location.Y >= YSize() - 1;

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

void ADungeonRoom::CreateNewTileMesh(const UDungeonTile* Tile, const FTransform& Location)
{
	if (DungeonSpace->FloorComponentLookup.Contains(Tile))
	{
		DungeonSpace->FloorComponentLookup[Tile]->AddInstance(FloorTileMeshSelections[Tile], Location);
	}
	else if (DungeonSpace->CeilingComponentLookup.Contains(Tile))
	{
		DungeonSpace->CeilingComponentLookup[Tile]->AddInstance(CeilingTileMeshSelections[Tile], Location);
	}
	else
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Could not create new mesh at location!"));
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
	return RoomMetadata.Difficulty * RoomDifficultyModifier;
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

void ADungeonRoom::DoTileReplacementPreprocessing(FRandomStream& Rng)
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

void ADungeonRoom::PlaceTile(TMap<const UDungeonTile*, ASpaceMeshActor*>& ComponentLookup,
	const UDungeonTile* Tile, int32 MeshID, const FTransform& MeshTransformOffset, const FIntVector& Location)
{
	if (!ComponentLookup.Contains(Tile))
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Missing Instanced Static Mesh Component for tile %s!"), *Tile->TileID.ToString());
		return;
	}

	FTransform objectTransform = CreateMeshTransform(MeshTransformOffset, Location);
	ComponentLookup[Tile]->AddInstance(MeshID, objectTransform);
}

FTransform ADungeonRoom::CreateMeshTransform(const FTransform &MeshTransformOffset, const FIntVector &Location) const
{
	// Fetch Dungeon transform
	FTransform actorTransform = GetActorTransform();
	FVector actorPosition = actorTransform.GetLocation();
	// Add the local tile position, rotation, and the dungeon offset
	FVector meshPosition = MeshTransformOffset.GetLocation();
	FVector offset = FVector(Location.X * UDungeonTile::TILE_SIZE, Location.Y * UDungeonTile::TILE_SIZE, Location.Z * UDungeonTile::TILE_SIZE);
	FVector finalPosition = actorPosition + meshPosition + offset;
	// Add the rotations
	FRotator meshRotation = FRotator(MeshTransformOffset.GetRotation());
	FRotator finalRotation = FRotator(actorTransform.GetRotation());
	finalRotation.Add(meshRotation.Pitch, meshRotation.Yaw, meshRotation.Roll);

	// Create the tile
	return FTransform(finalRotation, finalPosition, MeshTransformOffset.GetScale3D());
}

AActor* ADungeonRoom::SpawnInteraction(const UDungeonTile* Tile, FDungeonTileInteractionOptions TileInteractionOptions, 
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
	ETileDirection direction = GetTileDirection(Location);
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

void ADungeonRoom::CreateAllRoomTiles(TMap<const UDungeonTile*, TArray<FIntVector>>& TileLocations,
	TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
	TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
	FRandomStream& Rng)
{
	// Determine how high the ceiling is
	ActualRoomHeight = Rng.RandRange(MinRoomSize.Z, MaxRoomHeight);

	// Place tiles
	for (auto& kvp : TileLocations)
	{
		if (FloorComponentLookup.Contains(kvp.Key))
		{
			for (int i = 0; i < kvp.Value.Num(); i++)
			{
				int32 meshSelection;
				if (FloorTileMeshSelections.Contains(kvp.Key))
				{
					meshSelection = FloorTileMeshSelections[kvp.Key];
				}
				else
				{
					do
					{
						meshSelection = Rng.RandRange(0, kvp.Key->GroundMesh.Num() - 1);
					} while (kvp.Key->GroundMesh[meshSelection].SelectionChance < Rng.GetFraction());
				}
				PlaceTile(FloorComponentLookup, kvp.Key, meshSelection, kvp.Key->GroundMesh[meshSelection].Transform, TileLocations[kvp.Key][i]);
			}
		}
		if (CeilingComponentLookup.Contains(kvp.Key))
		{
			for (int i = 0; i < kvp.Value.Num(); i++)
			{
				int32 meshSelection;
				FIntVector currentLocation = TileLocations[kvp.Key][i];
				if (CeilingTileMeshSelections.Contains(kvp.Key))
				{
					meshSelection = CeilingTileMeshSelections[kvp.Key];
				}
				else
				{
					do
					{
						meshSelection = Rng.RandRange(0, kvp.Key->CeilingMesh.Num() - 1);
					} while (kvp.Key->CeilingMesh[meshSelection].SelectionChance < Rng.GetFraction());
				}
				FTransform meshTransform = kvp.Key->CeilingMesh[meshSelection].Transform;
				
				// On any tile that's not part of the entrance, set the ceiling up high
				// Entrances should have their ceiling match the door
				if (!EntranceLocations.Contains(currentLocation))
				{
					meshTransform.AddToTranslation(FVector(0.0f, 0.0f, (ActualRoomHeight - 1) * 500.0f));
				}
				PlaceTile(CeilingComponentLookup, kvp.Key, meshSelection, meshTransform, currentLocation);
			}
		}
	}

	// Spawn walls up to the ceiling height
	if (ActualRoomHeight > 1)
	{
		const UDungeonTile* wallTile = GetTile(0, 0);
		if (FloorComponentLookup.Contains(wallTile))
		{
			int32 meshSelection = FloorTileMeshSelections[wallTile];
			TArray<FIntVector> wallLocations = GetTileLocations(wallTile);
			wallLocations.Append(EntranceLocations.Array());
			for (int i = 1; i < ActualRoomHeight; i++)
			{
				for (int j = 0; j < wallLocations.Num(); j++)
				{
					FIntVector location = wallLocations[j];
					location.Z = i;
					FTransform offset;
					PlaceTile(FloorComponentLookup, wallTile, FloorTileMeshSelections[wallTile], offset, location);
				}
			}
		}
	}

	// Place tile interactions
	for (auto& kvp : InteractionOptions)
	{
		for (int i = 0; i < TileLocations[kvp.Key].Num(); i++)
		{
			SpawnInteraction(kvp.Key, kvp.Value, TileLocations[kvp.Key][i], Rng);
		}
	}

	SpawnInterfaces(Rng);

	// Determine ground scatter
	DetermineGroundScatter(TileLocations, Rng);
}

void ADungeonRoom::SpawnInterfaces(FRandomStream &Rng)
{
	// Place traps
	if (GetClass()->ImplementsInterface(UTrialRoom::StaticClass()))
	{
		TArray<AActor*> spawnedTriggers = ITrialRoom::Execute_CreateTriggers(this, Rng);
#if WITH_EDITOR
		for (AActor* trigger : spawnedTriggers)
		{
			FString folderPath = "Rooms/Triggers/";
			folderPath.Append(trigger->GetClass()->GetName());
			trigger->SetFolderPath(FName(*folderPath));
		}
#endif

		TArray<AActor*> spawnedTraps = ITrialRoom::Execute_CreateTraps(this, Rng);
#if WITH_EDITOR
		for (AActor* trap : spawnedTraps)
		{
			FString folderPath = "Rooms/Traps/";
			folderPath.Append(trap->GetClass()->GetName());
			trap->SetFolderPath(FName(*folderPath));
		}
#endif
	}

	if (GetClass()->ImplementsInterface(ULockedRoom::StaticClass()))
	{
		AActor* lock = ILockedRoom::Execute_SpawnLock(this, Rng);
		// Generate the next seed, since the RNG has to be passed via copy
		Rng.GenerateNewSeed();
#if WITH_EDITOR
		FString folderPath = "Rooms/Locks/";
		folderPath.Append(lock->GetClass()->GetName());
		lock->SetFolderPath(FName(*folderPath));
#endif
	}

	if (GetClass()->ImplementsInterface(UKeyRoom::StaticClass()))
	{
		AActor* key = IKeyRoom::Execute_SpawnKey(this, Rng);
		Rng.GenerateNewSeed();
#if WITH_EDITOR
		FString folderPath = "Rooms/Keys/";
		folderPath.Append(key->GetClass()->GetName());
		key->SetFolderPath(FName(*folderPath));
#endif
	}
}
