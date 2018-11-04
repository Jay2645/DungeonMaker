// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonFloor.h"
#include <DrawDebugHelpers.h>

const float UDungeonTile::TILE_SIZE = 500.0f;

void FLowResDungeonFloor::DrawDungeonFloor(AActor* Context, int32 ZOffset)
{
	if (Context == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Can't draw without a context actor provided!"));
		return;
	}

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
	if (Context == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Can't draw without a context actor provided!"));
		return;
	}

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

			FIntVector location = tile.RoomLocation;
			if (location.X < 0 || location.Y < 0 || location.Z < 0)
			{
				continue;
			}
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

FString FHighResDungeonFloor::RoomToString(ADungeonRoom* Room, TArray<FLowResDungeonFloor> LowResFloors)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Did not specify room to create string from!"));
		return "Not a valid room!";
	}

	FString output = "";
	FString lastOutput = "";
	for (int i = 0; i < Rows.Num(); i++)
	{
		if (lastOutput != "")
		{
			output += "\n";
		}
		lastOutput = Rows[i].RoomToString(Room, LowResFloors);
		output += lastOutput;
	}
	if (output == "")
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Could not find room %s in dungeon."), *Room->GetName());
		output = "Room not found!";
	}
	return output;
}

FString FDungeonSpace::RoomToString(ADungeonRoom* Room)
{
	if (Room == NULL)
	{
		return "No room specified!";
	}
	return HighResFloors[Room->GetRoomLocation().Z].RoomToString(Room, LowResFloors);
}

TSet<const UDungeonTile*> FHighResDungeonFloorRow::FindAllTiles(TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room /*= NULL*/)
{
	TSet<const UDungeonTile*> tiles;
	for (int i = 0; i < DungeonTiles.Num(); i++)
	{
		FIntVector roomSpace = DungeonTiles[i].RoomLocation;
		if (roomSpace.X < 0 || roomSpace.Y < 0 || roomSpace.Z < 0)
		{
			continue;
		}
		if (Room != NULL && LowResFloors[roomSpace.Z].Get(roomSpace.Y).Get(roomSpace.X).SpawnedRoom != Room)
		{
			continue;
		}
		if (DungeonTiles[i].Tile != NULL)
		{
			tiles.Add(DungeonTiles[i].Tile);
		}
	}
	return tiles;
}

TSet<FIntVector> FHighResDungeonFloorRow::GetTileLocations(const ETileType& TileType, int32 Y, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room /*= NULL*/)
{
	TSet<FIntVector> locations;
	for (int x = 0; x < DungeonTiles.Num(); x++)
	{
		FIntVector roomSpace = DungeonTiles[x].RoomLocation;
		if (roomSpace.X < 0 || roomSpace.Y < 0 || roomSpace.Z < 0)
		{
			continue;
		}
		if (Room != NULL && Room != LowResFloors[roomSpace.Z].Get(roomSpace.Y).Get(roomSpace.X).SpawnedRoom)
		{
			continue;
		}
		if (DungeonTiles[x].Tile == NULL)
		{
			continue;
		}
		if (DungeonTiles[x].Tile->TileType == TileType)
		{
			locations.Add(FIntVector(x, Y, Z));
		}
	}
	return locations;
}

TSet<FIntVector> FHighResDungeonFloorRow::GetTileLocations(const UDungeonTile* Tile, int32 Y, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room /*= NULL*/)
{
	TSet<FIntVector> locations;
	for (int x = 0; x < DungeonTiles.Num(); x++)
	{
		FIntVector roomSpace = DungeonTiles[x].RoomLocation;
		if (roomSpace.X < 0 || roomSpace.Y < 0 || roomSpace.Z < 0)
		{
			continue;
		}
		if (Room != NULL && Room != LowResFloors[roomSpace.Z].Get(roomSpace.Y).Get(roomSpace.X).SpawnedRoom)
		{
			continue;
		}
		if (DungeonTiles[x].Tile == Tile)
		{
			locations.Add(FIntVector(x, Y, Z));
		}
	}
	return locations;
}

FString FHighResDungeonFloorRow::RoomToString(ADungeonRoom* Room, TArray<FLowResDungeonFloor> LowResFloors)
{
	if (Room == NULL)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Did not specify room to create string from!"));
		return "Not a valid room!";
	}

	FString output = "";
	for (int i = 0; i < DungeonTiles.Num(); i++)
	{
		FIntVector roomSpace = DungeonTiles[i].RoomLocation;
		if (roomSpace.X < 0 || roomSpace.Y < 0 || roomSpace.Z < 0)
		{
			continue;
		}

		ADungeonRoom* spawnedRoom = LowResFloors[roomSpace.Z].Get(roomSpace.Y).Get(roomSpace.X).SpawnedRoom;
		if (spawnedRoom != Room)
		{
			if (spawnedRoom == NULL)
			{
				UE_LOG(LogSpaceGen, Warning, TEXT("Found an unspawned room at (%d, %d, %d)!"), roomSpace.X, roomSpace.Y, roomSpace.Z);
			}
			continue;
		}
		else if (DungeonTiles[i].Tile == NULL)
		{
			output += 'X';
		}
		else
		{
			output += DungeonTiles[i].Tile->TileID.ToString();
		}
	}
	return output;
}
