// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonTile.h"
#include "DungeonRoom.h"
#include <DrawDebugHelpers.h>

const float UDungeonTile::TILE_SIZE = 500.0f;

FColor FDungeonRoomMetadata::DrawRoom(AActor* ContextObject, FIntVector Position)
{
	int32 xPosition = Position.X;
	int32 yPosition = Position.Y;
	int32 zPosition = Position.Z;

	FColor randomColor = FColor::MakeRandomColor();

	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			int32 xOffset = x + xPosition;
			int32 yOffset = y + yPosition;
			FVector startingLocation(xOffset * UDungeonTile::TILE_SIZE, yOffset * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			FVector endingLocation(xOffset * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);

			// Draw a square
			DrawDebugLine(ContextObject->GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector((xOffset + 1) * UDungeonTile::TILE_SIZE, yOffset * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(ContextObject->GetWorld(), startingLocation, endingLocation, randomColor, true);
			startingLocation = FVector((xOffset + 1) * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(ContextObject->GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector(xOffset * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(ContextObject->GetWorld(), startingLocation, endingLocation, randomColor, true);

			// Label the center with the type of tile this is
			FVector midpoint((xOffset + 0.5f) * UDungeonTile::TILE_SIZE, (yOffset + 0.5f) * UDungeonTile::TILE_SIZE, (zPosition * UDungeonTile::TILE_SIZE) + 100.0f);
			const UDungeonTile* tile = DungeonRows[y].DungeonTiles[x];
			if (tile != NULL)
			{
				DrawDebugString(ContextObject->GetWorld(), midpoint, tile->TileID.ToString());
			}
		}
	}
	return randomColor;
}

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
		if (NewTile == NULL)
		{
			return;
		}
		ADungeonRoom* room = NULL;
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				FIntVector location = CurrentLocation;
				location.X += x;
				location.Y += y;
				if (TileLocations.Contains(location))
				{
					room = TileLocations[location].Room;
					break;
				}
			}
		}
		if (room == NULL)
		{
			UE_LOG(LogDungeonGen, Warning, TEXT("Floor did not contain %d, %d, %d, but it tried to update it!"), CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
			return;
		}
		FDungeonFloorTile dungeonFloor;
		dungeonFloor.Room = room;
		dungeonFloor.Tile = NewTile;
		TileLocations.Add(CurrentLocation, dungeonFloor);
	}
	else
	{
		//UE_LOG(LogDungeonGen, Log, TEXT("Changing tile at %d, %d, %d from %s to %s."), CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z, *TileLocations[CurrentLocation].Tile->TileID.ToString(), *NewTile->TileID.ToString());
		TileLocations[CurrentLocation].Tile = NewTile;
		TileLocations[CurrentLocation].Room->SetTileGridCoordinates(CurrentLocation, NewTile);
	}
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


FDungeonRoomMetadata FDungeonFloor::ToRoom()
{
	FDungeonRoomMetadata room = FDungeonRoomMetadata(XSize(), YSize());
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			FIntVector location = FIntVector(x, y, 0);
			room.Set(x, y, GetTileAt(location));
		}
	}
	return room;
}

void FDungeonFloor::DrawDungeonFloor(AActor* Context, int32 ZOffset)
{
	FDungeonRoomMetadata room = ToRoom();
	room.DrawRoom(Context, FIntVector(0, 0, ZOffset));
}
