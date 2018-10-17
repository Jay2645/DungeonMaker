

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DungeonRoom.h"
#include "DungeonTile.h"

#include "DungeonFloorHelpers.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONMAKER_API UDungeonFloorHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FIntVector GetRoomTileSpacePosition(ADungeonRoom* Room);

	// Gets the tile at a particular location inside of a room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FRoomTile& GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static TSet<FRoomTile> GetTileLocations(const UDungeonTile* Tile, ADungeonRoom* Room = NULL);
	
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FIntVector FindTileInDirection(ADungeonRoom* Room, const FIntVector& StartingLocation, const FIntVector& SearchDirection, const UDungeonTile* TileToSearchFor);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static bool IsLocationInRoom(ADungeonRoom* Room, const FIntVector& StartingLocation);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static FTransform CreateTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float ZLocation, float Offset);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static FTransform FindTileTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float Offset);
};
