#pragma once

#include "CoreMinimal.h"
#include "Interface.h"
#include "Trap.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTrap : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUNGEONMAKER_API ITrap
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool InitializeTrap(ADungeonRoom* Room);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool ActivateTrap();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool DeactivateTrap();
};
