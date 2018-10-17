

#include "DungeonFloorHelpers.h"

FIntVector UDungeonFloorHelpers::GetRoomTileSpacePosition(ADungeonRoom* Room)
{
	return FIntVector(0, 0, 0);
}

FRoomTile& UDungeonFloorHelpers::GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location)
{
	FDungeonSpace& dungeonSpace = Room->GetDungeon();
	return dungeonSpace.GetHighRes(Location.Z).Get(Location.X, Location.Y);
}

TSet<FRoomTile> UDungeonFloorHelpers::GetTileLocations(const UDungeonTile* Tile, ADungeonRoom* Room)
{
	return TSet<FRoomTile>();
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
	return false;
}

FTransform UDungeonFloorHelpers::CreateTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float ZLocation, float Offset)
{
	FTransform tfm = FindTileTransform(Room, RelativeLocation, Offset);
	FVector location = tfm.GetLocation();
	location.Z = ZLocation;
	tfm.SetLocation(location);
	return tfm;
}

FTransform UDungeonFloorHelpers::FindTileTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float Offset)
{
	ETileDirection direction = Room->GetTileDirectionLocalSpace(RelativeLocation);
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
