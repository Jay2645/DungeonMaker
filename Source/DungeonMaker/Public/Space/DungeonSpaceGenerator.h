// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Tiles/RoomReplacementPattern.h"
#include "Tiles/DungeonTile.h"
#include "Rooms/DungeonRoom.h"
#include "Floor/DungeonMissionSpaceHandler.h"
#include "Floor/DungeonFloorManager.h"
#include "../Mission/DungeonMissionNode.h"
#include "DungeonSpaceGenerator.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonSpaceGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDungeonSpaceGenerator();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultFloorTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultWallTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultEntranceTile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxGeneratedRooms;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	bool bDebugDungeon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRoomReplacements> PreGenerationRoomReplacementPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRoomReplacements> PostGenerationRoomReplacementPhases;

	// The size (in tiles) of this dungeon.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 DungeonSize = 128;

	// The size of a room in this dungeon, in tiles.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RoomSize = 24;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TSet<ADungeonRoom*> MissionRooms;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TSet<ADungeonRoom*> UnresolvedHooks;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> ComponentLookup;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	UDungeonMissionSpaceHandler* MissionSpaceHandler;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	int32 TotalSymbolCount;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TArray<FDungeonFloor> DungeonSpace;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	TArray<UDungeonFloorManager*> Floors;
public:	
	void CreateDungeonSpace(UDungeonMissionNode* Head, int32 SymbolCount, FRandomStream& Rng);
	void DrawDebugSpace();
	FIntVector ConvertToFloorSpace(FIntVector TileSpaceLocation);
	FFloorRoom GetRoomFromFloorCoordinates(FIntVector FloorSpaceLocation);
};
