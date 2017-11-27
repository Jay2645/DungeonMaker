

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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	TArray<AActor*> CreateTriggers(FRandomStream Rng);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	TArray<AActor*> CreateTraps(FRandomStream Rng);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool OnTrapActivated(AActor* ActivatedTrap);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool RetractTrap();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Trials")
	bool OnTrapComplete();
};
