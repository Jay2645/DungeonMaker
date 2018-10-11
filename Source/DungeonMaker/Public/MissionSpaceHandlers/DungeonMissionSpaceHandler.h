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
	// Gets a room based on floor-space coordinates.
	// This is different than world space (where things are physically placed
	// as well as different from tile space (where tiles get placed).
	// This governs which rooms are neighboring which.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FFloorRoom GetRoomFromFloorCoordinates(FIntVector FloorSpaceCoordinates);
	// Gets a room based on tile space coordinates.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FFloorRoom GetRoomFromTileSpace(FIntVector TileSpaceLocation);
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector ConvertToFloorSpace(FIntVector TileSpaceVector) const;
	// Given a vector in floor space, returns whether that location can potentially contain a room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	bool IsLocationValid(FIntVector FloorSpaceCoordinates) const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	TArray<FFloorRoom> GetAllNeighbors(FFloorRoom Room);

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
