// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "NoExportTypes.h"

#include "DungeonTile.h"
#include "DungeonMissionNode.h"
#include "GraphNode.h"

#include "DungeonFloor.generated.h"

class ADungeonRoom;
struct FLowResDungeonFloor;

/*
* This represents a room which will get spawned on this floor.
* It also contains data about which rooms will neighbor this room.
* Once the room is spawned, it contains a reference to the spawned room
* as well as any metadata involving that room.
*/
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FFloorRoom
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TSubclassOf<ADungeonRoom> RoomClass;
	UDungeonMissionNode* RoomNode;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FIntVector Location;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TSet<FIntVector> NeighboringRooms;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TSet<FIntVector> NeighboringTightlyCoupledRooms;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FIntVector IncomingRoom;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	int32 MaxRoomSize;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	float Difficulty;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	ADungeonRoom* SpawnedRoom;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FNumberedGraphSymbol DungeonSymbol;

	FFloorRoom()
	{
		RoomClass = NULL;
		RoomNode = NULL;
		Location = FIntVector(-1, -1, -1);
		NeighboringRooms = TSet<FIntVector>();
		NeighboringTightlyCoupledRooms = TSet<FIntVector>();
		IncomingRoom = FIntVector::ZeroValue;
		Difficulty = 0.0f;
		MaxRoomSize = -1;
		SpawnedRoom = NULL;
		DungeonSymbol = FNumberedGraphSymbol();
	}

	TSet<FIntVector> GetOutgoingRooms() const
	{
		TSet<FIntVector> neighbors = TSet<FIntVector>(NeighboringRooms);
		neighbors.Append(NeighboringTightlyCoupledRooms);
		neighbors.Remove(IncomingRoom);
		return neighbors;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FRoomTile
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	const UDungeonTile* Tile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector RoomLocation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector TileLocation;

	FRoomTile()
	{
		Tile = NULL;
		RoomLocation = FIntVector(-1, -1, -1);
		TileLocation = FIntVector(-1, -1, -1);
	}

	FRoomTile(const UDungeonTile* NewTile, FIntVector NewRoomLocation, FIntVector NewTileLocation)
	{
		Tile = NewTile;
		RoomLocation = NewRoomLocation;
		TileLocation = NewTileLocation;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FHighResDungeonFloorRow
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FRoomTile> DungeonTiles;

public:
	FHighResDungeonFloorRow()
	{
		DungeonTiles = TArray<FRoomTile>();
	}

	FHighResDungeonFloorRow(int32 Size)
	{
		DungeonTiles = TArray<FRoomTile>();
		DungeonTiles.SetNum(Size);
		for (int i = 0; i < DungeonTiles.Num(); i++)
		{
			DungeonTiles[i] = FRoomTile();
		}
	}

	void Set(FRoomTile Tile, int32 MaxPossibleRoomSize)
	{
		// @TODO: This only does square rooms and does it badly
		for (int i = 0; i < MaxPossibleRoomSize; i++)
		{
			int32 location = MaxPossibleRoomSize * Tile.RoomLocation.X + i;
			Tile.TileLocation.X = location;
			DungeonTiles[location] = Tile;
		}
	}

	FRoomTile& Get(int Index)
	{
		return DungeonTiles[Index];
	}

	TSet<const UDungeonTile*> FindAllTiles(TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL);
	TSet<FIntVector> GetTileLocations(const UDungeonTile* Tile, int32 Y, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL);
	TSet<FIntVector> GetTileLocations(const ETileType& TileType, int32 Y, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL);

	bool IsValidLocation(int32 Location) const
	{
		return DungeonTiles.IsValidIndex(Location);
	}

	int32 Num() const
	{
		return DungeonTiles.Num();
	}

	FString RoomToString(ADungeonRoom* Room, TArray<FLowResDungeonFloor> LowResFloors);

	FString ToString() const
	{
		FString output;
		for (int i = 0; i < DungeonTiles.Num(); i++)
		{
			if (DungeonTiles[i].Tile == NULL)
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
};

/*
* Helper data type for FDungeonFloor.
* Represents a row of FFloorRooms.
*/
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FLowResDungeonFloorRow
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FFloorRoom> DungeonRooms;

public:
	FLowResDungeonFloorRow()
	{
		DungeonRooms = TArray<FFloorRoom>();
	}

	FLowResDungeonFloorRow(int Size)
	{
		check(Size >= 0);

		DungeonRooms.SetNum(Size);

		for (int i = 0; i < DungeonRooms.Num(); i++)
		{
			DungeonRooms[i] = FFloorRoom();
		}
	}

	FFloorRoom& Set(FFloorRoom Room, int Index)
	{
		DungeonRooms[Index] = Room;
		return DungeonRooms[Index];
	}

	FFloorRoom& Get(int Index)
	{
		return DungeonRooms[Index];
	}

	FFloorRoom& operator[] (int Index)
	{
		return Get(Index);
	}

	int Num() const
	{
		return DungeonRooms.Num();
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FHighResDungeonFloor
{
	GENERATED_BODY()
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FHighResDungeonFloorRow> Rows;

public:
	FHighResDungeonFloor()
	{
		Rows = TArray<FHighResDungeonFloorRow>();
	}

	FHighResDungeonFloor(int32 XSize, int32 YSize)
	{
		Rows = TArray<FHighResDungeonFloorRow>();
		Rows.SetNum(YSize);
		for (int i = 0; i < Rows.Num(); i++)
		{
			Rows[i] = FHighResDungeonFloorRow(XSize);
		}
	}

	void Set(FRoomTile Tile, int32 MaxPossibleRoomSize)
	{
		for (int i = 0; i < MaxPossibleRoomSize; i++)
		{
			int32 location = MaxPossibleRoomSize * Tile.RoomLocation.Y + i;
			Tile.TileLocation.Y = location;
			Rows[location].Set(Tile, MaxPossibleRoomSize);
		}
	}

	FRoomTile& Get(int X, int Y)
	{
		return Rows[Y].Get(X);
	}

	int XSize() const
	{
		if (Rows.Num() == 0)
		{
			return 0;
		}
		else
		{
			return Rows[0].Num();
		}
	}

	int YSize() const
	{
		return Rows.Num();
	}

	TSet<const UDungeonTile*> FindAllTiles(TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL)
	{
		TSet<const UDungeonTile*> tiles;
		for (int i = 0; i < Rows.Num(); i++)
		{
			tiles.Append(Rows[i].FindAllTiles(LowResFloors, Room));
		}
		return tiles;
	}

	TSet<FIntVector> GetTileLocations(const UDungeonTile* Tile, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL)
	{
		TSet<FIntVector> locations;
		for (int y = 0; y < Rows.Num(); y++)
		{
			locations.Append(Rows[y].GetTileLocations(Tile, y, Z, LowResFloors, Room));
		}
		return locations;
	}

	TSet<FIntVector> GetTileLocations(const ETileType& TileType, int32 Z, TArray<FLowResDungeonFloor>& LowResFloors, ADungeonRoom* Room = NULL)
	{
		TSet<FIntVector> locations;
		for (int y = 0; y < Rows.Num(); y++)
		{
			locations.Append(Rows[y].GetTileLocations(TileType, y, Z, LowResFloors, Room));
		}
		return locations;
	}

	bool IsValidLocation(int32 X, int32 Y) const
	{
		if (!Rows.IsValidIndex(Y))
		{
			return false;
		}
		else
		{
			return Rows[Y].IsValidLocation(X);
		}
	}

	void DrawDungeonFloor(AActor* Context, int32 ZOffset);

	FString RoomToString(ADungeonRoom* Room, TArray<FLowResDungeonFloor> LowResFloors);

	FString ToString() const
	{
		FString output;
		for (int i = 0; i < Rows.Num(); i++)
		{
			if (i > 0)
			{
				output += "\n";
			}
			output += Rows[i].ToString();
		}
		return output;
	}
};


/*
* This is a 2D array of FFloorRooms.
*
* These rooms have not been turned into tiles yet, so all this
* does is map out where rooms are in relation to other rooms.
*
* When the space has been generated, the generation algorithm
* will do its best to ensure that rooms get placed relative to
* each other according to this array. In a way, this is the
* "low-resolution" version of the "high-resolution" map that
* will be created once tiles start coming into play.
*/
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FLowResDungeonFloor
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FLowResDungeonFloorRow> DungeonRooms;

public:
	FLowResDungeonFloor()
	{
		DungeonRooms = TArray<FLowResDungeonFloorRow>();
	}

	FLowResDungeonFloor(int SizeX, int SizeY)
	{
		check(SizeX >= 0);

		DungeonRooms.SetNum(SizeY);
		for (int i = 0; i < DungeonRooms.Num(); i++)
		{
			DungeonRooms[i] = FLowResDungeonFloorRow(SizeX);
		}
	}

	FLowResDungeonFloorRow& Get(int Index)
	{
		return DungeonRooms[Index];
	}

	FLowResDungeonFloorRow& operator[] (int Index)
	{
		return Get(Index);
	}

	int XSize() const
	{
		if (DungeonRooms.Num() == 0)
		{
			return 0;
		}
		else
		{
			return DungeonRooms[0].Num();
		}
	}

	int YSize() const
	{
		return DungeonRooms.Num();
	}

	void DrawDungeonFloor(AActor* Context, int32 ZOffset);

	FFloorRoom& Set(const FFloorRoom& Room)
	{
		return Get(Room.Location.Y).Set(Room, Room.Location.X);
	}

	void SetTileSpace(FFloorRoom Room, FIntVector TileSpaceStartPosition)
	{
		for (int i = TileSpaceStartPosition.Y; i < TileSpaceStartPosition.Y + Room.MaxRoomSize; i++)
		{
			DungeonRooms[i].Set(Room, TileSpaceStartPosition.X);
		}
	}

	void UpdateChildren(FIntVector A, FIntVector B)
	{
		DungeonRooms[A.Y][A.X].NeighboringRooms.Add(B);
		DungeonRooms[B.Y][B.X].NeighboringRooms.Add(A);
	}
};

/*
* This is a graph representing an entire dungeon, from
* start to finish.
*
* However, the rooms in this dungeon have not been
* broken down into tiles -- instead, this structure
* just lists the eventual relative position of each room
* in a 3D array-like structure.
*/
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonSpace
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FLowResDungeonFloor> LowResFloors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FHighResDungeonFloor> HighResFloors;
	UPROPERTY(VisibleInstanceOnly)
	int32 RoomSize;

public:
	FDungeonSpace()
	{
		LowResFloors = TArray<FLowResDungeonFloor>();
		HighResFloors = TArray<FHighResDungeonFloor>();
	}

	FDungeonSpace(TArray<int32> LevelSizes, int32 MaxRoomSize)
	{
		LowResFloors = TArray<FLowResDungeonFloor>();
		HighResFloors = TArray<FHighResDungeonFloor>();
		LowResFloors.SetNum(LevelSizes.Num());
		HighResFloors.SetNum(LevelSizes.Num());
		for (int i = 0; i < LowResFloors.Num(); i++)
		{
			LowResFloors[i] = FLowResDungeonFloor(LevelSizes[i], LevelSizes[i]);
			HighResFloors[i] = FHighResDungeonFloor(LevelSizes[i] * MaxRoomSize, LevelSizes[i] * MaxRoomSize);
		}
		RoomSize = MaxRoomSize;
	}

	FLowResDungeonFloor& GetLowRes(int32 Index)
	{
		return LowResFloors[Index];
	}

	FHighResDungeonFloor& GetHighRes(int32 Index)
	{
		return HighResFloors[Index];
	}

	FFloorRoom& GetLowRes(const FIntVector& Location)
	{
		return GetLowRes(Location.Z).Get(Location.Y).Get(Location.X);
	}

	int LowResXSize() const
	{
		if (LowResFloors.Num() == 0)
		{
			return 0;
		}
		else
		{
			return LowResFloors[0].XSize();
		}
	}

	int LowResYSize() const
	{
		if (LowResFloors.Num() == 0)
		{
			return 0;
		}
		else
		{
			return LowResFloors[0].YSize();
		}
	}

	int Num() const
	{
		return LowResFloors.Num();
	}

	int ZSize() const
	{
		return Num();
	}

	const UDungeonTile* GetTile(const FIntVector& Location)
	{
		FHighResDungeonFloor& floor = GetHighRes(Location.Z);
		FRoomTile& tile = floor.Get(Location.X, Location.Y);
		return tile.Tile;
	}

	TSet<FIntVector> GetTileLocations(const UDungeonTile* Tile, ADungeonRoom* Room = NULL)
	{
		if (Tile == NULL)
		{
			return TSet<FIntVector>();
		}
		TSet<FIntVector> locations;
		for (int z = 0; z < HighResFloors.Num(); z++)
		{
			locations.Append(HighResFloors[z].GetTileLocations(Tile, z, LowResFloors, Room));
		}
		return locations;
	}

	TSet<FIntVector> GetTileLocations(const ETileType& TileType, ADungeonRoom* Room = NULL)
	{
		TSet<FIntVector> locations;
		for (int z = 0; z < HighResFloors.Num(); z++)
		{
			locations.Append(HighResFloors[z].GetTileLocations(TileType, z, LowResFloors, Room));
		}
		UE_LOG(LogSpaceGen, Log, TEXT("Found %d locations for %d."), locations.Num(), (int32)TileType);
		return locations;
	}

	void SetTile(const FIntVector& Location, const UDungeonTile* Tile)
	{
		if (!HighResFloors.IsValidIndex(Location.Z))
		{
			UE_LOG(LogSpaceGen, Error, TEXT("Invalid tile Z location! %d (max is %d)."), Location.Z, HighResFloors.Num() - 1);
			return;
		}
		FHighResDungeonFloor& floor = GetHighRes(Location.Z);
		if (!floor.IsValidLocation(Location.X, Location.Y))
		{
			UE_LOG(LogSpaceGen, Error, TEXT("Invalid tile X, Y location! (%d, %d), max is (%d, %d)."), Location.X, Location.Y, floor.XSize() -1, floor.YSize() - 1);
			return;
		}
		FRoomTile& tile = floor.Get(Location.X, Location.Y);
		tile.Tile = Tile;
	}

	void Set(const FFloorRoom& Room)
	{
		// @TODO: Multi-floor support
		LowResFloors[Room.Location.Z].Set(Room);
	}

	void CopyLosResToHighRes(const UDungeonTile* DefaultTile)
	{
		for (int x = 0; x < LowResXSize(); x++)
		{
			for (int y = 0; y < LowResYSize(); y++)
			{
				for (int z = 0; z < ZSize(); z++)
				{
					FIntVector location = FIntVector(x, y, z);
					FFloorRoom room = GetLowRes(location);
					if (room.MaxRoomSize <= 0)
					{
						continue;
					}
					FRoomTile roomTile = FRoomTile(DefaultTile, location, FIntVector(-1, -1, location.Z));
					HighResFloors[location.Z].Set(roomTile, room.MaxRoomSize);
				}
			}
		}
	}

	TSet<const UDungeonTile*> FindAllTiles(ADungeonRoom* Room = NULL)
	{
		TSet<const UDungeonTile*> tiles;
		for (int i = 0; i < HighResFloors.Num(); i++)
		{
			tiles.Append(HighResFloors[i].FindAllTiles(LowResFloors, Room));
		}
		return tiles;
	}

	TSet<FIntVector>& GetNeighbors(const FIntVector& Location, bool bGetTightlyCoupled)
	{
		FFloorRoom& room = GetLowRes(Location);
		if (bGetTightlyCoupled)
		{
			return room.NeighboringTightlyCoupledRooms;
		}
		else
		{
			return room.NeighboringRooms;
		}
	}

	FIntVector ConvertHighResLocationToLowRes(const FIntVector& TileSpaceVector) const
	{
		// Floor space is found by dividing by how big each room is, then rounding down
		// As an example, if the room is 24 tiles long and the location is 22x22, it
		// would return the room located at 0, 0 (which stretches from (0,0) to (23, 23)).
		FIntVector floorSpaceVector = TileSpaceVector;
		floorSpaceVector.X = FMath::FloorToInt(TileSpaceVector.X / (float)RoomSize);
		floorSpaceVector.Y = FMath::FloorToInt(TileSpaceVector.Y / (float)RoomSize);
		// Z is left alone -- it's assumed that Z in tile space and floor space are the same
		return floorSpaceVector;
	}

	bool IsValidLocation(const FIntVector& Location) const
	{
		if (Location.X < 0 || Location.Y < 0 || Location.Z < 0)
		{
			return false;
		}
		else if (Location.Z >= HighResFloors.Num())
		{
			return false;
		}
		else
		{
			return HighResFloors[Location.Z].XSize() > Location.X && HighResFloors[Location.Z].YSize() > Location.Y;
		}
	}

	FString RoomToString(ADungeonRoom* Room);

	void DrawDungeon(AActor* ContextObject)
	{
		for (int i = 0; i < HighResFloors.Num(); i++)
		{
			HighResFloors[i].DrawDungeonFloor(ContextObject, i);
		}
	}
	FIntVector GetSize() const
	{
		if (HighResFloors.Num() == 0)
		{
			return FIntVector(0, 0, 0);
		}
		return FIntVector(HighResFloors[0].XSize(), HighResFloors[0].YSize(), ZSize());
	}
};