// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "../Tiles/RoomReplacementPattern.h"
#include "DungeonMissionNode.h"
#include "DungeonFloor.h"
#include "GroundScatterManager.h"
#include "DungeonFloorManager.generated.h"

class UDungeonSpaceGenerator;

/*
* This class manages spawning rooms and replacing tiles on a DungeonFloor.
*
* In general, it does tile replacements and handles the actual generation of
* the dungeon itself.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonFloorManager : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	//UDungeonFloorManager();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonSpaceGenerator* DungeonSpaceGenerator;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultFloorTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultWallTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultEntranceTile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRoomReplacements> PreGenerationRoomReplacementPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRoomReplacements> PostGenerationRoomReplacementPhases;

	// The max size of any room on this floor, in tile space.
	// The total number of rooms this floor will have is determined by
	// ceil(sqrt(FloorSize / RoomSize))^2.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoomSize = 32;

	// Any rooms on this floor which require further processing
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TSet<ADungeonRoom*> UnresolvedHooks;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	int32 DungeonLevel = 0;

public:
	void InitializeFloorManager(UDungeonSpaceGenerator* SpaceGenerator, int32 Level);
	void SpawnRooms(FRandomStream& Rng, const FGroundScatterPairing& GlobalGroundScatter);
	void DrawDebugSpace();
	// Gets a room based on tile space coordinates.

	const UDungeonTile* GetTileFromTileSpace(FIntVector TileSpaceLocation);
	void UpdateTileFromTileSpace(FIntVector TileSpaceLocation, const UDungeonTile* NewTile);
	void SpawnRoomMeshes(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
		FRandomStream& Rng);
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	int XSize() const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	int YSize() const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	TSet<FIntVector> GetAllTilesOfType(ETileType Type);

	FFloorRoom GetRoomFromTileSpace(const FIntVector& TileSpaceLocation);
private:
	ADungeonRoom* CreateRoom(const FFloorRoom& Room, FRandomStream& Rng, 
		const FGroundScatterPairing& GlobalGroundScatter);
	// Returns a COPY of the DungeonFloor we represent.
	FLowResDungeonFloor GetDungeonFloor() const;
	void CreateEntrances(ADungeonRoom* Room, FRandomStream& Rng);
	void DoTileReplacement(ADungeonRoom* Room, FRandomStream& Rng);
	void DoFloorWideTileReplacement(TArray<FRoomReplacements> ReplacementPhases, FRandomStream &Rng);
};
