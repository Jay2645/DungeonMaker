

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
	// Gets the tile at a particular location inside of a room.
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	static FRoomTile& GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location);
	
};
