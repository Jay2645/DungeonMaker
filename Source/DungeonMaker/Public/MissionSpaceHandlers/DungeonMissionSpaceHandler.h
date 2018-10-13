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
	TKeyValuePair<FIntVector, FIntVector> GetOpenRoom(UDungeonMissionNode* Node,
		TMap<FIntVector, FIntVector>& AvailableRooms, FRandomStream& Rng, TSet<FIntVector>& ProcessedRooms);
	void ProcessRoomNeighbors();
};
