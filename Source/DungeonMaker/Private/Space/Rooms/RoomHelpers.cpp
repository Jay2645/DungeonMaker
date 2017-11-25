// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomHelpers.h"
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