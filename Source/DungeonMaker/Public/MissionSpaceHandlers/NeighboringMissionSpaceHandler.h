

#pragma once

#include "CoreMinimal.h"
#include "DungeonMissionSpaceHandler.h"
#include "NeighboringMissionSpaceHandler.generated.h"

/**
 * Generates a space by placing rooms next to any already-generated room.
 * Has a tendency to create long, linear spaces with a lot of backtracking.
 */
UCLASS()
class DUNGEONMAKER_API UNeighboringMissionSpaceHandler : public UDungeonMissionSpaceHandler
{
	GENERATED_BODY()
	
protected:
	virtual void GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount) override;
	
private:
	bool PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms,
		FMissionSpaceHelper& SpaceHelper, bool bIsTightCoupling, int32 TotalSymbolCount);

	void UpdateNeighbors(const FRoomPairing& RoomPairing, bool bIsTightCoupling);

	TMap<FIntVector, FIntVector> GetRoomNeighbors(FIntVector nextLocation, FMissionSpaceHelper &SpaceHelper);

	FRoomPairing GetOpenRoom(UDungeonMissionNode* Node,
		TMap<FIntVector, FIntVector>& AvailableRooms, FMissionSpaceHelper& SpaceHelper);
};
