

#pragma once

#include "CoreMinimal.h"
#include "TrialRoom.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTrialRoom : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DUNGEONMAKER_API ITrialRoom
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	TArray<AActor*> CreateTraps();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool OnTrapActivated();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool RetractTrap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool OnTrapComplete();
};
