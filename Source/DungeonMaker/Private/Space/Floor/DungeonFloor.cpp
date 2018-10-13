// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloor.h"
#include <DrawDebugHelpers.h>

void FLowResDungeonFloor::DrawDungeonFloor(AActor* Context, int32 ZOffset)
{
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			FColor randomColor = FColor::MakeRandomColor();

			int32 xOffset = x;
			int32 yOffset = y;

			float offset = UDungeonTile::TILE_SIZE * DungeonRooms[y][x].MaxRoomSize;

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
				FString symbolDescription = DungeonRooms[y][x].DungeonSymbol.GetSymbolDescription();
				symbolDescription += " (";
				symbolDescription.AppendInt(DungeonRooms[y][x].DungeonSymbol.SymbolID);
				symbolDescription += ")";
				DrawDebugString(Context->GetWorld(), midpoint, symbolDescription);
			}

			for (FIntVector neighborLocation : DungeonRooms[y][x].NeighboringRooms)
			{
				FVector otherMidpoint = FVector((neighborLocation.X + 0.5f) * offset, (neighborLocation.Y + 0.5f) * offset, neighborLocation.Z * offset);
				DrawDebugLine(Context->GetWorld(), midpoint, otherMidpoint, randomColor, true);
			}
			for (FIntVector neighborLocation : DungeonRooms[y][x].NeighboringTightlyCoupledRooms)
			{
				FVector otherMidpoint = FVector((neighborLocation.X + 0.5f) * offset, (neighborLocation.Y + 0.5f) * offset, neighborLocation.Z * offset);
				DrawDebugLine(Context->GetWorld(), midpoint, otherMidpoint, randomColor, true);
				DrawDebugLine(Context->GetWorld(), midpoint + FVector(0.0f, 0.0f, 50.0f), otherMidpoint + FVector(0.0f, 0.0f, 50.0f), randomColor, true);
			}
		}
	}
}

void FHighResDungeonFloor::DrawDungeonFloor(AActor* Context, int32 ZOffset)
{
	TMap<FIntVector, FColor> randomColorLookup;
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			FRoomTile tile = Get(x, y);
			if (tile.Tile == NULL)
			{
				continue;
			}

			FIntVector location = tile.Room.Location;
			FColor randomColor;
			if (randomColorLookup.Contains(location))
			{
				randomColor = randomColorLookup[location];
			}
			else
			{
				randomColor = FColor::MakeRandomColor();
				randomColorLookup.Add(location, randomColor);
			}

			int32 xOffset = x;
			int32 yOffset = y;

			float offset = UDungeonTile::TILE_SIZE;

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
		}
	}
}