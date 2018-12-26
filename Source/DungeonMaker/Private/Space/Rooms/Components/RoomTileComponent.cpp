

#include "RoomTileComponent.h"
#include "DungeonFloorHelpers.h"

// Sets default values for this component's properties
URoomTileComponent::URoomTileComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	MinRoomSize = FIntVector(5, 5, 1);

	bDrawDebugTiles = false;
	bPrintRoomTiles = true;
	bDoTileReplacement = true;
}

const UDungeonTile* URoomTileComponent::GetTile(const FIntVector& Location)
{
	return GetDungeon().GetTile(Location);
}

void URoomTileComponent::InitializeTileComponent(ADungeonRoom* Room, const FIntVector& RoomDimensions, const FIntVector& RoomPosition,
	const UDungeonTile* DefaultFloorTile, const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile, 
	const UDungeonTile* DefaultExitTile, FRandomStream& Rng, bool bUseRandomSize)
{
	ParentRoom = Room;
	RoomLocation = RoomPosition;
	RoomEntranceTile = DefaultEntranceTile;
	RoomExitTile = DefaultExitTile;

	int32 xSize, ySize, zSize;

	if (bUseRandomSize)
	{
		xSize = Rng.RandRange(MinRoomSize.X, FMath::Abs(RoomDimensions.X));
		ySize = Rng.RandRange(MinRoomSize.Y, FMath::Abs(RoomDimensions.Y));
		zSize = Rng.RandRange(MinRoomSize.Z, (int32)MaxRoomHeight);
	}
	else
	{
		xSize = FMath::Max(RoomDimensions.X, MinRoomSize.X);
		ySize = FMath::Max(RoomDimensions.Y, MinRoomSize.Y);
		zSize = FMath::Max(RoomDimensions.Z, MinRoomSize.Z);
	}
	RoomSize = FIntVector(xSize, ySize, zSize);

	FVector worldPosition = FVector(
		RoomLocation.X * UDungeonTile::TILE_SIZE,
		RoomLocation.Y * UDungeonTile::TILE_SIZE,
		RoomLocation.Z * UDungeonTile::TILE_SIZE);
	ParentRoom->SetActorLocation(worldPosition);

	SpawnStartingDefaultTiles(DefaultFloorTile);
	CarveWalls(DefaultWallTile);
}

void URoomTileComponent::SpawnStartingDefaultTiles(const UDungeonTile* DefaultTile)
{
	FDungeonSpace& dungeon = GetDungeon();

	for (int x = RoomLocation.X; x < RoomLocation.X + RoomSize.X; x++)
	{
		for (int y = RoomLocation.Y; y < RoomLocation.Y + RoomSize.Y; y++)
		{
			dungeon.SetTile(FIntVector(x, y, RoomLocation.Z), DefaultTile);
		}
	}
}

void URoomTileComponent::CarveWalls(const UDungeonTile* WallTile)
{
	FDungeonSpace& dungeon = GetDungeon();

	for (int x = RoomLocation.X; x < RoomLocation.X + RoomSize.X; x++)
	{
		for (int y = RoomLocation.Y; y < RoomLocation.Y + RoomSize.Y; y++)
		{
			FIntVector location = FIntVector(x, y, RoomLocation.Z);
			ETileDirection direction = GetTileDirection(location);

			if (direction == ETileDirection::Center)
			{
				// Not a wall
				continue;
			}

			dungeon.SetTile(location, WallTile);
		}
	}
}

TArray<ADungeonRoom*> URoomTileComponent::ConnectToRoom(ADungeonRoom* OtherRoom, TSubclassOf<ADungeonRoom> HallwayClass, FRandomStream& Rng)
{
	TArray<ADungeonRoom*> hallways;
	if (ParentRoom == NULL || OtherRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Can't create hallways as a room was null!"));
		return hallways;
	}

	FDungeonSpace& dungeon = GetDungeon();

	ADungeonRoom* firstRoom = ParentRoom;
	ADungeonRoom* secondRoom = OtherRoom;

	FIntVector firstMinExtent = ParentRoom->GetRoomLocation();
	FIntVector secondMinExtent = OtherRoom->GetRoomLocation();
	FIntVector firstMaxExtent = firstMinExtent + ParentRoom->GetRoomSize();
	FIntVector secondMaxExtent = secondMinExtent + OtherRoom->GetRoomSize();
	if (UDungeonFloorHelpers::AreRoomsLeftRightAdjacent(ParentRoom, OtherRoom))
	{
		if (firstMinExtent.X > secondMinExtent.X)
		{
			// Set the left one as the first
			FIntVector temp = firstMinExtent;
			firstMinExtent = secondMinExtent;
			secondMinExtent = temp;
			temp = firstMaxExtent;
			firstMaxExtent = secondMaxExtent;
			secondMaxExtent = temp;
			ADungeonRoom* tempRoom = firstRoom;
			firstRoom = secondRoom;
			secondRoom = tempRoom;
		}
		// First is guaranteed to be on the left now
		// We know it is adjacent to the one on the right
		
		// Adjust for the walls
		firstMinExtent.Y += (1 + firstRoom->GetTileComponent()->DoorYOffset);
		secondMinExtent.Y += (1 + secondRoom->GetTileComponent()->DoorXOffset);
		firstMaxExtent.Y -= (1 + firstRoom->GetTileComponent()->DoorYOffset);
		secondMaxExtent.Y -= (1 + secondRoom->GetTileComponent()->DoorYOffset);

		// Now check the range
		int32 endRange = FMath::Min(firstMaxExtent.Y, secondMaxExtent.Y) - 1;
		int32 startRange;
		if (firstMinExtent.Y >= secondMinExtent.Y && firstMinExtent.Y <= secondMaxExtent.Y)
		{
			startRange = firstMinExtent.Y;
		}
		else
		{
			startRange = secondMinExtent.Y;
		}

		int32 yCoordinate = Rng.RandRange(startRange, endRange);
		FIntVector first = FIntVector(firstMaxExtent.X - 1, yCoordinate, firstMinExtent.Z);
		FIntVector second = FIntVector(secondMinExtent.X, yCoordinate, secondMinExtent.Z);

		if (firstRoom->IsChildOf(secondRoom))
		{
			// Player enters first room from second room
			dungeon.SetTile(first, RoomEntranceTile);
			dungeon.SetTile(second, RoomExitTile);
		}
		else
		{
			// Player enters second room from first room
			dungeon.SetTile(first, RoomExitTile);
			dungeon.SetTile(second, RoomEntranceTile);
		}
	}
	else if(UDungeonFloorHelpers::AreRoomsTopDownAdjacent(ParentRoom, OtherRoom))
	{
		if (firstMinExtent.Y > secondMinExtent.Y)
		{
			// Set the left one as the first
			FIntVector temp = firstMinExtent;
			firstMinExtent = secondMinExtent;
			secondMinExtent = temp;
			temp = firstMaxExtent;
			firstMaxExtent = secondMaxExtent;
			secondMaxExtent = temp;
			ADungeonRoom* tempRoom = firstRoom;
			firstRoom = secondRoom;
			secondRoom = tempRoom;
		}
		// First is guaranteed to be on the left now
		// We know it is adjacent to the one on the right

		// Adjust for the walls
		firstMinExtent.X += (1 + firstRoom->GetTileComponent()->DoorXOffset);
		secondMinExtent.X += (1 + secondRoom->GetTileComponent()->DoorXOffset);
		firstMaxExtent.X -= (1 + firstRoom->GetTileComponent()->DoorXOffset);
		secondMaxExtent.X -= (1 + secondRoom->GetTileComponent()->DoorXOffset);

		// Now check the range
		int32 endRange = FMath::Min(firstMaxExtent.X, secondMaxExtent.X) - 1;
		int32 startRange;
		if (firstMinExtent.X >= secondMinExtent.X && firstMinExtent.X <= secondMaxExtent.X)
		{
			startRange = firstMinExtent.X;
		}
		else
		{
			startRange = secondMinExtent.X;
		}

		int32 xCoordinate = Rng.RandRange(startRange, endRange);
		FIntVector first = FIntVector(xCoordinate, firstMaxExtent.Y - 1, firstMinExtent.Z);
		FIntVector second = FIntVector(xCoordinate, secondMinExtent.Y, secondMinExtent.Z);

		if (firstRoom->IsChildOf(secondRoom))
		{
			// Player enters first room from second room
			dungeon.SetTile(first, RoomEntranceTile);
			dungeon.SetTile(second, RoomExitTile);
		}
		else
		{
			// Player enters second room from first room
			dungeon.SetTile(first, RoomExitTile);
			dungeon.SetTile(second, RoomEntranceTile);
		}
	}
	else
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("%s needs hallways to connect to %s."), *ParentRoom->GetName(), *OtherRoom->GetName());
	}
	return hallways;
}

FDungeonSpace& URoomTileComponent::GetDungeon() const
{
	return ParentRoom->GetDungeon();
}

void URoomTileComponent::DoTileReplacement(FRandomStream &Rng)
{
	if (!bDoTileReplacement)
	{
		return;
	}

	// Replace them based on our replacement rules
	TArray<FRoomReplacements> replacementPhases = RoomReplacementPhases;
	TMap<int32, uint8> replacementCounts;

#if !UE_BUILD_SHIPPING
	int32 totalReplacements = 0;
#endif

	// Iterate over each replacement phase
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
			if (Rng.GetFraction() > replacementPatterns[rngIndex]->GetActualSelectionChance(ParentRoom))
			{
				continue;
			}

			if (!replacementPatterns[rngIndex]->FindAndReplace(GetDungeon(), ParentRoom, Rng))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
			else
			{
				// Found replacement!

#if !UE_BUILD_SHIPPING
				totalReplacements++;
#endif
				// Keep track of the replacement count for this tile
				replacementCounts[rngIndex]++;

				// See if we've used it too much
				uint8 maxReplacements = replacementPatterns[rngIndex]->MaxReplacementCount;
				if (maxReplacements > 0 && replacementCounts[rngIndex] >= maxReplacements)
				{
					// If we've exceeded our max replacement count, remove us from consideration
					replacementPatterns.RemoveAt(rngIndex);
				}
			}
		}
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogSpaceGen, Verbose, TEXT("%s made a total of %d tile replacements."), *ParentRoom->GetName(), totalReplacements);

	if (bPrintRoomTiles)
	{
		UE_LOG(LogSpaceGen, Log, TEXT("%s (%s) Tile Map:\n%s"), *ParentRoom->GetName(), *ParentRoom->GetClass()->GetName(), *(GetDungeon().RoomToString(ParentRoom)));
	}
#endif
}

FIntVector URoomTileComponent::GetRoomTileSpacePosition() const
{
	return RoomLocation;
}

FTransform URoomTileComponent::GetTileTransformFromLocalSpace(const FIntVector& LocalLocation) const
{
	FIntVector worldLocation = GetRoomTileSpacePosition() + FIntVector(LocalLocation);
	return GetTileTransformFromTileSpace(worldLocation);
}

FTransform URoomTileComponent::GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const
{
	if (ParentRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("RoomTileComponent was never initialized!"));
		return FTransform();
	}

	float tileSize = UDungeonTile::TILE_SIZE;
	float halfTileSize = tileSize * 0.5f;

	FVector location = FVector(WorldLocation.X * tileSize, WorldLocation.Y * tileSize, WorldLocation.Z * tileSize);
	FRotator rotation = ParentRoom->GetActorRotation();
	FVector scale = ParentRoom->GetActorScale();

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

ETileDirection URoomTileComponent::GetTileDirection(const FIntVector& Location) const
{
	// Top-left is northwest corner
	// Bottom-right is southeast corner
	bool bIsOnLeft = Location.X == RoomLocation.X;
	bool bIsOnRight = Location.X == RoomLocation.X + RoomSize.X - 1;
	bool bIsOnTop = Location.Y == RoomLocation.Y;
	bool bIsOnBottom = Location.Y == RoomLocation.Y + RoomSize.Y - 1;

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

void URoomTileComponent::AddReplacementPhases(TArray<FRoomReplacements> AdditionalTileReplacements)
{
	RoomReplacementPhases.Insert(AdditionalTileReplacements, 0);
}