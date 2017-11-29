

#pragma once

#include "CoreMinimal.h"
#include "Space/Rooms/DungeonRoom.h"
#include "Space/Rooms/Trials/TrialRoom.h"
#include "GroundScatterItem.h"
#include "TrialRoomBase.generated.h"

/**
 * An example room, forming the base class for a "trial".
 * These trials are puzzles forcing the player to test their skill.
 */
UCLASS(Abstract)
class DUNGEONMAKER_API ATrialRoomBase : public ADungeonRoom, public ITrialRoom
{
	GENERATED_BODY()
public:
	// A list of all tile replacements which will place triggers.
	// One item in this list will be selected; the rest will be discarded.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traps")
	TArray<FRoomReplacements> TriggerTileReplacements;
	// Should we include the "On Player Enter Room" event as a trigger?
	// If this is true and there are tiles specified in the Trigger Tile Replacements array,
	// then either this or one of the potential tile replacements will
	// be chosen at random.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traps")
	bool bIncludeOnPlayerEnterAsTrigger;
	// A list of all trap replacement rules.
	// One item in this list will be selected; the rest will be discarded.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traps")
	TArray<FRoomReplacements> TrapTileReplacements;

	// What kind of trigger should be spawned on various tiles. Please ensure you use an
	// actor and not a static mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traps")
	TMap<const UDungeonTile*, const UGroundScatterItem*> TileTriggerObjects;
	// What kind of trap should be spawned on various tiles. Please ensure you use an
	// actor and not a static mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traps")
	TMap<const UDungeonTile*, const UGroundScatterItem*> TileTrapObjects;

	// A list of all traps we've spawned in this room.
	// If we're using On Player Enter Room as a trigger, everything in this array will
	// be triggered.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traps")
	TArray<AActor*> TrapList;

	// A list of all triggers we've spawned in this room.
	// By default, any one of these triggers will trigger all the traps -- if you need different
	// behavior, you should probably make a subclass which handles it.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Traps")
	TArray<AActor*> TriggerList;

protected:
	UFUNCTION()
	virtual void OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
	virtual void DoTileReplacementPreprocessing(FRandomStream& Rng) override;

	TArray<AActor*> CreateTriggers_Implementation(FRandomStream Rng);
	TArray<AActor*> CreateTraps_Implementation(FRandomStream Rng);
};
