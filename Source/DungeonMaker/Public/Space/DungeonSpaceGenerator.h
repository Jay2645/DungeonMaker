// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceMeshActor.h"
#include "Tiles/RoomReplacementPattern.h"
#include "Tiles/DungeonTile.h"
#include "Rooms/DungeonRoom.h"
#include "DungeonMissionSpaceHandler.h"
#include "Floor/DungeonFloorManager.h"
#include "../Mission/DungeonMissionNode.h"
#include "GroundScatterManager.h"
#include "DungeonSpaceGenerator.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonSpaceGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDungeonSpaceGenerator();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles")
	const UDungeonTile* DefaultFloorTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles")
	const UDungeonTile* DefaultWallTile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tiles")
	const UDungeonTile* DefaultEntranceTile;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	TSubclassOf<UDungeonMissionSpaceHandler> MissionToSpaceHandlerClass;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	int32 MaxGeneratedRooms;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bDebugDungeon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement")
	TArray<FRoomReplacements> PreGenerationRoomReplacementPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement")
	TArray<FRoomReplacements> PostGenerationRoomReplacementPhases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FGroundScatterPairing GlobalGroundScatter;

	// The size (in tiles) of this dungeon.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	int32 DungeonSize = 128;

	// The size of a room in this dungeon, in tiles.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	int32 RoomSize = 24;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	TSet<ADungeonRoom*> MissionRooms;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	TSet<ADungeonRoom*> UnresolvedHooks;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Tiles")
	TMap<const UDungeonTile*, ASpaceMeshActor*> FloorComponentLookup;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Tiles")
	TMap<const UDungeonTile*, ASpaceMeshActor*> CeilingComponentLookup;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	UDungeonMissionSpaceHandler* MissionSpaceHandler;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	int32 TotalSymbolCount;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	FDungeonSpace DungeonSpace;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Dungeon")
	TArray<UDungeonFloorManager*> Floors;
public:	
	bool CreateDungeonSpace(UDungeonMissionNode* Head, int32 SymbolCount, FRandomStream& Rng);

	bool IsLocationValid(FIntVector FloorSpaceCoordinates);
	TArray<FFloorRoom> GetAllNeighbors(FFloorRoom Room);
	void SetRoom(FFloorRoom Room);

	FFloorRoom GetRoomFromFloorCoordinates(const FIntVector& FloorSpaceCoordinates);
	FFloorRoom GetRoomFromTileSpace(const FIntVector& TileSpaceLocation);
	FIntVector ConvertToFloorSpace(const FIntVector& TileSpaceVector) const;

	void DrawDebugSpace();
};
