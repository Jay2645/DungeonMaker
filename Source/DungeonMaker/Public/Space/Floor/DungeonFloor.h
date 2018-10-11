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
struct DUNGEONMAKER_API FDungeonFloorRow
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
	TArray<FFloorRoom> DungeonRooms;
	int RoomSize;

public:
	FDungeonFloorRow()
	{
		DungeonRooms = TArray<FFloorRoom>();
		RoomSize = 1;
	}

	FDungeonFloorRow(int Size, int MaxRoomSize)
	{
		RoomSize = MaxRoomSize;
		int realSize = RoomSize * Size;
		DungeonRooms.SetNum(realSize);
		for (int i = 0; i < DungeonRooms.Num(); i++)
		{
			DungeonRooms[i] = FFloorRoom();
		}
	}

	void Set(FFloorRoom Room, int Index)
	{
		for (int i = 0; i < Room.MaxRoomSize; i++)
		{
			DungeonRooms[Index + i] = Room;
		}
	}

	FFloorRoom& GetTileSpace(int Index)
	{
		return DungeonRooms[Index];
	}

	FFloorRoom& operator[] (int Index)
	{
		return DungeonRooms[Index * RoomSize];
	}

	int TileSizeNum() const
	{
		return DungeonRooms.Num();
	}

	int Num() const
	{
		return TileSizeNum() / RoomSize;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonFloor
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere)
	TArray<FDungeonFloorRow> DungeonRooms;

	int RoomSize;

public:
	FDungeonFloor()
	{
		DungeonRooms = TArray<FDungeonFloorRow>();
		RoomSize = 1;
	}

	FDungeonFloor(int SizeX, int SizeY, int MaxRoomSize)
	{
		check(SizeX >= 0);
		check(SizeY >= 0);

		RoomSize = MaxRoomSize;
		int realSize = RoomSize * SizeX;
		
		DungeonRooms.SetNum(realSize);
		for (int i = 0; i < DungeonRooms.Num(); i++)
		{
			DungeonRooms[i] = FDungeonFloorRow(SizeX, RoomSize);
		}
	}

	FFloorRoom& GetTileSpace(int X, int Y)
	{
		return DungeonRooms[Y].GetTileSpace(X);
	}

	FDungeonFloorRow& operator[] (int Index)
	{
		return DungeonRooms[Index * RoomSize];
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

	int XTileSize() const
	{
		if (DungeonRooms.Num() == 0)
		{
			return 0;
		}
		else
		{
			return DungeonRooms[0].TileSizeNum();
		}
	}

	int YSize() const
	{
		return YTileSize() / RoomSize;
	}

	int YTileSize() const
	{
		return DungeonRooms.Num();
	}

	void DrawDungeonFloor(AActor* Context, int32 RoomSize, int32 ZOffset);

	void Set(FFloorRoom Room)
	{
		for (int i = 0; i < Room.MaxRoomSize; i++)
		{
			DungeonRooms[Room.Location.Y * i].Set(Room, Room.Location.X);
		}
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