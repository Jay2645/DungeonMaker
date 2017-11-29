#pragma once

#include "CoreMinimal.h"
#include "Interface.h"
#include "LockedDoor.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class ULockedDoor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUNGEONMAKER_API ILockedDoor
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Items")
	bool OnSpawned(AActor* Room);
};
