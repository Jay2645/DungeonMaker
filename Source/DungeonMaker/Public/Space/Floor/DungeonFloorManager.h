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
* 
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonFloorManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	//UDungeonFloorManager();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonFloor DungeonFloor;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultFloorTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultWallTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultEntranceTile;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<FRoomReplacements> PreGenerationRoomReplacementPhases;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<FRoomReplacements> PostGenerationRoomReplacementPhases;

	// This size of this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 FloorSize = 128;

	// The size of any room on this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoomSize = 32;

	// Any rooms on this floor which require further processing
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TSet<ADungeonRoom*> UnresolvedHooks;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	UDungeonFloorManager* TopNeighbor = NULL;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	UDungeonFloorManager* BottomNeighbor = NULL;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	uint8 DungeonLevel = 0;

public:
	void DrawDebugSpace(int32 ZOffset);
	// Gets a room based on floor-space coordinates.
	// This is different than world space (where things are physically placed
	// as well as different from tile space (where tiles get placed).
	// This governs which rooms are neighboring which.
	FFloorRoom GetRoomFromFloorCoordinates(FIntVector FloorSpaceCoordinates);
	// Gets a room based on tile space coordinates.
	FFloorRoom GetRoomFromTileSpace(FIntVector TileSpaceLocation);

	void CreateDungeonSpace(UDungeonMissionNode* Head, FIntVector StartLocation,
		int32 SymbolCount, FRandomStream& Rng);


	void InitializeDungeonFloor();

	const UDungeonTile* GetTileFromTileSpace(FIntVector TileSpaceLocation);
	void UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile);
	int XSize() const;
	int YSize() const;
private:
	// Takes a DungeonMissionNode and reserves spaces for all its tightly-coupled children (if any).
	// Note that the ParentLocation and GrandparentLocation may be modified if a tightly-coupled chain
	// doesn't "fit" in the dungeon.
	void ProcessTightlyCoupledNodes(UDungeonMissionNode* Parent,
		FFloorRoom& ParentRoom, TMap<UDungeonMissionNode*, FIntVector>& NodeLocations,
		FRandomStream& Rng, TMap<FIntVector, FIntVector>& AvailableRooms,
		FIntVector& GrandparentLocation, int32 SymbolCount, TMap<FIntVector, FFloorRoom>& ReservedRooms);
	
	// This verifies that the parent room list is valid.
	// If it isn't, it will search for a new location which will provide a valid parent room list.
	bool VerifyParentRoomList(TSet<FIntVector>& AvailableParentRooms,
		TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>>& UndoList,
		TMap<FIntVector, FIntVector>& AvailableRooms,
		TMap<UDungeonMissionNode*, TArray<TKeyValuePair<UDungeonMissionNode*, FIntVector>>>& NodesToReserve,
		TSet<UDungeonMissionNode*>& TightlyCoupledChildren, FIntVector& ParentLocation,
		FRandomStream& Rng, FIntVector& GrandparentLocation, TSet<FIntVector>& PlacedLocations,
		TArray<UDungeonMissionNode*>& NextNodes, TSet<FIntVector> AttemptedLocations);


	//void DoFloorWideTileReplacement(TArray<FRoomReplacements> ReplacementPhases, FRandomStream &Rng);
	// Gets all open room spaces neighboring a location.
	TSet<FIntVector> GetAvailableLocations(FIntVector Location, TSet<FIntVector> IgnoredLocations = TSet<FIntVector>());
	FFloorRoom MakeFloorRoom(UDungeonMissionNode* Node, FIntVector Location,
		FRandomStream& Rng, int32 TotalSymbolCount);
	void SetRoom(FFloorRoom Room);
	UDungeonFloorManager* FindFloorManagerForLocation(FIntVector Location);
	void AddChild(FIntVector RoomLocation, FIntVector RoomParent);
	void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount);

	FIntVector ConvertToFloorSpace(FIntVector TileSpaceVector) const;
};
