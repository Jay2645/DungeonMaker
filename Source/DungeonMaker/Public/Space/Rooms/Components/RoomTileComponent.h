

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "DungeonRoom.h"
#include "RoomReplacementPattern.h"

#include "RoomTileComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API URoomTileComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	ADungeonRoom* ParentRoom;

	// The tile representing an entrance that opens into this room
	UPROPERTY()
	const UDungeonTile* RoomEntranceTile;
	// The tile representing an exit into further rooms.
	UPROPERTY()
	const UDungeonTile* RoomExitTile;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiles")
	TArray<FRoomReplacements> RoomReplacementPhases;

	// The max height of this room
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	uint8 MaxRoomHeight;
	// The minimum size this room can possibly be
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room")
	FIntVector MinRoomSize;
	// Where this room is located, in tile space
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	FIntVector RoomLocation;
	// This room's actual size
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	FIntVector RoomSize;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugTiles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bPrintRoomTiles;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDoTileReplacement;
public:
	// Sets default values for this component's properties
	URoomTileComponent();

protected:
	const UDungeonTile* GetTile(const FIntVector& Location);

public:
	void InitializeTileComponent(ADungeonRoom* Room, const FIntVector& RoomDimensions, const FIntVector& RoomPosition,
		const UDungeonTile* DefaultFloorTile, const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
		const UDungeonTile* DefaultExitTile, FRandomStream& Rng, bool bUseRandomSize);

	// Replaces all tiles in our room with the default tiles.
	// If any of these tiles were changed, they will be reset to the defaults!
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void SpawnStartingDefaultTiles(const UDungeonTile* DefaultTile);
	// Places walls all along the edge of our room.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void CarveWalls(const UDungeonTile* WallTile);

	// Creates entrances/hallways to another room.
	// Will return an array of all hallways we've generated to connect us to this other room.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	TArray<ADungeonRoom*> ConnectToRoom(ADungeonRoom* OtherRoom, TSubclassOf<ADungeonRoom> HallwayClass, FRandomStream& Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FDungeonSpace& GetDungeon() const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms")
	void DoTileReplacement(FRandomStream &Rng);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FIntVector GetRoomTileSpacePosition() const;

	// Gets the transform for a tile from that tile's position in local space ((0,0,0) to Room Bounds).
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransformFromLocalSpace(const FIntVector& LocalLocation) const;

	// Gets the transform for a tile from that tile's position in world space.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform GetTileTransformFromTileSpace(const FIntVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	ETileDirection GetTileDirection(const FIntVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void AddReplacementPhases(TArray<FRoomReplacements> AdditionalTileReplacements);
};
