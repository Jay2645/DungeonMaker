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
			if (MatchesReplacement(roomReplacement))
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
							ReplaceRoom.Set(x, y, Output[localYOffset][localXOffset]);
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

	UE_LOG(LogDungeonGen, Log, TEXT("Replacing floor of size %d x %d."), width, height);

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
				if (MatchesReplacement(roomReplacement))
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
								ReplaceFloor.UpdateTile(location, Output[localYOffset][localXOffset]);
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

bool URoomReplacementPattern::MatchesReplacement(FDungeonRoomMetadata& InputToCheck)
{
	if (InputToCheck.XSize() != Input.XSize() || InputToCheck.YSize() != Input.YSize())
	{
		return false;
	}
	for (int x = 0; x < InputToCheck.XSize(); x++)
	{
		for (int y = 0; y < InputToCheck.YSize(); y++)
		{
			const UDungeonTile* inputTile = Input[y][x];
			if (InputToCheck[y][x] != inputTile)
			{
				return false;
			}
		}
	}
	return true;
}
