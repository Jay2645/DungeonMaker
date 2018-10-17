// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DungeonMaker.h"
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DungeonTile.h"
#include "SpaceMeshActor.h"
#include "Components/BoxComponent.h"
#include "../Mission/DungeonMissionSymbol.h"
#include "DungeonFloorManager.h"
#include "GameplayTagContainer.h"
#include "DungeonRoom.generated.h"

class UDungeonSpaceGenerator;
class UGroundScatterManager;

UCLASS(Blueprintable)
class DUNGEONMAKER_API ADungeonRoom : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ADungeonRoom();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	USceneComponent* DummyRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UBoxComponent* RoomTrigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UBoxComponent* NorthEntranceTrigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UBoxComponent* SouthEntranceTrigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UBoxComponent* WestEntranceTrigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UBoxComponent* EastEntranceTrigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UGroundScatterManager* GroundScatter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UDungeonFloorManager* DungeonFloor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UDungeonSpaceGenerator* DungeonSpace;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiles")
	FDungeonRoomMetadata RoomTiles;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Room")
	const UDungeonMissionSymbol* Symbol;

	// An optional icon for this room.
	// By default, we don't do anything with it, but this could be used
	// as an icon on a minimap or some such.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	UTexture2D* RoomIcon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FIntVector MinRoomSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	uint8 MaxRoomHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float RoomDifficultyModifier;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	uint8 ActualRoomHeight;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	FFloorRoom RoomMetadata;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiles")
	TArray<FRoomReplacements> RoomReplacementPhases;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TSet<FIntVector> EntranceLocations;
	// A list of actors that get scattered throughout the room
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	uint8 RoomLevel;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	TSet<ADungeonRoom*> AllNeighbors;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	TSet<ADungeonRoom*> TightlyCoupledNeighbors;

	// These all determine which tiles we have selected, to ensure consistency
	// throughout the room
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, int32> FloorTileMeshSelections;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, int32> CeilingTileMeshSelections;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, FDungeonTileInteractionOptions> InteractionOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FGameplayTagContainer RoomTags;


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
	UFUNCTION()
	virtual void OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnEndTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	virtual void OnBeginEntranceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerEnterRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerEnterRoomEntrance();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerLeaveRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnPlayerEnterNeighborRoom();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void OnPreRoomTilesReplaced();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void OnRoomTilesReplaced();
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnRoomInitialized();

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "World Generation|Dungeon Generation|Rooms")
	void OnRoomGenerationComplete();

public:
	// Creates a room of X by Y tiles long, populated with the specified default tile.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void InitializeRoom(UDungeonSpaceGenerator* SpaceGenerator, const UDungeonTile* DefaultFloorTile,
		const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
		UDungeonFloorManager* FloorManager, int32 MaxXSize, int32 MaxYSize,
		int32 XPosition, int32 YPosition, int32 ZPosition, FFloorRoom Room,
		FRandomStream &Rng, bool bUseRandomDimensions = true, bool bIsDeterminedFromPoints = false);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void DoTileReplacement(FRandomStream &Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Ground Scatter")
	UGroundScatterManager* GetGroundScatter() const;
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation")
	UDungeonFloorManager* GetFloorManager() const;

	void PlaceRoomTiles(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
		FRandomStream& Rng);

	void DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations,
		FRandomStream& Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FDungeonSpace& GetDungeon() const;
	
	// Gets the transform for a tile from that tile's position in local space ((0,0,0) to Room Bounds).
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransform(const FIntVector& LocalLocation) const;
	
	// Gets the transform for a tile from that tile's position in world space.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const;
	
	// Returns the set of all DungeonTiles used by this room.
	TSet<const UDungeonTile*> FindAllTiles();
	
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	TSet<FIntVector> GetAllTilesOfType(ETileType Type) const;

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
	ETileDirection GetTileDirectionLocalSpace(FIntVector Location) const;
	
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void CreateNewTileMesh(const UDungeonTile* Tile, const FTransform& Location);
	
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector GetRoomTileSpacePosition() const;

	void SetTileGridCoordinates(FIntVector currentLocation, const UDungeonTile* Tile);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	float GetRoomDifficulty() const;
	void TryToPlaceEntrances(const UDungeonTile* EntranceTile, FRandomStream& Rng);

protected:
	virtual void DoTileReplacementPreprocessing(FRandomStream& Rng);
	ADungeonRoom* AddNeighborEntrances(const FIntVector& Neighbor, FRandomStream& Rng,
		const UDungeonTile* EntranceTile);
	void PlaceTile(TMap<const UDungeonTile*, ASpaceMeshActor*>& ComponentLookup,
		const UDungeonTile* Tile, int32 MeshID, const FTransform& MeshTransformOffset, const FIntVector& Location);

	FTransform CreateMeshTransform(const FTransform &MeshTransformOffset, const FIntVector &Location) const;
	AActor* SpawnInteraction(const UDungeonTile* Tile, FDungeonTileInteractionOptions InteractionOptions, 
		const FIntVector& Location, FRandomStream& Rng);
	void CreateAllRoomTiles(TMap<const UDungeonTile*, TArray<FIntVector>>& TileLocations,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
		FRandomStream& Rng);

	virtual void SpawnInterfaces(FRandomStream &Rng);
};
