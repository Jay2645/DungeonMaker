// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "../Tiles/RoomReplacementPattern.h"
#include "DungeonMissionNode.h"
#include "DungeonFloor.h"
#include "DungeonFloorManager.generated.h"


/*
* This is a class which takes a DungeonMission and converts it into a DungeonFloor, representing
* the space in the level.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonMissionSpaceHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	//UDungeonFloorManager();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDungeonFloor> DungeonSpace;
	
	/*UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultFloorTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultWallTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultEntranceTile;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<FRoomReplacements> PreGenerationRoomReplacementPhases;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<FRoomReplacements> PostGenerationRoomReplacementPhases;*/

	// This size of this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	/*UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 FloorSize = 128;*/

	// The size of any room on this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoomSize = 32;

	/*// Any rooms on this floor which require further processing
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TSet<ADungeonRoom*> UnresolvedHooks;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	UDungeonMissionSpaceHandler* TopNeighbor = NULL;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	UDungeonMissionSpaceHandler* BottomNeighbor = NULL;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	uint8 DungeonLevel = 0;*/

public:
	void DrawDebugSpace();
	// Gets a room based on floor-space coordinates.
	// This is different than world space (where things are physically placed
	// as well as different from tile space (where tiles get placed).
	// This governs which rooms are neighboring which.
	FFloorRoom GetRoomFromFloorCoordinates(FIntVector FloorSpaceCoordinates);
	// Gets a room based on tile space coordinates.
	FFloorRoom GetRoomFromTileSpace(FIntVector TileSpaceLocation);
	// Given a vector in floor space, returns whether that location can potentially contain a room.
	bool IsLocationValid(FIntVector FloorSpaceCoordinates) const;

	TArray<FFloorRoom> GetAllNeighbors(FFloorRoom Room);

	// Creates a blank DungeonFloor array, with the specified size.
	void InitializeDungeonFloor(TArray<int32> LevelSizes);

	TArray<FDungeonFloor>& CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
		int32 SymbolCount, FRandomStream& Rng);

	/*const UDungeonTile* GetTileFromTileSpace(FIntVector TileSpaceLocation);
	void UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile);
	int XSize() const;
	int YSize() const;*/
private:
	bool PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms, 
		FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<FIntVector>& ProcessedRooms, 
		FIntVector EntranceRoom, TMap<FIntVector, FIntVector>& AllOpenRooms, 
		bool bIsTightCoupling, int32 TotalSymbolCount);

	TSet<FIntVector> GetAvailableLocations(FIntVector Location, TSet<FIntVector> IgnoredLocations = TSet<FIntVector>());
	FFloorRoom MakeFloorRoom(UDungeonMissionNode* Node, FIntVector Location,
		FRandomStream& Rng, int32 TotalSymbolCount);
	void SetRoom(FFloorRoom Room);
	void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount);

	FIntVector ConvertToFloorSpace(FIntVector TileSpaceVector) const;
	TKeyValuePair<FIntVector, FIntVector> GetOpenRoom(UDungeonMissionNode* Node,
		TMap<FIntVector, FIntVector>& AvailableRooms, FRandomStream& Rng, TSet<FIntVector>& ProcessedRooms);
	bool VerifyPathIsValid(FIntVector StartLocation);
};
