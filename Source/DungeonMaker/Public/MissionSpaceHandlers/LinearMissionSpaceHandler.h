

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
public:
	// The max possible length (in rooms) any hallway should be.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	uint8 MaxDistanceBetweenRooms;

public:
	ULinearMissionSpaceHandler();

public:
	virtual void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount) override;

protected:
	void PairNodesToRooms(FFloorRoom Parent, TMap<UDungeonMakerNode*, FFloorRoom>& Processed, FIntVector StartLocation, TSet<FIntVector>& AvailableRooms, FRandomStream &Rng, int32 SymbolCount);
	void ConnectRooms(FFloorRoom Parent, FFloorRoom Child);
};
