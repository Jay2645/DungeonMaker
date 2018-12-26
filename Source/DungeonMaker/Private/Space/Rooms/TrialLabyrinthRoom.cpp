#include "TrialLabyrinthRoom.h"

void ATrialLabyrinthRoom::DoTileReplacementPreprocessing(FRandomStream& Rng)
{
	FDungeonSpace& dungeon = GetDungeon();
	TSet<FIntVector> entranceLocations = dungeon.GetTileLocations(ETileType::Entrance, this);

	if (entranceLocations.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("%s had no entrance to create a labyrinth!"), *GetName());
		return;
	}

	if (XSize() <= 3 || YSize() <= 3)
	{
		// Not big enough to make a maze
		UE_LOG(LogSpaceGen, Error, TEXT("%s is too small to be a labyrinth!"), *GetName());
		return;
	}
	// We're technically big enough to be a maze, albeit we may not be a very fun one

	// This stores the tile used as the "default" tile for this room
	// The walls may already be set, but (1, 1) is guaranteed to become floor
	const UDungeonTile* defaultTile = dungeon.GetTile(FIntVector(GetRoomLocation() + FIntVector(1, 1, 0)));

	// Maze is made using a recursive backtracker
	TArray<FIntVector> cellPositions;
	// Find the list of entrances
	TArray<FIntVector> entrances = entranceLocations.Array();
	TArray<FIntVector> entranceTileLocations;
	for (int i = 0; i < entrances.Num(); i++)
	{
		bool bFoundEntranceDirection = false;
		// Find what direction the entrance is facing
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				// Skip diagonals
				if (x == -1 && y == -1 ||
					x == 1 && y == -1 ||
					x == -1 && y == 1 ||
					x == 1 && y == 1)
				{
					continue;
				}
				FIntVector position = entrances[i] + FIntVector(x, y, 0);
				const UDungeonTile* neighborTile = dungeon.GetTile(position);
				if (neighborTile != defaultTile)
				{
					continue;
				}
				bFoundEntranceDirection = true;
				// This is a spot next to an entrance
				entranceTileLocations.Add(position);
				break;
			}
			if (bFoundEntranceDirection)
			{
				break;
			}
		}
	}

	if (entranceTileLocations.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("%s had no entrances! Something funky is happening."), *GetName());
		return;
	}

	// Run the maze generator
	MakeSection(entranceTileLocations[0], defaultTile, true);
	RecursiveBacktracker(entranceTileLocations[0], defaultTile, Rng);

	for (FIntVector location : entranceLocations)
	{
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				const UDungeonTile* tile = dungeon.GetTile(FIntVector(location.X + x, location.Y + y, 0));
				if (tile == defaultTile)
				{
					dungeon.SetTile(FIntVector(location.X + x, location.Y + y, 0), MazeGroundTile);
				}
			}
		}
	}
}

bool ATrialLabyrinthRoom::PositionIsValid(FIntVector Position, const UDungeonTile* DefaultTile, bool bCheckNeighborCount)
{
	// Check to see if a position touches exactly one existing passage
	if (FloorPositions.Contains(Position))
	{
		return false;
	}

	FIntVector roomMinExtent = GetRoomLocation();
	FIntVector roomMaxExtent = roomMinExtent + GetRoomSize();

	if (Position.X < roomMinExtent.X || Position.Y < roomMinExtent.Y ||
		Position.X >= roomMaxExtent.X || Position.Y >= roomMaxExtent.Y)
	{
		return false;
	}

	const UDungeonTile* tile = GetDungeon().GetTile(Position);
	if (tile != DefaultTile)
	{
		// Not available for carving
		return false;
	}

	TArray<FIntVector> neighborLocations;
	neighborLocations.Add(FIntVector(-1, 0, 0));
	neighborLocations.Add(FIntVector(1, 0, 0));
	neighborLocations.Add(FIntVector(0, -1, 0));
	neighborLocations.Add(FIntVector(0, 1, 0));

	int adjacentCount = 0;
	for (int i = 0; i < neighborLocations.Num(); i++)
	{
		if (FloorPositions.Contains(Position + neighborLocations[i]))
		{
			adjacentCount++;
		}
	}
	return adjacentCount <= 1;
}

bool ATrialLabyrinthRoom::MakeSection(FIntVector Location, const UDungeonTile* DefaultTile, bool bForceGenerate)
{
	if (!bForceGenerate && !PositionIsValid(Location, DefaultTile))
	{
		return false;
	}
	FloorPositions.Add(Location);
	DungeonSpace->DungeonSpace.SetTile(Location, MazeGroundTile);
	return true;
}

bool ATrialLabyrinthRoom::RecursiveBacktracker(const FIntVector& Start, const UDungeonTile* DefaultTile,
	FRandomStream& Rng)
{
	// Add array of neighbor locations
	TArray<FIntVector> neighborLocations;
	neighborLocations.Add(FIntVector(-1, 0, 0));
	neighborLocations.Add(FIntVector(1, 0, 0));
	neighborLocations.Add(FIntVector(0, -1, 0));
	neighborLocations.Add(FIntVector(0, 1, 0));
	
	// Shuffle
	neighborLocations.Swap(Rng.RandRange(0, neighborLocations.Num() - 1), Rng.RandRange(0, neighborLocations.Num() - 1));
	neighborLocations.Swap(Rng.RandRange(0, neighborLocations.Num() - 1), Rng.RandRange(0, neighborLocations.Num() - 1));
	neighborLocations.Swap(Rng.RandRange(0, neighborLocations.Num() - 1), Rng.RandRange(0, neighborLocations.Num() - 1));
	neighborLocations.Swap(Rng.RandRange(0, neighborLocations.Num() - 1), Rng.RandRange(0, neighborLocations.Num() - 1));

	// Place neighbors randomly
	for (int i = 0; i < neighborLocations.Num(); i++)
	{
		if (!MakeSection(Start + neighborLocations[i], DefaultTile))
		{
			continue;
		}
/*		FIntVector next = Start + neighborLocations[i] + neighborLocations[i];
		if (next.X >= 0 && next.X < XSize() && next.Y >= 0 && next.Y < YSize())
		{
			MakeSection(Start + neighborLocations[i] + neighborLocations[i], DefaultTile, true);
			RecursiveBacktracker(Start + neighborLocations[i] + neighborLocations[i], DefaultTile, Rng);
		}
		else
		{*/
			RecursiveBacktracker(Start + neighborLocations[i], DefaultTile, Rng);
		//}
	}

	return true;
}

bool ATrialLabyrinthRoom::RecursiveBacktrackerSearch(const FIntVector& Start, const FIntVector& Goal,
	TSet<FIntVector>& Visited)
{
	if (Start == Goal)
	{
		return true;
	}

	if (Visited.Contains(Start))
	{
		return false;
	}

	const UDungeonTile* tile = GetDungeon().GetTile(Start);
	if (tile == NULL)
	{
		// Wall tile
		return false;
	}

	Visited.Add(Start);
	if (Start.X < XSize() - 1 && RecursiveBacktrackerSearch(Start + FIntVector(1, 0, 0), Goal, Visited) ||
		Start.Y < YSize() - 1 && RecursiveBacktrackerSearch(Start + FIntVector(0, 1, 0), Goal, Visited) ||
		Start.X > 0 && RecursiveBacktrackerSearch(Start + FIntVector(-1, 0, 0), Goal, Visited) ||
		Start.Y > 0 && RecursiveBacktrackerSearch(Start + FIntVector(0, -1, 0), Goal, Visited))
	{
		return true;
	}
	else
	{
		return false;
	}
}