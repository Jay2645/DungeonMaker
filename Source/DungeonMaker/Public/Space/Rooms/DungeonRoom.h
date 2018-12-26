// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DungeonMaker.h"
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"

#include "../Mission/DungeonMissionSymbol.h"
#include "DungeonFloorManager.h"

#include "DungeonRoom.generated.h"

class UDungeonSpaceGenerator;
class UGroundScatterManager;
class URoomMeshComponent;
class URoomTileComponent;
class UDungeonTile;

UCLASS(Abstract, Blueprintable, HideCategories = (Physics, Rendering, Collision, Tags, Activation, Cooking, Shape, Navigation, Input))
class DUNGEONMAKER_API ADungeonRoom : public AActor, public IGameplayTagAssetInterface
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
	URoomMeshComponent* RoomMeshes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	URoomTileComponent* RoomTiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UDungeonFloorManager* DungeonFloor;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Room")
	UDungeonSpaceGenerator* DungeonSpace;

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Room")
	const UDungeonMissionSymbol* Symbol;

	// An optional icon for this room.
	// By default, we don't do anything with it, but this could be used
	// as an icon on a minimap or some such.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	UTexture2D* RoomIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	TSubclassOf<ADungeonRoom> HallwayRoomClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	float RoomDifficultyModifier;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	FFloorRoom RoomMetadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	bool bRoomShouldBeRandomlySized;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	TSet<ADungeonRoom*> AllNeighbors;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	TSet<ADungeonRoom*> TightlyCoupledNeighbors;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Floor")
	FGameplayTag RoomType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer RoomTags;


	// Debug

	// Should this room be generated "standalone"?
	// This means that there's not ADungeon Actor telling it what to do.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bIsStandaloneRoomForDebug;
	// If we're being generated standalone, what tile should we use as our default tile?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultFloorTile;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultWallTile;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultEntranceTile;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	const UDungeonTile* DebugDefaultExitTile;
	// If we're being generated standalone, what seed should we use for the RNG?
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	int32 DebugSeed;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	FIntVector DebugRoomMaxExtents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bSpawnInterfaces;
	
protected:
	virtual void BeginPlay();

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
		void InitializeRoom(UDungeonSpaceGenerator* SpaceGenerator, UDungeonFloorManager* FloorManager,
			const UDungeonTile* DefaultFloorTile, const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
			const UDungeonTile* DefaultExitTile, const FIntVector& RoomDimensions, const FIntVector& RoomPosition,
			FFloorRoom RoomData, FRandomStream &Rng, bool bIsDeterminedFromPoints = false);
	
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void DoTileReplacement(FRandomStream &Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Ground Scatter")
	UGroundScatterManager* GetGroundScatter() const
	{
		return GroundScatter;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation")
	UDungeonFloorManager* GetFloorManager() const
	{
		return DungeonFloor;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	URoomTileComponent* GetTileComponent() const
	{
		return RoomTiles;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Meshes")
	URoomMeshComponent* GetMeshComponent() const
	{
		return RoomMeshes;
	}

	// Gets the tile-space position of this room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector GetRoomLocation() const;

	// Gets how large this room is, in tiles.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector GetRoomSize() const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 XSize() const
	{
		return GetRoomSize().X;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 YSize() const
	{
		return GetRoomSize().Y;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	int32 ZSize() const
	{
		return GetRoomSize().Z;
	}

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FDungeonSpace& GetDungeon() const;
	
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	bool IsChangedAtRuntime() const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	float GetRoomDifficulty() const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generaton|Rooms")
	void CreateEntranceToNeighbors(FRandomStream& Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	bool IsChildOf(ADungeonRoom* ParentRoom) const;

	/**
	* Get any owned gameplay tags on the asset
	*
	* @param OutTags	[OUT] Set of tags on the asset
	*/
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = RoomTags;
	}

protected:
	virtual void DoTileReplacementPreprocessing(FRandomStream& Rng);
	virtual void SpawnInterfaces(FRandomStream &Rng);
};
