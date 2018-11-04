// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomReplacementPattern.h"
#include "DungeonRoom.h"
#include "DungeonFloorManager.h"

URoomReplacementPattern::URoomReplacementPattern()
{
	SelectionChance = 1.0f;
	bCanBeRotated = true;
	bRandomlyPlaced = false;
}

bool URoomReplacementPattern::FindAndReplace(FDungeonSpace& DungeonSpace, ADungeonRoom* Room, FRandomStream& Rng)
{
	check(Room != NULL);

	FIntVector roomTileSpaceLocation = Room->GetRoomLocation();
	FIntVector roomSize = Room->GetRoomSize();

	return FindAndReplaceByLocation(roomTileSpaceLocation, roomSize, DungeonSpace, Rng);
}

bool URoomReplacementPattern::FindAndReplaceByLocation(const FIntVector& RoomTileSpaceLocation, const FIntVector& RoomSize, FDungeonSpace& DungeonSpace, FRandomStream &Rng)
{
	// This will store all possible replacement positions
	// It's used only if we're picking a replacement randomly
	TArray<FIntVector> possibleReplacements;
	checkf(InputPattern.IsNotNull(), TEXT("You didn't specify any input for replacement data for %s!"), *GetName());

	//int32 rotationAmount = 0;
	//do
	//{
		int32 xLocation;
		int32 yLocation;
		int32 zLocation;
		int32 xSize;
		int32 ySize;

		/*switch (rotationAmount)
		{
		case 90:
			// Swap X and Y
			xLocation = RoomTileSpaceLocation.Y;
			yLocation = RoomTileSpaceLocation.X;
			zLocation = RoomTileSpaceLocation.Z;
			xSize = RoomSize.Y;
			ySize = RoomSize.X;
			break;
		case 180:
			// Start at end and go backwards
			xLocation = RoomTileSpaceLocation.X + RoomSize.X;
			yLocation = RoomTileSpaceLocation.Y + RoomSize.Y;
			zLocation = RoomTileSpaceLocation.Z;
			xSize = -RoomSize.X;
			ySize = -RoomSize.Y;
			break;
		case 270:
			// Swap X and Y, start at end and go backwards
			xLocation = RoomTileSpaceLocation.Y + RoomSize.Y;
			yLocation = RoomTileSpaceLocation.X + RoomSize.X;
			zLocation = RoomTileSpaceLocation.Z;
			xSize = -RoomSize.Y;
			ySize = -RoomSize.X;
			break;
		default:*/
			xLocation = RoomTileSpaceLocation.X;
			yLocation = RoomTileSpaceLocation.Y;
			zLocation = RoomTileSpaceLocation.Z;
			xSize = RoomSize.X;
			ySize = RoomSize.Y;
//			break;
//		}

		possibleReplacements.Append(FindPossibleReplacements(DungeonSpace, xLocation, yLocation, zLocation, xSize, ySize));

//		rotationAmount += 90;
		// Continue this loop if:
		// * We can be rotated
		// * We have not reached a full 360 degree rotation yet
		// * We are randomly placed OR we are just going to select the first match, which has not been found yet
	//} while (bCanBeRotated && rotationAmount < 360 && (bRandomlyPlaced || !bRandomlyPlaced && possibleReplacements.Num() == 0));

	if (possibleReplacements.Num() > 0)
	{
		FIntVector replacementPosition;
		if (bRandomlyPlaced)
		{
			// Randomly select a position
			int32 randomVectorID = Rng.RandRange(0, possibleReplacements.Num() - 1);
			replacementPosition = possibleReplacements[randomVectorID];
		}
		else
		{
			// Select the first position we can
			replacementPosition = possibleReplacements[0];
		}

		// Change the tiles
		UpdateFloorTiles(replacementPosition, DungeonSpace);
		return true;
	}
	else
	{
		// No replacements found
		return false;
	}
}

TArray<FIntVector> URoomReplacementPattern::FindPossibleReplacements(FDungeonSpace &DungeonSpace, int32 StartX, int32 StartY, int32 StartZ, int32 XSize, int32 YSize) const
{
	TArray<FIntVector> possibleReplacements;

	UE_LOG(LogSpaceGen, Verbose, TEXT("Checking replacements from (%d, %d, %d) to (%d, %d, %d)."), StartX, StartY, StartZ, StartX + XSize, StartY + YSize, StartZ);

	int xIncrement = 1;
	int yIncrement = 1;

	// If we have negative sizes, we're counting down
	// This ensures that we properly check for patterns going backwards
	if (XSize < 0)
	{
		xIncrement *= -1;
	}
	if (YSize < 0)
	{
		yIncrement *= -1;
	}

	// @TODO: Modify this so we can check above and below each floor if needed
	int z = StartZ;

	// @TODO: Make sure we stay in the right room
	for (int x = StartX; x < StartX + XSize; x += xIncrement)
	{
		for (int y = StartY; y < StartY + YSize; y += yIncrement)
		{
			FIntVector location = FIntVector(x, y, z);
			bool bMatches = true;
			// Iterate over our input pattern, seeing if we can fit all the tiles here
			for (auto& kvp : InputPattern.Pattern)
			{
				FIntVector checkLocation = kvp.Key + location;
				const UDungeonTile* tile = NULL;

				if (DungeonSpace.IsValidLocation(checkLocation))
				{
					tile = DungeonSpace.GetTile(checkLocation);
				}

				if(tile != kvp.Value)
				{
					bMatches = false;
					break;
				}
			}

			// If all tiles match, add this spot to our list of replacement positions
			if (bMatches)
			{
				possibleReplacements.Add(location);
			}

			if (!bRandomlyPlaced && possibleReplacements.Num() > 0)
			{
				break;
			}
		}
		if (!bRandomlyPlaced && possibleReplacements.Num() > 0)
		{
			break;
		}
	}
	return possibleReplacements;
}

bool URoomReplacementPattern::FindAndReplaceFloor(FDungeonSpace& DungeonSpace, int32 DungeonLevel, FRandomStream& Rng)
{
	return FindAndReplaceByLocation(FIntVector(0, 0, 0), DungeonSpace.GetSize(), DungeonSpace, Rng);
}

void URoomReplacementPattern::UpdateFloorTiles(const FIntVector& ReplacementPosition, FDungeonSpace& DungeonSpace, int32 RotationAmount)
{
	// Iterate over our output pattern, replacing all tiles with their updated version
	for (auto& kvp : OutputPattern.Pattern)
	{
		FIntVector checkLocation = kvp.Key + ReplacementPosition;
		DungeonSpace.SetTile(checkLocation, kvp.Value);
	}
}

float URoomReplacementPattern::GetActualSelectionChance(ADungeonRoom* InputRoom) const
{
	return SelectionChance + (InputRoom->GetRoomDifficulty() * SelectionDifficultyModifier);
}