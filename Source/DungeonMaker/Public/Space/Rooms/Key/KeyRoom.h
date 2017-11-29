#pragma once

#include "CoreMinimal.h"
#include "Interface.h"
#include "KeyRoom.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UKeyRoom : public UInterface
{
	GENERATED_BODY()
};

/**
 * This is an interface which indicates this room has a key in it.
 */
class DUNGEONMAKER_API IKeyRoom
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Items")
	AActor* SpawnKey(FRandomStream Rng);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Items")
	bool OnKeyTaken(AActor* Key);
};