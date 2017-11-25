#pragma once

#include "CoreMinimal.h"
#include "Items/LockedDoor.h"
#include "LockedRoom.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULockedRoom : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUNGEONMAKER_API ILockedRoom
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Items")
	AActor* SpawnLock(FRandomStream Rng);

	// Things to do when the locked door in this room becomes unlocked.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Items")
	bool OnDoorUnlocked();
};
