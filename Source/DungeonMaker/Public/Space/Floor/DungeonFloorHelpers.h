

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
	// Returns the location in tile space of a given room.
	// Tile space is defined as the "lower" left-hand corner of the room; i.e., the point of
	// the room with the lowest coordinates.
	// Will return (-1, -1, -1) on failure.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FIntVector GetRoomTileSpacePosition(ADungeonRoom* Room);

	// Gets the tile at a particular location inside of a room.
	// Location should be given in tile space. Will return a reference to that tile.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FRoomTile& GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location);

	// Get the location of all tiles of a certain type.
	// This can optionally be limited to just grabbing the location of tiles inside of a particular room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static TSet<FIntVector> GetTileLocations(const UDungeonTile* Tile, ADungeonRoom* Room = NULL);
	
	// Finds the closest tile in a given direction.
	// For example, if you were in the middle of a room and wanted to find the coordinate of the first 
	// wall you can find to the left, you'd pass in the coordinate of the tile in the center, a vector 
	// telling the algorithm to go left (-1, 0, 0), and a UDungeonTile object representing the type of 
	// tile you're searching for. 
	// If traveling in that direction will cause the algorithm to leave the room, it will fail out.
	// Will return (-1, -1, -1) on failure.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static FIntVector FindTileInDirection(ADungeonRoom* Room, const FIntVector& StartingLocation, const FIntVector& SearchDirection, const UDungeonTile* TileToSearchFor);

	// Checks if a location is inside a given room. Will return false if the room is NULL.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static bool IsLocationInRoom(ADungeonRoom* Room, const FIntVector& StartingLocation);

	// Creates a transform from a given tile space location. Useful if you want to place something on top of a tile.
	// Location given should be relative to the provided room; i.e. (0, 0, 0) is the bottom-left corner of the room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static FTransform CreateTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float ZLocation, float Offset);

	// Gets the cardinal direction that a tile is in, in tile space.
	// For example, if a tile represents the south wall of a room, it would be ETileDirection::South.
	// If a tile is in the center of the room, it would be ETileDirection::Center.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	static ETileDirection GetTileDirection(ADungeonRoom* Room, const FIntVector& Location);

	// Creates a transform from a given tile space location. Useful if you want to place something on top of a tile.
	// Location given should be relative to the provided room; i.e. (0, 0, 0) is the bottom-left corner of the room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static FTransform FindTileTransform(ADungeonRoom* Room, const FIntVector& RelativeLocation, float Offset);

	// Draws a debug version of the dungeon. Allows you to see where tiles will be placed and how rooms are set up.
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Debug")
	static void DrawDungeon(AActor* ContextObject, FDungeonSpace DungeonSpace);

	// Converts a given floor on the dungeon to a string. This will be a text representation of all tiles and rooms
	// on that floor.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Debug")
	static FString FloorToTileString(FDungeonSpace DungeonSpace, uint8 FloorNum);

	// Converts a given room in the dungeon to a string. This will be a text representation of all tiles inside of
	// that room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Debug")
	static FString RoomToTileString(FDungeonSpace DungeonSpace, ADungeonRoom* Room);

	// Do these rooms directly border each other east-west?
	// Note that this only return true if you can make the rooms border each other by replacing 2 or fewer walls
	// with floor tiles. If they border diagonally, or only border on a single tile (meaning you'd have to remove
	// more than 2 wall tiles to have an accessible path between them), it will return false.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static bool AreRoomsLeftRightAdjacent(ADungeonRoom* First, ADungeonRoom* Second);
	// Do these rooms directly border each other north-south?
	// Note that this only return true if you can make the rooms border each other by replacing 2 or fewer walls
	// with floor tiles. If they border diagonally, or only border on a single tile (meaning you'd have to remove
	// more than 2 wall tiles to have an accessible path between them), it will return false.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static bool AreRoomsTopDownAdjacent(ADungeonRoom* First, ADungeonRoom* Second);
	// Do these rooms directly border each other?
	// Note that this only return true if you can make the rooms border each other by replacing 2 or fewer walls
	// with floor tiles. If they border diagonally, or only border on a single tile (meaning you'd have to remove
	// more than 2 wall tiles to have an accessible path between them), it will return false.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static bool AreRoomsAdjacent(ADungeonRoom* First, ADungeonRoom* Second);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static bool AreFloorRoomsAdjacent(const FFloorRoom& First, const FFloorRoom& Second);
};
