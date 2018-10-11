

#pragma once

#include "CoreMinimal.h"
#include "DungeonMissionSpaceHandler.h"
#include "LinearMissionSpaceHandler.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONMAKER_API ULinearMissionSpaceHandler : public UDungeonMissionSpaceHandler
{
	GENERATED_BODY()

	virtual void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount) override;
};
