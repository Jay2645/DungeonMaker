// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomReplacementPattern.h"
#include "DungeonRoom.h"

URoomReplacementPattern::URoomReplacementPattern()
{
	SelectionChance = 1.0f;
}

bool URoomReplacementPattern::FindAndReplace(FDungeonRoomMetadata& ReplaceRoom)
{
	checkf(Input.IsNotNull(), TEXT("You didn't specify any input for replacement data!"));

	int replacementXSize = Input.XSize();
	int replacementYSize = Input.YSize();

	FDungeonRoomMetadata roomReplacement = FDungeonRoomMetadata(replacementXSize, replacementYSize);

	int xOffset = -replacementXSize;
	int yOffset = -replacementYSize;

	int width = ReplaceRoom.XSize();
	int height = ReplaceRoom.YSize();

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
				for (int localXOffset = 0; localXOffset < replacementXSize; localXOffset++)
				{
					int x = xOffset + localXOffset;
					for (int localYOffset = 0; localYOffset < replacementYSize; localYOffset++)
					{
						int y = yOffset + localYOffset;
						if (x < 0 || x >= width || y < 0 || y >= height)
						{
							// Pass
						}
						else
						{
							int32 outputOffsetX;
							int32 outputOffsetY;
							if (replacementOutput == 1)
							{
								outputOffsetX = localXOffset;
								outputOffsetY = localYOffset;
							}
							else if (replacementOutput == 2)
							{
								outputOffsetX = localXOffset;
								outputOffsetY = replacementYSize - localYOffset - 1;
							}
							else if (replacementOutput == 3)
							{
								outputOffsetX = replacementXSize - localXOffset - 1;
								outputOffsetY = localYOffset;
							}
							else if (replacementOutput == 4)
							{
								outputOffsetX = replacementXSize - localXOffset - 1;
								outputOffsetY = replacementYSize - localYOffset - 1;
							}
							else
							{
								outputOffsetX = 0;
								outputOffsetY = 0;
								checkNoEntry();
							}
							ReplaceRoom.Set(x, y, Output[outputOffsetY][outputOffsetX]);
						}
					}
				}
				return true;
			}
			xOffset++;
		}
		yOffset++;
		xOffset = -replacementXSize;
	}

	return false;
}

bool URoomReplacementPattern::FindAndReplaceFloor(FDungeonFloor& ReplaceFloor)
{
	checkf(Input.IsNotNull(), TEXT("You didn't specify any input for replacement data!"));

	int replacementXSize = Input.XSize();
	int replacementYSize = Input.YSize();

	FDungeonRoomMetadata roomReplacement = FDungeonRoomMetadata(replacementXSize, replacementYSize);

	int xOffset = -replacementXSize;
	int yOffset = -replacementYSize;

	int width = ReplaceFloor.XSize();
	int height = ReplaceFloor.YSize();

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
						FIntVector location = FIntVector(x, y, 0);
						tile = ReplaceFloor.GetTileAt(location);
					}
					roomReplacement.Set(localXOffset, localYOffset, tile);
				}
			}
			if (roomReplacement.IsNotNull())
			{
				uint8 replacementOutput = MatchesReplacement(roomReplacement);
				if (replacementOutput != 0)
				{
					for (int localXOffset = 0; localXOffset < replacementXSize; localXOffset++)
					{
						int x = xOffset + localXOffset;
						for (int localYOffset = 0; localYOffset < replacementYSize; localYOffset++)
						{
							int y = yOffset + localYOffset;
							if (x < 0 || x >= width || y < 0 || y >= height)
							{
								// Pass
							}
							else
							{
								FIntVector location = FIntVector(x, y, 0);
								int32 outputOffsetX;
								int32 outputOffsetY;
								if (replacementOutput == 1)
								{
									outputOffsetX = localXOffset;
									outputOffsetY = localYOffset;
								}
								else if (replacementOutput == 2)
								{
									outputOffsetX = localXOffset;
									outputOffsetY = replacementYSize - localYOffset - 1;
								}
								else if (replacementOutput == 3)
								{
									outputOffsetX = replacementXSize - localXOffset - 1;
									outputOffsetY = localYOffset;
								}
								else if (replacementOutput == 4)
								{
									outputOffsetX = replacementXSize - localXOffset - 1;
									outputOffsetY = replacementYSize - localYOffset - 1;
								}
								else
								{
									outputOffsetX = 0;
									outputOffsetY = 0;
									checkNoEntry();
								}
								ReplaceFloor.UpdateTile(location, Output[outputOffsetY][outputOffsetX]);
							}
						}
					}
					return true;
				}
			}
			xOffset++;
		}
		yOffset++;
		xOffset = -replacementXSize;
	}

	return false;
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
