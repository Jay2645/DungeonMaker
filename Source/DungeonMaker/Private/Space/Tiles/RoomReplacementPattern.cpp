// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomReplacementPattern.h"
#include "DungeonRoom.h"
#include "DungeonFloorManager.h"

URoomReplacementPattern::URoomReplacementPattern()
{
	SelectionChance = 1.0f;
}

bool URoomReplacementPattern::FindAndReplace(FDungeonRoomMetadata& ReplaceRoom, FRandomStream& Rng)
{
	checkf(Input.IsNotNull(), TEXT("You didn't specify any input for replacement data!"));

	int replacementXSize = Input.XSize();
	int replacementYSize = Input.YSize();

	FDungeonRoomMetadata roomReplacement = FDungeonRoomMetadata(replacementXSize, replacementYSize);

	int xOffset = -replacementXSize;
	int yOffset = -replacementYSize;

	int width = ReplaceRoom.XSize();
	int height = ReplaceRoom.YSize();

	// This will store all possible replacements
	// It's used only if we're picking a replacement randomly
	TArray<FIntVector> possibleReplacements;

	while (yOffset < height + replacementYSize)
	{
		while (xOffset < width + replacementXSize)
		{
			for (int localXOffset = 0; localXOffset < replacementXSize; localXOffset++)
			{
				int x = xOffset + localXOffset;
				for (int localYOffset = 0; localYOffset < replacementYSize; localYOffset++)
				{
					int y = yOffset + localYOffset;
					const UDungeonTile* tile;
					if (x < 0 || x >= width || y < 0 || y >= height)
					{
						tile = NULL;
					}
					else
					{
						tile = ReplaceRoom[y][x];
					}
					roomReplacement.Set(localXOffset, localYOffset, tile);
				}
			}
			uint8 replacementOutput = MatchesReplacement(roomReplacement);
			if (replacementOutput != 0)
			{
				if (bRandomlyPlaced)
				{
					// Update the possible replacements array with this potential replacement,
					// then move on.
					possibleReplacements.Add(FIntVector(xOffset, yOffset, replacementOutput));
				}
				else
				{
					UpdateFloorTiles(replacementXSize, replacementYSize, xOffset, yOffset, width, height, replacementOutput, NULL, ReplaceRoom);
					return true;
				}
			}
			xOffset++;
		}
		yOffset++;
		xOffset = -replacementXSize;
	}

	if (bRandomlyPlaced && possibleReplacements.Num() > 0)
	{
		int32 randomVectorID = Rng.RandRange(0, possibleReplacements.Num() - 1);
		FIntVector randomVector = possibleReplacements[randomVectorID];
		UpdateFloorTiles(replacementXSize, replacementYSize, randomVector.X, randomVector.Y, width, height, (uint8)randomVector.Z, NULL, ReplaceRoom);
		return true;
	}
	else
	{
		return false;
	}
}

bool URoomReplacementPattern::FindAndReplaceFloor(UDungeonFloorManager* ReplaceFloor, FRandomStream& Rng)
{
	checkf(Input.IsNotNull(), TEXT("You didn't specify any input for replacement data!"));

	// Replacement X and Y sizes are going to be the same size as the input
	int replacementXSize = Input.XSize();
	int replacementYSize = Input.YSize();

	FDungeonRoomMetadata roomReplacement = FDungeonRoomMetadata(replacementXSize, replacementYSize);

	// We start on the negative side, so we can match against edges (which will be null)
	int xOffset = -replacementXSize;
	int yOffset = -replacementYSize;

	int width = ReplaceFloor->XSize();
	int height = ReplaceFloor->YSize();

	// This will store all possible replacements
	// It's used only if we're picking a replacement randomly
	TArray<FIntVector> possibleReplacements;

	// Iterate from the beyond the bottom left of the map to beyond the top right of the map
	// Again, this ensures that we can properly detect edges and corners
	while (yOffset < height + replacementYSize)
	{
		while (xOffset < width + replacementXSize)
		{
			// Build the replacement, with this X and Y offset serving as the bottom-left corner
			for (int localXOffset = 0; localXOffset < replacementXSize; localXOffset++)
			{
				int x = xOffset + localXOffset;
				for (int localYOffset = 0; localYOffset < replacementYSize; localYOffset++)
				{
					int y = yOffset + localYOffset;

					const UDungeonTile* tile;
					// Grab the tile at this location, or null if it's beyond our boundaries
					if (x < 0 || x >= width || y < 0 || y >= height)
					{
						tile = NULL;
					}
					else
					{
						FIntVector location = FIntVector(x, y, 0);
						tile = ReplaceFloor->GetTileFromTileSpace(location);
					}
					// Update our current replacement's tiles with this current tile
					// Since local X and Y offsets go from zero, this "scrubs" the
					// room replacement tiles every time the X or Y offset moves.
					roomReplacement.Set(localXOffset, localYOffset, tile);
				}
			}

			// Attempt the replacement
			if (roomReplacement.IsNotNull())
			{
				// Do we match the replacement?
				uint8 replacementOutput = MatchesReplacement(roomReplacement);
				if (replacementOutput != 0)
				{
					if (bRandomlyPlaced)
					{
						// Update the possible replacements array with this potential replacement,
						// then move on.
						possibleReplacements.Add(FIntVector(xOffset, yOffset, replacementOutput));
					}
					else
					{
						// We match the replacement; update the floor tiles
						UpdateFloorTiles(replacementXSize, replacementYSize, xOffset, yOffset, width, height, replacementOutput, ReplaceFloor, roomReplacement);
						return true;
					}
				}
			}
			xOffset++;
		}
		yOffset++;
		xOffset = -replacementXSize;
	}

	if (bRandomlyPlaced && possibleReplacements.Num() > 0)
	{
		int32 randomVectorID = Rng.RandRange(0, possibleReplacements.Num() - 1);
		FIntVector randomVector = possibleReplacements[randomVectorID];
		UpdateFloorTiles(replacementXSize, replacementYSize, randomVector.X, randomVector.Y, width, height, (uint8)randomVector.Z, ReplaceFloor, roomReplacement);
		return true;
	}
	else
	{
		return false;
	}
}

void URoomReplacementPattern::UpdateFloorTiles(int ReplacementXSize, int ReplacementYSize,
	int XOffset, int YOffset, int Width, int Height, uint8 ReplacementOutput, 
	UDungeonFloorManager* ReplaceFloor, FDungeonRoomMetadata &ReplaceRoom)
{
	for (int localXOffset = 0; localXOffset < ReplacementXSize; localXOffset++)
	{
		int x = XOffset + localXOffset;
		for (int localYOffset = 0; localYOffset < ReplacementYSize; localYOffset++)
		{
			int y = YOffset + localYOffset;
			if (x < 0 || x >= Width || y < 0 || y >= Height)
			{
				// Pass
			}
			else
			{
				FIntVector location = FIntVector(x, y, 0);
				int32 outputOffsetX;
				int32 outputOffsetY;
				if (ReplacementOutput == 1)
				{
					outputOffsetX = localXOffset;
					outputOffsetY = localYOffset;
				}
				else if (ReplacementOutput == 2)
				{
					outputOffsetX = localXOffset;
					outputOffsetY = ReplacementYSize - localYOffset - 1;
				}
				else if (ReplacementOutput == 3)
				{
					outputOffsetX = ReplacementXSize - localXOffset - 1;
					outputOffsetY = localYOffset;
				}
				else if (ReplacementOutput == 4)
				{
					outputOffsetX = ReplacementXSize - localXOffset - 1;
					outputOffsetY = ReplacementYSize - localYOffset - 1;
				}
				else
				{
					outputOffsetX = 0;
					outputOffsetY = 0;
					checkNoEntry();
				}
				if (ReplaceFloor == NULL)
				{
					ReplaceRoom.Set(x, y, Output[outputOffsetY][outputOffsetX]);
				}
				else
				{
					ReplaceFloor->UpdateTileFromTileSpace(location, Output[outputOffsetY][outputOffsetX]);
				}
			}
		}
	}
}

float URoomReplacementPattern::GetActualSelectionChance(ADungeonRoom* InputRoom) const
{
	return SelectionChance + (InputRoom->GetRoomDifficulty() * SelectionDifficultyModifier);
}

uint8 URoomReplacementPattern::MatchesReplacement(FDungeonRoomMetadata& InputToCheck)
{
	if (InputToCheck.XSize() != Input.XSize() || InputToCheck.YSize() != Input.YSize())
	{
		return 0;
	}
	// First attempt -- "normal"
	bool bCurrentStatus = true;
	for (int x = 0; x < InputToCheck.XSize(); x++)
	{
		for (int y = 0; y < InputToCheck.YSize(); y++)
		{
			const UDungeonTile* inputTile = Input[y][x];
			if (InputToCheck[y][x] != inputTile)
			{
				bCurrentStatus = false;
				break;
			}
		}
		if (!bCurrentStatus)
		{
			break;
		}
	}
	if (bCurrentStatus)
	{
		// Matched this attempt!
		return 1;
	}
	
	if (InputToCheck.YSize() > 1)
	{
		// Second attempt -- reversed Y
		bCurrentStatus = true;
		for (int x = 0; x < InputToCheck.XSize(); x++)
		{
			for (int y = 0; y < InputToCheck.YSize(); y++)
			{
				const UDungeonTile* inputTile = Input[InputToCheck.YSize() - y - 1][x];
				if (InputToCheck[y][x] != inputTile)
				{
					bCurrentStatus = false;
					break;
				}
			}
			if (!bCurrentStatus)
			{
				break;
			}
		}
		if (bCurrentStatus)
		{
			// Matched this attempt!
			return 2;
		}
	}

	if (InputToCheck.XSize() > 1)
	{
		// Third attempt -- reversed X
		bCurrentStatus = true;
		for (int x = 0; x < InputToCheck.XSize(); x++)
		{
			for (int y = 0; y < InputToCheck.YSize(); y++)
			{
				const UDungeonTile* inputTile = Input[y][InputToCheck.XSize() - x - 1];
				if (InputToCheck[y][x] != inputTile)
				{
					bCurrentStatus = false;
					break;
				}
			}
			if (!bCurrentStatus)
			{
				break;
			}
		}
		if (bCurrentStatus)
		{
			// Matched this attempt!
			return 3;
		}
	}

	if (InputToCheck.XSize() > 1 && InputToCheck.YSize() > 1)
	{
		// Last attempt -- reversed XY
		bCurrentStatus = true;
		for (int x = 0; x < InputToCheck.XSize(); x++)
		{
			for (int y = 0; y < InputToCheck.YSize(); y++)
			{
				const UDungeonTile* inputTile = Input[InputToCheck.YSize() - y - 1][InputToCheck.XSize() - x - 1];
				if (InputToCheck[y][x] != inputTile)
				{
					bCurrentStatus = false;
					break;
				}
			}
			if (!bCurrentStatus)
			{
				break;
			}
		}
	}

	if (bCurrentStatus)
	{
		return 4;
	}
	else
	{
		return 0;
	}
}
