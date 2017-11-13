

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
	
	virtual void DoTileReplacementPreprocessing() override;

	bool PositionIsValid(FIntVector Position, const UDungeonTile* DefaultTile, bool bCheckNeighborCount = true) const;
	void MakeSection(FIntVector Location, const UDungeonTile* DefaultTile);
protected:
	void RecursiveBacktracker(const FIntVector& Start, const UDungeonTile* DefaultTile);
	UPROPERTY()
	TSet<FIntVector> FloorPositions;
private:
	bool ConnectToMainMaze(FIntVector ConnectLocation, const UDungeonTile* DefaultTile,
		TSet<FIntVector>& NewlyPlaced);
	void GetTileNeighbors(FIntVector Source, TSet<FIntVector>& OutNeighbors);
};
