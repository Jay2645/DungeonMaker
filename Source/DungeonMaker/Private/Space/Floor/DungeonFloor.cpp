// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloor.h"
#include <DrawDebugHelpers.h>

void FDungeonFloor::DrawDungeonFloor(AActor* Context, int32 RoomSize, int32 ZOffset)
{
	FColor randomColor = FColor::MakeRandomColor();

	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			int32 xOffset = x;
			int32 yOffset = y;

			float offset = UDungeonTile::TILE_SIZE * RoomSize;

			FVector startingLocation(xOffset * offset, yOffset * offset, ZOffset * offset);
			FVector endingLocation(xOffset * offset, (yOffset + 1) * offset, ZOffset * offset);

			// Draw a square
			DrawDebugLine(Context->GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector((xOffset + 1) * offset, yOffset * offset, ZOffset * offset);
			DrawDebugLine(Context->GetWorld(), startingLocation, endingLocation, randomColor, true);
			startingLocation = FVector((xOffset + 1) * offset, (yOffset + 1) * offset, ZOffset * offset);
			DrawDebugLine(Context->GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector(xOffset * offset, (yOffset + 1) * offset, ZOffset * offset);
			DrawDebugLine(Context->GetWorld(), startingLocation, endingLocation, randomColor, true);

			// Label the center with the type of tile this is
			FVector midpoint((xOffset + 0.5f) * offset, (yOffset + 0.5f) * offset, (ZOffset * offset) + 100.0f);
			TSubclassOf<ADungeonRoom> room = DungeonRooms[y][x].RoomClass;
			if (room != NULL)
			{
				DrawDebugString(Context->GetWorld(), midpoint, room->GetName());
			}
		}
	}
}

void FDungeonFloor::Set(FFloorRoom Room)
{
	DungeonRooms[Room.Location.Y].Set(Room, Room.Location.X);
}

void FDungeonFloor::UpdateChildren(FIntVector A, FIntVector B)
{
	DungeonRooms[A.Y][A.X].NeighboringRooms.Add(B);
	DungeonRooms[B.Y][B.X].NeighboringRooms.Add(A);
}