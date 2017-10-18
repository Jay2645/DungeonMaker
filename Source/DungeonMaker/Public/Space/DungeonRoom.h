// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DungeonTile.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "../Mission/DungeonMissionSymbol.h"
#include "DungeonRoom.generated.h"

class UBSPLeaf;

UENUM(BlueprintType, meta = (Bitflags))
enum ETileDirection
{
	Center = 0,
	North = 1,
	South = 2,
	East = 4,
	West = 8
};
ENUM_CLASS_FLAGS(ETileDirection)

UCLASS(Blueprintable)
class DUNGEONMAKER_API ADungeonRoom : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ADungeonRoom();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tiles")
	FDungeonRoomMetadata RoomTiles;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Room")
	const UDungeonMissionSymbol* Symbol;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room")
	USceneComponent* DummyRoot;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Room")
	TSet<ADungeonRoom*> MissionNeighbors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	UBoxComponent* RoomTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiles")
	TArray<FRoomReplacements> RoomReplacementPhases;

	// Debug

	// Should this room be generated "standalone"?
	// This means that there's not ADungeon Actor telling it what to do.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bIsStandaloneRoomForDebug;
	// If we're being generated standalone, what seed should we use for the RNG?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	int32 DebugSeed;
	// If we're being generated standalone, what tile should we use as our default tile?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultTile;
	// How large can our debug room be?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	FIntVector DebugRoomMaxExtents;
	// What symbol should we use for our hallways?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	const UDungeonMissionSymbol* DebugHallwaySymbol;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerEnterRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerLeaveRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerEnterNeighborRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void OnRoomTilesReplaced();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void OnRoomInitialized();

public:
	// Creates a room of X by Y tiles long, populated with the specified default tile.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
		void InitializeRoom(const UDungeonTile* DefaultRoomTile,
			int32 MaxXSize, int32 MaxYSize,
			int32 XPosition, int32 YPosition, int32 ZPosition,
			const UDungeonMissionSymbol* RoomSymbol, FRandomStream &Rng,
			bool bUseRandomDimensions = true);
	
	void InitializeRoomFromPoints(const UDungeonTile* DefaultRoomTile, const UDungeonMissionSymbol* RoomSymbol, 
		FIntVector StartLocation, FIntVector EndLocation, int32 Width);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void DoTileReplacement(FDungeonFloor& DungeonFloor, FRandomStream &Rng);
	
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	TSet<ADungeonRoom*> MakeHallways(FRandomStream& Rng, const UDungeonTile* DefaultTile, const UDungeonMissionSymbol* HallwaySymbol);
	// Places this room's tile meshes in the game world.
	//UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> ComponentLookup);
	// Returns the set of all DungeonTiles used by this room.
	//UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	TSet<const UDungeonTile*> FindAllTiles();

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	TArray<FIntVector> GetTileLocations(const UDungeonTile* Tile);

	// Change the tile at the given coordinates to a specified tile.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void Set(int32 X, int32 Y, const UDungeonTile* Tile);
	// Returns the tile at the given coordinates.
	//UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	const UDungeonTile* GetTile(int32 X, int32 Y) const;
	// How big is this room on the X axis (width)?
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 XSize() const;
	// How big is this room on the Y axis (height)?
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 YSize() const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 ZSize() const;
	// Returns a string representation of this room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FString ToString() const;
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void DrawDebugRoom();
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	bool IsChangedAtRuntime() const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	ETileDirection GetTileDirection(FIntVector Location) const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector GetRoomTileSpacePosition() const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	static TSet<ADungeonRoom*> ConnectRooms(ADungeonRoom* A, ADungeonRoom* B, FRandomStream& Rng, 
		const UDungeonMissionSymbol* HallwaySymbol, const UDungeonTile* DefaultTile);
	void SetTileGridCoordinates(FIntVector currentLocation, const UDungeonTile* Tile);
};
