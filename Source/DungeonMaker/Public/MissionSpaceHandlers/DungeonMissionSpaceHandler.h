// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "../Tiles/RoomReplacementPattern.h"
#include "DungeonMissionNode.h"
#include "DungeonFloor.h"
#include "DungeonMissionSpaceHandler.generated.h"

class UDungeonSpaceGenerator;

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FRoomPairing
{
	GENERATED_BODY()

public:
	// The room that leads into the next room
	// Should have already been processed
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FIntVector ParentRoom;
	// The room we've selected as a child room
	// Should not be processed yet
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FIntVector ChildRoom;

	FRoomPairing()
	{
		ParentRoom = FIntVector(-1, -1, -1);
		ChildRoom = FIntVector(-1, -1, -1);
	}

	FRoomPairing(FIntVector Child, FIntVector Parent)
	{
		ChildRoom = Child;
		ParentRoom = Parent;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FMissionSpaceHelper
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FRandomStream& Rng;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TSet<UDungeonMissionNode*> ProcessedNodes;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TSet<FIntVector> ProcessedRooms;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TMap<FIntVector, FIntVector> OpenRooms;

	FMissionSpaceHelper() : Rng(*(new FRandomStream()))
	{
		UE_LOG(LogMissionGen, Log, TEXT("Default MissionSpaceHelper constructor!"));
		Rng.Initialize(0);
	}

	FMissionSpaceHelper(FRandomStream& RandomNumbers, FIntVector StartLocation) : Rng(RandomNumbers)
	{
		OpenRooms.Add(StartLocation, FIntVector(-1, -1, -1));
	}

	FMissionSpaceHelper& operator= (const FMissionSpaceHelper& Helper)
	{
		Rng = Helper.Rng;
		ProcessedNodes.Append(Helper.ProcessedNodes.Array());
		ProcessedRooms.Append(Helper.ProcessedRooms.Array());
		OpenRooms.Append(Helper.OpenRooms);
		return *this;
	}

	bool HasProcessed(UDungeonMissionNode* Node) const
	{
		return ProcessedNodes.Contains(Node);
	}

	bool HasProcessed(FIntVector RoomLocation) const
	{
		return ProcessedRooms.Contains(RoomLocation);
	}

	bool HasOpenRooms() const
	{
		return OpenRooms.Num() > 0;
	}

	TSet<FIntVector>& GetProcessedRooms()
	{
		return ProcessedRooms;
	}

	void MarkAsProcessed(FIntVector RoomLocation)
	{
		ProcessedRooms.Add(RoomLocation);
	}

	void MarkAsProcessed(UDungeonMissionNode* Node)
	{
		ProcessedNodes.Add(Node);
	}

	void MarkAsUnprocessed(UDungeonMissionNode* Node)
	{
		ProcessedNodes.Remove(Node);
	}

	void AddOpenRooms(TMap<FIntVector, FIntVector> MoreOpenRooms)
	{
		OpenRooms.Append(MoreOpenRooms);
	}
};

/*
* This is a class which takes a DungeonMission and converts it into a DungeonFloor, representing
* the space in the level.
*/
UCLASS(Abstract)
class DUNGEONMAKER_API UDungeonMissionSpaceHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonSpaceGenerator* DungeonSpaceGenerator;

	// The max size of any room on this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoomSize = 32;

private:
	int32 RoomCount = 0;

public:
	void DrawDebugSpace();
	// Creates a blank DungeonFloor array, with the specified size.
	void InitializeDungeonFloor(UDungeonSpaceGenerator* SpaceGenerator, TArray<int32> LevelSizes);

	bool CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
		int32 SymbolCount, FRandomStream& Rng);

protected:
	TSet<FIntVector> GetAvailableLocations(FIntVector Location, TSet<FIntVector> IgnoredLocations = TSet<FIntVector>());
	FFloorRoom MakeFloorRoom(UDungeonMissionNode* Node, FIntVector Location,
		FRandomStream& Rng, int32 TotalSymbolCount);
	void SetRoom(FFloorRoom Room);
	virtual void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount);
	void ProcessRoomNeighbors();
};
