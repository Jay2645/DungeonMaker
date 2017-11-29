#pragma once

#include "CoreMinimal.h"
#include "Interface.h"
#include "TrapTrigger.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTrapTrigger : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUNGEONMAKER_API ITrapTrigger
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool OnTriggerPlaced(AActor* Room);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool OnTriggerTriggered();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Traps")
	bool AddAssociatedTrap(AActor* Trap);
};
