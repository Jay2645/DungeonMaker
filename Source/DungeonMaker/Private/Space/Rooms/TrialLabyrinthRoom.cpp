#include "TrialLabyrinthRoom.h"

void ATrialLabyrinthRoom::DoTileReplacementPreprocessing()
{
	if (EntranceLocations.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Room had no entrance!"));
		return;
	}

	if (XSize() <= 3 || YSize() <= 3)
	{
		// Not big enough to make a maze
		return;
	}
	// We're technically big enough to be a maze, albeit we may not be a very fun one

	// This stores the tile used as the "default" tile for this room
	// The walls may already be set, but (1, 1) is guaranteed to become floor
	const UDungeonTile* defaultTile = GetTile(1, 1);

	// Maze is made using a recursive backtracker
	TArray<FIntVector> cellPositions;
	// Find the list of entrances
	TArray<FIntVector> entrances = EntranceLocations.Array();
	TArray<FIntVector> entranceTileLocations;
	for (int i = 0; i < entrances.Num(); i++)
	{
		bool bFoundEntranceDirection = false;
		for (int x = entrances[i].X - 1; x <= entrances[i].X + 1; x++)
		{
			for (int y = entrances[i].Y - 1; y < entrances[i].Y + 1; y++)
			{
				// Skip diagonals
				if (x == entrances[i].X - 1 && y == entrances[i].Y - 1 ||
					x == entrances[i].X + 1 && y == entrances[i].Y - 1 ||
					x == entrances[i].X - 1 && y == entrances[i].Y + 1 ||
					x == entrances[i].X + 1 && y == entrances[i].Y + 1)
				{
					continue;
				}
				const UDungeonTile* neighborTile = GetTile(x, y);
				if (neighborTile != defaultTile)
				{
					continue;
				}
				bFoundEntranceDirection = true;
				FIntVector position = FIntVector(x, y, entrances[i].Z);
				entranceTileLocations.Add(position);
				break;
			}
			if (bFoundEntranceDirection)
			{
				break;
			}
		}
	}

	// Carve out this entrance
	MakeSection(entranceTileLocations[0], defaultTile);
	// Run the maze generator for this entrance
	RecursiveBacktracker(entranceTileLocations[0], defaultTile);

	if (entranceTileLocations.Num() > 1)
	{
		for (int i = 1; i < entranceTileLocations.Num(); i++)
		{
			if (!FloorPositions.Contains(entranceTileLocations[i]))
			{
				TSet<FIntVector> newlyPlaced;
				ConnectToMainMaze(entranceTileLocations[i], defaultTile, newlyPlaced);
			}
		}
	}
}

bool ATrialLabyrinthRoom::PositionIsValid(FIntVector Position, const UDungeonTile* DefaultTile, bool bCheckNeighborCount) const
{
	// Check to see if a position touches exactly one existing passage
	const UDungeonTile* tile = GetTile(Position.X, Position.Y);
	if (tile != DefaultTile && tile != MazeWallTile)
	{
		// Not available for carving
		return false;
	}
	const int MAX_TILE_TOUCH_COUNT = 2;
	int currentTouchCount = 0;
	for (int x = Position.X - 1; x <= Position.X + 1; x++)
	{
		for (int y = Position.Y - 1; y <= Position.Y + 1; y++)
		{
			// Skip checking us
			if (x == Position.X && y == Position.Y)
			{
				continue;
			}
			// Skip diagonals
			const UDungeonTile* neighbor = GetTile(x, y);

			// Abort if placing this would make us neighbor open air
			if (neighbor == NULL)
			{
				// (Except on diagonals)
				if (x == Position.X - 1 && y == Position.Y - 1 ||
					x == Position.X + 1 && y == Position.Y - 1 ||
					x == Position.X - 1 && y == Position.Y + 1 ||
					x == Position.X + 1 && y == Position.Y + 1)
				{
					// Pass
					continue;
				}
				else
				{
					// Directly neighbor open air
					return false;
				}
			}

			if (neighbor == MazeGroundTile)
			{
				currentTouchCount++;
				if (bCheckNeighborCount && currentTouchCount > MAX_TILE_TOUCH_COUNT)
				{
					// Touches more than the maximum number of floor tiles!
					return false;
				}

			}
		}
	}
	// We only care if we touched at least one ground tile
	return currentTouchCount > 0;
}

void ATrialLabyrinthRoom::MakeSection(FIntVector Location, const UDungeonTile* DefaultTile)
{
	for (int x = Location.X - 1; x <= Location.X + 1; x++)
	{
		for (int y = Location.Y - 1; y <= Location.Y + 1; y++)
		{
			// Skip diagonals
			if (x == Location.X - 1 && y == Location.Y - 1 ||
				x == Location.X + 1 && y == Location.Y - 1 ||
				x == Location.X - 1 && y == Location.Y + 1 ||
				x == Location.X + 1 && y == Location.Y + 1)
			{
				continue;
			}

			// Set the ground tile
			if (x == Location.X && y == Location.Y)
			{
				Set(x, y, MazeGroundTile);
				FloorPositions.Add(Location);
			}
			else
			{
				// Set the wall tiles
				const UDungeonTile* neighbor = GetTile(x, y);
				if (neighbor == DefaultTile)
				{
					Set(x, y, MazeWallTile);
				}
			}
		}
	}
}

void ATrialLabyrinthRoom::RecursiveBacktracker(const FIntVector& Start, const UDungeonTile* DefaultTile)
{
	TArray<FIntVector> stack;
	stack.Add(Start);
	// Iterate over the stack
	while (stack.Num() > 0)
	{
		int nextIndex = stack.Num() - 1;
		FIntVector next = stack[nextIndex];
		bool bFoundPositions = false;
		for (int x = next.X - 1; x <= next.X + 1; x++)
		{
			for (int y = next.Y - 1; y < next.Y + 1; y++)
			{
				FIntVector position = FIntVector(x, y, next.Z);
				// Always carve into an unmade section
				if (PositionIsValid(position, DefaultTile))
				{
					MakeSection(position, DefaultTile);
					stack.Add(position);
					bFoundPositions = true;
				}
			}
		}
		if (!bFoundPositions)
		{
			// No unmade cells next to the current position
			// Pop the stack back to the last position
			stack.RemoveAt(nextIndex);
		}
	}
}

bool ATrialLabyrinthRoom::ConnectToMainMaze(FIntVector ConnectLocation, const UDungeonTile* DefaultTile, 
	TSet<FIntVector>& NewlyPlaced)
{
	// Make the current section
	MakeSection(ConnectLocation, DefaultTile);
	// Mark it as a new connection -- don't get our hopes up that we found
	// a connection when it was really just our neighbor
	NewlyPlaced.Add(ConnectLocation);

	for (int x = ConnectLocation.X - 1; x <= ConnectLocation.X + 1; x++)
	{
		for (int y = ConnectLocation.Y - 1; y <= ConnectLocation.Y + 1; y++)
		{
			FIntVector neighborPosition = FIntVector(x, y, ConnectLocation.Z);
			// Skip anything already processed
			if (NewlyPlaced.Contains(neighborPosition))
			{
				continue;
			}

			// Skip diagonals
			if (x == ConnectLocation.X - 1 && y == ConnectLocation.Y - 1 ||
				x == ConnectLocation.X + 1 && y == ConnectLocation.Y - 1 ||
				x == ConnectLocation.X - 1 && y == ConnectLocation.Y + 1 ||
				x == ConnectLocation.X + 1 && y == ConnectLocation.Y + 1)
			{
				continue;
			}

			if (FloorPositions.Contains(neighborPosition))
			{
				// All done!
				return true;
			}

			// Skip if the neighbor is an invalid tile
			if (!PositionIsValid(neighborPosition, DefaultTile, false))
			{
				continue;
			}

			// Go recursive
			if (ConnectToMainMaze(neighborPosition, DefaultTile, NewlyPlaced))
			{
				// Our neighbor connects!
				return true;
			}
		}
	}
	return false;
}

void ATrialLabyrinthRoom::GetTileNeighbors(FIntVector Source, TSet<FIntVector>& OutNeighbors)
{
	OutNeighbors.Add(Source);
	for (int x = Source.X - 1; x <= Source.X + 1; x++)
	{
		for (int y = Source.Y - 1; y <= Source.Y + 1; y++)
		{
			FIntVector neighborPosition = FIntVector(x, y, Source.Z);
			// Skip anything already processed
			if (OutNeighbors.Contains(neighborPosition))
			{
				continue;
			}

			// Skip diagonals
			if (x == Source.X - 1 && y == Source.Y - 1 ||
				x == Source.X + 1 && y == Source.Y - 1 ||
				x == Source.X - 1 && y == Source.Y + 1 ||
				x == Source.X + 1 && y == Source.Y + 1)
			{
				continue;
			}

			if (FloorPositions.Contains(neighborPosition))
			{
				GetTileNeighbors(neighborPosition, OutNeighbors);
			}
		}
	}
}