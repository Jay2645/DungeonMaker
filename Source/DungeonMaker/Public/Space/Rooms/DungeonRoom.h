// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DungeonTile.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "../Mission/DungeonMissionSymbol.h"
#include "DungeonFloorManager.h"
#include "DungeonRoom.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSpaceGen, Log, All);

UENUM(BlueprintType)
enum class ETileDirection : uint8
{
	Center,
	North,
	South,
	East,
	West,
	Northeast,
	Northwest,
	Southeast,
	Southwest
};


USTRUCT(BlueprintType)
struct FScatterObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> ScatterObject;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SelectionChance;

	FScatterObject()
	{
		ScatterObject = NULL;
		SelectionChance = 1.0f;
	}
};

USTRUCT(BlueprintType)
struct FGroundScatter
{
	GENERATED_BODY()
public:
	// A list of all objects we should scatter on this tile.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FScatterObject> ScatterObjects;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTransform ObjectOffset;
	// Which edges of the room this ground scatter is valid at.
	// Center means anywhere which is not an edge.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<ETileDirection> AllowedDirections;
	// How far away this should be from the edge of any room.
	// Bear in mind that this doesn't make sense with any allowed directions other than Center.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FIntVector EdgeOffset;

	// Whether we should be able to place this adjacent to next rooms.
	// Next rooms are determined by our current mission.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlaceAdjacentToNextRooms;
	// Whether we should be able to place this adjacent to previous rooms.
	// Next rooms are determined by our current mission.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlaceAdjacentToPriorRooms;

	// Should we keep track of how many objects we place at all, or should we place as many as we want?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseRandomCount;
	// Should we place this object randomly in the room?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseRandomLocation;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bConformToGrid;
	// What's the minimum count of objects we should place?
	// Only used if we're using a random count.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MinCount;
	// What's the maximum count of objects we should place?
	// Only used if we're using a random count.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MaxCount;
	// Skip every n tiles when placing this.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 SkipTiles;
	
	FGroundScatter()
	{
		AllowedDirections.Add(ETileDirection::Center);
		bUseRandomCount = false;
		bUseRandomLocation = true;
		bConformToGrid = true;
		bPlaceAdjacentToNextRooms = true;
		bPlaceAdjacentToPriorRooms = true;
		MinCount = 0;
		MaxCount = 255;
		SkipTiles = 0;
		EdgeOffset = FIntVector(0, 0, 0);
	}
};

USTRUCT(BlueprintType)
struct FGroundScatterSet
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FGroundScatter> GroundScatter;
};

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	UBoxComponent* RoomTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiles")
	TArray<FRoomReplacements> RoomReplacementPhases;
	// A list of actors that get scattered throughout the room
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	TMap<const UDungeonTile*, FGroundScatterSet> GroundScatter;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	float RoomDifficulty;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	uint8 RoomLevel;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	FFloorRoom RoomMetadata;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	UDungeonFloorManager* DungeonFloor;

	// Debug

	// Should this room be generated "standalone"?
	// This means that there's not ADungeon Actor telling it what to do.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bIsStandaloneRoomForDebug;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugTiles;
	// If we're being generated standalone, what seed should we use for the RNG?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	int32 DebugSeed;
	// If we're being generated standalone, what tile should we use as our default tile?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultFloorTile;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultWallTile;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultEntranceTile;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	FIntVector DebugRoomMaxExtents;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
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
		void InitializeRoom(const UDungeonTile* DefaultFloorTile,
			const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
			float Difficulty, int32 MaxXSize, int32 MaxYSize,
			int32 XPosition, int32 YPosition, int32 ZPosition,
			const UDungeonMissionSymbol* RoomSymbol, FRandomStream &Rng,
			bool bUseRandomDimensions = true, bool bIsDeterminedFromPoints = false);
	
	/*void InitializeRoomFromPoints(const UDungeonTile* DefaultFloorTile,
		const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
		const UDungeonMissionSymbol* RoomSymbol, FIntVector StartLocation, FIntVector EndLocation,
		int32 Width, bool bIsJoinedToHallway = false);*/

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void DoTileReplacement(FRandomStream &Rng);

	/*UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void UpdateDungeonFloor(FDungeonFloor& DungeonFloor);
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	TSet<ADungeonRoom*> MakeHallways(FRandomStream& Rng, const UDungeonTile* DefaultFloorTile, 
		const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
		const UDungeonMissionSymbol* HallwaySymbol, FDungeonFloor& DungeonFloor);*/
	// Places this room's tile meshes in the game world.
	//UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*>& ComponentLookup, 
		FRandomStream& Rng);
	void DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations, 
		FRandomStream& Rng);
	// Gets the transform for a tile from that tile's position in local space ((0,0,0) to Room Bounds).
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransform(const FIntVector& LocalLocation) const;
	// Gets the transform for a tile from that tile's position in world space.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const;
	// Returns the set of all DungeonTiles used by this room.
	//UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	TSet<const UDungeonTile*> FindAllTiles();

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	TArray<FIntVector> GetTileLocations(const UDungeonTile* Tile);

	//UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	//static bool PathIsClear(FIntVector StartLocation, FIntVector EndLocation, 
	//	int32 SweepWidth, FDungeonFloor& DungeonFloor);

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
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnRoomGenerationComplete();

	/*UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
		static TSet<ADungeonRoom*> ConnectRooms(ADungeonRoom* A, ADungeonRoom* B, FRandomStream& Rng,
			const UDungeonMissionSymbol* HallwaySymbol, FDungeonFloor& DungeonFloor,
			const UDungeonTile* DefaultFloorTile, const UDungeonTile* DefaultWallTile, 
			const UDungeonTile* DefaultEntranceTile);*/
	void SetTileGridCoordinates(FIntVector currentLocation, const UDungeonTile* Tile);
protected:
	/*static FIntVector FindClosestVertex(const FIntVector& Source1, const FIntVector& Source2, 
		const FIntVector& Destination);*/
};
