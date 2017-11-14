

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
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* MazeWallTile;

protected:
	UPROPERTY()
	TSet<FIntVector> FloorPositions;

public:
	virtual void DoTileReplacementPreprocessing(FRandomStream& Rng) override;

	bool PositionIsValid(FIntVector Position, const UDungeonTile* DefaultTile, bool bCheckNeighborCount = true) const;
	void MakeSection(FIntVector Location, const UDungeonTile* DefaultTile);
protected:
	void RecursiveBacktracker(const FIntVector& Start, const UDungeonTile* DefaultTile);
	bool RecursiveBacktrackerSearch(const FIntVector& Start, const FIntVector& Goal, TSet<FIntVector>& Visited);

private:
	bool ConnectToMainMaze(FIntVector ConnectLocation, const UDungeonTile* DefaultTile,
		TSet<FIntVector>& NewlyPlaced);
	void GetTileNeighbors(FIntVector Source, TSet<FIntVector>& OutNeighbors);
};
