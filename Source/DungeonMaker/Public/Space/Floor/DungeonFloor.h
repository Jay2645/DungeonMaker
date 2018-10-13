// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"

#include "DungeonTile.h"
#include "DungeonMissionNode.h"
#include "GraphNode.h"

#include "DungeonFloor.generated.h"

class ADungeonRoom;

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
		Location = FIntVector::ZeroValue;
		NeighboringRooms = TSet<FIntVector>();
		NeighboringTightlyCoupledRooms = TSet<FIntVector>();
		IncomingRoom = FIntVector::ZeroValue;
		Difficulty = 0.0f;
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
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	const UDungeonTile* Tile;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FFloorRoom Room;

	FRoomTile()
	{
		Tile = NULL;
		Room = FFloorRoom();
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FHighResDungeonFloorRow
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
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

	void Set(FRoomTile Tile)
	{
		FFloorRoom room = Tile.Room;
		for (int i = 0; i < room.MaxRoomSize; i++)
		{
			DungeonTiles[room.MaxRoomSize * room.Location.X + i] = Tile;
		}
	}

	FRoomTile& Get(int Index)
	{
		return DungeonTiles[Index];
	}

	int32 Num() const
	{
		return DungeonTiles.Num();
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
	UPROPERTY(EditAnywhere)
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

	void Set(FFloorRoom Room, int Index)
	{
		DungeonRooms[Index] = Room;
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

	void Set(FRoomTile Tile)
	{
		FFloorRoom room = Tile.Room;
		for (int i = 0; i < room.MaxRoomSize; i++)
		{
			Rows[room.MaxRoomSize * room.Location.Y + i].Set(Tile);
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

	void DrawDungeonFloor(AActor* Context, int32 ZOffset);
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
	UPROPERTY(EditAnywhere)
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

	void Set(FFloorRoom Room)
	{
		Get(Room.Location.Y).Set(Room, Room.Location.X);
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
	UPROPERTY(EditAnywhere)
	TArray<FLowResDungeonFloor> LowResFloors;
	UPROPERTY(EditAnywhere)
	TArray<FHighResDungeonFloor> HighResFloors;
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

	void Set(FFloorRoom Room, const UDungeonTile* DefaultTile)
	{
		// @TODO: Multi-room support
		LowResFloors[Room.Location.Z].Set(Room);
		
		FRoomTile roomTile = FRoomTile();
		roomTile.Tile = DefaultTile;
		roomTile.Room = Room;
		HighResFloors[Room.Location.Z].Set(roomTile);
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
};