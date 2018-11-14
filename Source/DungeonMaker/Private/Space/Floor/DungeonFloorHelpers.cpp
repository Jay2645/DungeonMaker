

#include "DungeonFloorHelpers.h"
#include "Components/RoomTileComponent.h"

FIntVector UDungeonFloorHelpers::GetRoomTileSpacePosition(ADungeonRoom* Room)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Did not specify room!"));
		return FIntVector(-1, -1, -1);
	}
	return Room->GetRoomLocation();
}

FRoomTile& UDungeonFloorHelpers::GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location)
{
	check(Room != NULL);
	FDungeonSpace& dungeonSpace = Room->GetDungeon();
	return dungeonSpace.GetHighRes(Location.Z).Get(Location.X, Location.Y);
}

TSet<FIntVector> UDungeonFloorHelpers::GetTileLocations(const UDungeonTile* Tile, ADungeonRoom* Room)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Did not specify room to get location of!"));
		return TSet<FIntVector>();
	}
	return Room->GetDungeon().GetTileLocations(Tile, Room);
}

FIntVector UDungeonFloorHelpers::FindTileInDirection(ADungeonRoom* Room, const FIntVector& StartingLocation, const FIntVector& SearchDirection, const UDungeonTile* TileToSearchFor)
{
	FIntVector location = FIntVector(StartingLocation);
	const UDungeonTile* tile = NULL;

	while (IsLocationInRoom(Room, location) && tile != TileToSearchFor)
	{
		tile = GetTileAtLocation(Room, location).Tile;
		location += SearchDirection;
	}
	if (tile == TileToSearchFor)
	{
		return location;
	}
	else
	{
		return FIntVector(-1, -1, -1);
	}
}

bool UDungeonFloorHelpers::IsLocationInRoom(ADungeonRoom* Room, const FIntVector& StartingLocation)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Did not specify room when searching for location in room!"));
		return false;
	}
	FIntVector startLocation = GetRoomTileSpacePosition(Room);
	FIntVector endLocation = startLocation;
	endLocation.X += Room->XSize();
	endLocation.Y += Room->YSize();
	endLocation.Z += Room->ZSize();
	return	StartingLocation.X >= startLocation.X && StartingLocation.X <= endLocation.X &&
			StartingLocation.Y >= startLocation.Y && StartingLocation.Y <= endLocation.Y &&
			StartingLocation.Z >= startLocation.Z && StartingLocation.Z <= endLocation.Z;
}

FTransform UDungeonFloorHelpers::CreateTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float ZLocation, float Offset)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Did not specify room!"));
		return FTransform();
	}

	FTransform tfm = FindTileTransform(Room, RelativeLocation, Offset);
	FVector location = tfm.GetLocation();
	location.Z = ZLocation;
	tfm.SetLocation(location);
	return tfm;
}

ETileDirection UDungeonFloorHelpers::GetTileDirection(ADungeonRoom* Room, const FIntVector& Location)
{
	if (Room == NULL)
	{
		return ETileDirection::Center;
	}
	URoomTileComponent* tileHelper = Room->GetTileComponent();
	if (tileHelper == NULL)
	{
		return ETileDirection::Center;
	}
	return tileHelper->GetTileDirection(Location);
}

FTransform UDungeonFloorHelpers::FindTileTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float Offset)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Did not specify room!"));
		return FTransform();
	}
	FIntVector worldLocation = RelativeLocation + Room->GetRoomLocation();
	ETileDirection direction = GetTileDirection(Room, worldLocation);
	FVector worldSpacePosition = Room->GetActorLocation() + (FVector(RelativeLocation) * UDungeonTile::TILE_SIZE);

	FTransform tfm = FTransform(); 
	FRotator rotation = tfm.Rotator();
	switch (direction)
	{
	case ETileDirection::South:
		worldSpacePosition.X += Offset;
		worldSpacePosition.Y -= UDungeonTile::TILE_SIZE;
		tfm.SetLocation(worldSpacePosition);

		rotation.Add(0.0f, 180.0f, 0.0f);
		tfm.SetRotation(rotation.Quaternion());
		break;
	case ETileDirection::West:
		worldSpacePosition.X += UDungeonTile::TILE_SIZE;
		worldSpacePosition.Y += Offset;
		tfm.SetLocation(worldSpacePosition);

		rotation.Add(0.0f, 270.0f, 0.0f);
		tfm.SetRotation(rotation.Quaternion());
		break;
	case ETileDirection::East:
		worldSpacePosition.Y -= Offset;
		tfm.SetLocation(worldSpacePosition);

		rotation.Add(0.0f, 90.0f, 0.0f);
		tfm.SetRotation(rotation.Quaternion());
		break;
	case ETileDirection::North:
		worldSpacePosition.X += Offset;
		tfm.SetLocation(worldSpacePosition);
		break;
	default:
		tfm.SetLocation(worldSpacePosition);
		break;
	}

	return tfm;
}

void UDungeonFloorHelpers::DrawDungeon(AActor* ContextObject, FDungeonSpace DungeonSpace)
{
	DungeonSpace.DrawDungeon(ContextObject);
}

FString UDungeonFloorHelpers::FloorToTileString(FDungeonSpace DungeonSpace, uint8 FloorNum)
{
	return DungeonSpace.GetHighRes((int32)FloorNum).ToString();
}

FString UDungeonFloorHelpers::RoomToTileString(FDungeonSpace DungeonSpace, ADungeonRoom* Room)
{
	return DungeonSpace.RoomToString(Room);
}

bool UDungeonFloorHelpers::AreRoomsLeftRightAdjacent(ADungeonRoom* First, ADungeonRoom* Second)
{
	if (First == NULL || Second == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Rooms are not adjacent because one is null!"));
		return false;
	}

	FIntVector firstMinExtent = GetRoomTileSpacePosition(First);
	FIntVector secondMinExtent = GetRoomTileSpacePosition(Second);
	if (firstMinExtent.Z != secondMinExtent.Z)
	{
		// Not on the same level
		// @TODO: Maybe check if we can add stairs/ladder to connect?
		UE_LOG(LogSpaceGen, Log, TEXT("Rooms are not adjacent because they are on different floors!"));
		return false;
	}
	FIntVector firstMaxExtent = firstMinExtent + First->GetRoomSize();
	FIntVector secondMaxExtent = secondMinExtent + Second->GetRoomSize();

	// We want to make sure that we only say we're adjacent if we can replace 2 or fewer tiles
	// to get a path between rooms.
	// For right now, we assume all rooms are squares. @TODO: Fix this
	if (firstMaxExtent.X == secondMinExtent.X || secondMaxExtent.X == firstMinExtent.X)
	{
		// Second is to the right of first or first is to the right of second
		UE_LOG(LogSpaceGen, Log, TEXT("Rooms may be left-right adjacent! First: (%d, %d, %d) - (%d, %d, %d); Second: (%d, %d %d) - (%d, %d, %d)."), firstMinExtent.X, firstMinExtent.Y, firstMinExtent.Z, firstMaxExtent.X, firstMaxExtent.Y, firstMaxExtent.Z, secondMinExtent.X, secondMinExtent.Y, secondMinExtent.Z, secondMaxExtent.X, secondMaxExtent.Y, secondMaxExtent.Z);

		// Adjust for the walls
		firstMinExtent.Y += 1;
		secondMinExtent.Y += 1;
		firstMaxExtent.Y -= 1;
		secondMaxExtent.Y -= 1;

		// True if the minimum extent of one is between the extents of the other
		return firstMinExtent.Y >= secondMinExtent.Y && firstMinExtent.Y <= secondMaxExtent.Y ||
			secondMinExtent.Y >= firstMinExtent.Y && secondMinExtent.Y <= firstMaxExtent.Y;
	}
	else
	{
		return false;
	}
}

bool UDungeonFloorHelpers::AreRoomsTopDownAdjacent(ADungeonRoom* First, ADungeonRoom* Second)
{
	if (First == NULL || Second == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Rooms are not adjacent because one is null!"));
		return false;
	}

	FIntVector firstMinExtent = GetRoomTileSpacePosition(First);
	FIntVector secondMinExtent = GetRoomTileSpacePosition(Second);
	if (firstMinExtent.Z != secondMinExtent.Z)
	{
		// Not on the same level
		// @TODO: Maybe check if we can add stairs/ladder to connect?
		UE_LOG(LogSpaceGen, Log, TEXT("Rooms are not adjacent because they are on different floors!"));
		return false;
	}
	FIntVector firstMaxExtent = firstMinExtent + First->GetRoomSize();
	FIntVector secondMaxExtent = secondMinExtent + Second->GetRoomSize();

	// We want to make sure that we only say we're adjacent if we can replace 2 or fewer tiles
	// to get a path between rooms.
	// For right now, we assume all rooms are squares. @TODO: Fix this
	if (firstMaxExtent.Y == secondMinExtent.Y || secondMaxExtent.Y == firstMinExtent.Y)
	{
		// Second is below first or first is below second
		UE_LOG(LogSpaceGen, Log, TEXT("Rooms may be top-down adjacent! First: (%d, %d, %d) - (%d, %d, %d); Second: (%d, %d %d) - (%d, %d, %d)."), firstMinExtent.X, firstMinExtent.Y, firstMinExtent.Z, firstMaxExtent.X, firstMaxExtent.Y, firstMaxExtent.Z, secondMinExtent.X, secondMinExtent.Y, secondMinExtent.Z, secondMaxExtent.X, secondMaxExtent.Y, secondMaxExtent.Z);

		// Adjust for the walls
		firstMinExtent.X += 1;
		secondMinExtent.X += 1;
		firstMaxExtent.X -= 1;
		secondMaxExtent.X -= 1;

		// True if the minimum extent of one is between the extents of the other
		return firstMinExtent.X >= secondMinExtent.X && firstMinExtent.X <= secondMaxExtent.X ||
			secondMinExtent.X >= firstMinExtent.X && secondMinExtent.X <= firstMaxExtent.X;
	}
	else
	{
		return false;
	}
}

bool UDungeonFloorHelpers::AreRoomsAdjacent(ADungeonRoom* First, ADungeonRoom* Second)
{
	return AreRoomsLeftRightAdjacent(First, Second) || AreRoomsTopDownAdjacent(First, Second);
}

bool UDungeonFloorHelpers::AreFloorRoomsAdjacent(const FFloorRoom& First, const FFloorRoom& Second)
{
	FIntVector firstLocation = First.Location;
	FIntVector secondLocation = Second.Location;
	FIntVector distance = firstLocation - secondLocation;
	// Get absolute value of the distance
	distance.X = FMath::Abs(distance.X);
	distance.Y = FMath::Abs(distance.Y);
	distance.Z = FMath::Abs(distance.Z);

	// Only adjacent if we are directly next to another room.
	// Don't count diagonals, and if the rooms are in the same place they're not adjacent
	return distance.X == 1 && distance.Y == 0 && distance.Z == 0 ||
		distance.X == 0 && distance.Y == 1 && distance.Z == 0 ||
		distance.X == 0 && distance.Y == 0 && distance.Z == 1;
}
