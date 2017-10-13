// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonTile.h"
#include "DungeonRoom.h"

const float UDungeonTile::TILE_SIZE = 500.0f;

void FDungeonFloor::PlaceNewTile(FIntVector CurrentLocation, ADungeonRoom* Room, const UDungeonTile* Tile)
{
	FDungeonFloorTile newTile;
	newTile.Room = Room;
	newTile.Tile = Tile;

	TileLocations.Add(CurrentLocation, newTile);
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
	return TileLocations[CurrentLocation].Tile;
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
