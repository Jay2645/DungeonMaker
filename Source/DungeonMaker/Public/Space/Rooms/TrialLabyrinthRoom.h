

#pragma once

#include "CoreMinimal.h"
#include "Space/Rooms/TrialRoomBase.h"
#include "TrialLabyrinthRoom.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class DUNGEONMAKER_API ATrialLabyrinthRoom : public ATrialRoomBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* MazeGroundTile;

protected:
	UPROPERTY()
	TSet<FIntVector> FloorPositions;

public:
	virtual void DoTileReplacementPreprocessing(FRandomStream& Rng) override;

	bool PositionIsValid(FIntVector Position, const UDungeonTile* DefaultTile, bool bCheckNeighborCount = true);
	bool MakeSection(FIntVector Location, const UDungeonTile* DefaultTile, bool bForceGenerate = false);
protected:
	bool RecursiveBacktracker(const FIntVector& Start, const UDungeonTile* DefaultTile,
		FRandomStream& Rng);
	bool RecursiveBacktrackerSearch(const FIntVector& Start, const FIntVector& Goal, TSet<FIntVector>& Visited);
};
