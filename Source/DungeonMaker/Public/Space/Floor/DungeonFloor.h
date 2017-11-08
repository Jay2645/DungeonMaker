// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "DungeonTile.h"
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
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FIntVector Location;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TSet<FIntVector> NeighboringRooms;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TSet<FIntVector> NeighboringTightlyCoupledRooms;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	FIntVector IncomingRoom;
	
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FFloorRoom> DungeonRooms;

	FDungeonFloorRow()
	{
		DungeonRooms = TArray<FFloorRoom>();
	}

	FDungeonFloorRow(int Size)
	{
		DungeonRooms.SetNum(Size);
		for (int i = 0; i < Size; i++)
		{
			DungeonRooms[i] = FFloorRoom();
		}
	}

	void Set(FFloorRoom Room, int Index)
	{
		DungeonRooms[Index] = Room;
	}

	FFloorRoom operator[] (int Index)
	{
		return DungeonRooms[Index];
	}

	int Num() const
	{
		return DungeonRooms.Num();
	}

};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonFloor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDungeonFloorRow> DungeonRooms;

	FDungeonFloor()
	{
		DungeonRooms = TArray<FDungeonFloorRow>();
	}

	FDungeonFloor(int SizeX, int SizeY)
	{
		check(SizeX >= 0);
		check(SizeY >= 0);
		DungeonRooms.SetNum(SizeY);
		for (int i = 0; i < SizeY; i++)
		{
			DungeonRooms[i] = FDungeonFloorRow(SizeX);
		}
	}

	FDungeonFloorRow& operator[] (int Index)
	{
		return DungeonRooms[Index];
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

	//FColor DrawFloor(AActor* ContextObject, FIntVector Position);

/*	void PlaceNewTile(FIntVector CurrentLocation, ADungeonRoom* Room, const UDungeonTile* Tile);
	void UpdateTile(FIntVector CurrentLocation, const UDungeonTile* NewTile);

	bool TileIsWall(FIntVector Location) const;
	const UDungeonTile* GetTileAt(FIntVector CurrentLocation);
	ADungeonRoom* GetRoom(FIntVector CurrentLocation);*/
	void DrawDungeonFloor(AActor* Context, int32 RoomSize, int32 ZOffset);
	void Set(FFloorRoom Room);
	void UpdateChildren(FIntVector A, FIntVector B);
};