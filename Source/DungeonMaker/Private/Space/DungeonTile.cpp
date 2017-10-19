// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonTile.h"
#include "DungeonRoom.h"

const float UDungeonTile::TILE_SIZE = 500.0f;


FDungeonFloor::FDungeonFloor()
{
	MaxExtents = FIntVector::ZeroValue;
	TileLocations = TMap<FIntVector, FDungeonFloorTile>();
}

void FDungeonFloor::PlaceNewTile(FIntVector CurrentLocation, ADungeonRoom* Room, const UDungeonTile* Tile)
{
	FDungeonFloorTile newTile;
	newTile.Room = Room;
	newTile.Tile = Tile;

	if (MaxExtents.X < CurrentLocation.X)
	{
		MaxExtents.X = CurrentLocation.X;
	}
	if (MaxExtents.Y < CurrentLocation.Y)
	{
		MaxExtents.Y = CurrentLocation.Y;
	}
	if (MaxExtents.Z < CurrentLocation.Z)
	{
		MaxExtents.Z = CurrentLocation.Z;
	}

	TileLocations.Add(CurrentLocation, newTile);
}


void FDungeonFloor::UpdateTile(FIntVector CurrentLocation, const UDungeonTile* NewTile)
{
	if (!TileLocations.Contains(CurrentLocation))
	{
		UE_LOG(LogDungeonGen, Warning, TEXT("Floor did not contain %d, %d, %d, but it tried to update it!"), CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
		return;
	}
	UE_LOG(LogDungeonGen, Log, TEXT("Changing tile at %d, %d, %d from %s to %s."), CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z, *TileLocations[CurrentLocation].Tile->TileID.ToString(), *NewTile->TileID.ToString());
	TileLocations[CurrentLocation].Tile = NewTile;
	TileLocations[CurrentLocation].Room->SetTileGridCoordinates(CurrentLocation, NewTile);
}

bool FDungeonFloor::TileIsWall(FIntVector Location) const
{
	if (TileLocations.Contains(Location))
	{
		return TileLocations[Location].Tile->TileType == ETileType::Wall;
	}
	else
	{
		return true;
	}
}

const UDungeonTile* FDungeonFloor::GetTileAt(FIntVector CurrentLocation)
{
	if (TileLocations.Contains(CurrentLocation))
	{
		return TileLocations[CurrentLocation].Tile;
	}
	else
	{
		return NULL;
	}
}

ADungeonRoom* FDungeonFloor::GetRoom(FIntVector CurrentLocation)
{
	if (TileLocations.Contains(CurrentLocation))
	{
		return TileLocations[CurrentLocation].Room;
	}
	else
	{
		return NULL;
	}
}

int32 FDungeonFloor::YSize() const
{
	return MaxExtents.Y;
}

int32 FDungeonFloor::XSize() const
{
	return MaxExtents.X;
}
