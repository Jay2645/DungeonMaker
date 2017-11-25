

#pragma once

#include "CoreMinimal.h"
#include "Space/Rooms/DungeonRoom.h"
#include "Space/Rooms/Trials/TrialRoom.h"
#include "TrialRoomBase.generated.h"

/**
 * An example room, forming the base class for a "trial".
 * These trials are puzzles forcing the player to test their skill.
 */
UCLASS(Abstract)
class DUNGEONMAKER_API ATrialRoomBase : public ADungeonRoom, public ITrialRoom
{
	GENERATED_BODY()	
};
