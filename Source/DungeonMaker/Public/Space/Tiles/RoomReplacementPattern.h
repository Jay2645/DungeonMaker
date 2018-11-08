// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"

#include "DungeonTile.h"
#include "DungeonFloor.h"

#include "RoomReplacementPattern.generated.h"

class URoomReplacementPattern;
class UDungeonFloorManager;

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonRow
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<const UDungeonTile*> DungeonTiles;

	FDungeonRow()
	{
		DungeonTiles = TArray<const UDungeonTile*>();
	}
	FDungeonRow(int Size)
	{
		DungeonTiles.SetNum(Size);
		for (int i = 0; i < Size; i++)
		{
			DungeonTiles[i] = NULL;
		}
	}

	TSet<const UDungeonTile*> FindAllTiles()
	{
		TSet<const UDungeonTile*> tiles;
		tiles.Append(DungeonTiles);
		tiles.Remove(NULL);
		return tiles;
	}

	void Set(int Index, const UDungeonTile* Tile)
	{
		DungeonTiles[Index] = Tile;
	}

	const UDungeonTile* operator[] (int Index)
	{
		return DungeonTiles[Index];
	}

	int Num() const
	{
		return DungeonTiles.Num();
	}

	FString ToString() const
	{
		FString output;
		for (int i = 0; i < DungeonTiles.Num(); i++)
		{
			if (DungeonTiles[i] == NULL)
			{
				output += 'X';
			}
			else
			{
				output += DungeonTiles[i]->TileID.ToString();
			}
		}
		return output;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonRoomMetadata
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonRow> DungeonRows;

	FDungeonRoomMetadata()
	{
		DungeonRows = TArray<FDungeonRow>();
	}

	FDungeonRoomMetadata(int SizeX, int SizeY)
	{
		check(SizeX >= 0);
		check(SizeY >= 0);
		DungeonRows.SetNum(SizeY);
		for (int i = 0; i < SizeY; i++)
		{
			DungeonRows[i] = FDungeonRow(SizeX);
		}
	}

	FDungeonRow& operator[] (int Index)
	{
		return DungeonRows[Index];
	}

	TSet<const UDungeonTile*> FindAllTiles()
	{
		TSet<const UDungeonTile*> tiles;
		for (int i = 0; i < DungeonRows.Num(); i++)
		{
			tiles.Append(DungeonRows[i].FindAllTiles());
		}
		return tiles;
	}

	void Set(int X, int Y, const UDungeonTile* Tile)
	{
		DungeonRows[Y].Set(X, Tile);
	}

	int XSize() const
	{
		if (DungeonRows.Num() == 0)
		{
			return 0;
		}
		else
		{
			return DungeonRows[0].Num();
		}
	}

	int YSize() const
	{
		return DungeonRows.Num();
	}

	FString ToString() const
	{
		FString output = "";
		for (int i = 0; i < YSize(); i++)
		{
			output += DungeonRows[i].ToString();
			output += "\n";
		}
		return output;
	}
	bool IsNotNull()
	{
		if (YSize() == 0 || XSize() == 0)
		{
			return false;
		}
		for (int x = 0; x < XSize(); x++)
		{
			for (int y = 0; y < YSize(); y++)
			{
				if (DungeonRows[y][x] != NULL)
				{
					return true;
				}
			}
		}
		return false;
	}

	FColor DrawRoom(AActor* ContextObject, FIntVector Position);
};

USTRUCT(BlueprintType)
struct FTilePattern
{
	GENERATED_BODY()
public:
	// A pattern of tiles to match.
	// The "center" of the pattern should be located at (0, 0, 0).
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FIntVector, const UDungeonTile*> Pattern;

	bool IsNotNull() const
	{
		return Pattern.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct FRoomReplacements
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<URoomReplacementPattern*> ReplacementPatterns;

	FRoomReplacements()
	{
		ReplacementPatterns = TArray<URoomReplacementPattern*>();
	}
};

/**
 * 
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API URoomReplacementPattern : public UDataAsset, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// A series of tiles that serve as input.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTilePattern InputPattern;
	// A series of tiles that serve as output (the replacement).
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTilePattern OutputPattern;

	// Whether this replacement is placed randomly, or if it'll be the first potential replacement we
	// come across. Random replacement is MUCH slower.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomlyPlaced;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanBeRotated;
	// How many replacements to place, or 0 if we can place as many as we want.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MaxReplacementCount;
	// How likely it is that we will select this replacement.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;
	// A modifier that scales based on the difficulty of the room being placed in.
	// This modifier winds up being added to the selection chance.
	// As an example, a room with difficulty 0.5, with a base selection chance of 0.2 and a
	// difficulty modifier of 0.4 will have an actual selection chance of 0.4 ((0.4 * 0.5) + 0.2).
	// This modifier can be negative, but the end result will be clamped between 0 and 1.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SelectionDifficultyModifier;

	// Any metadata regarding this replacement pattern.
	// Metadata may be used to select a more appropriate pattern in "themed" dungeons.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer PatternTags;

public:
	URoomReplacementPattern();

public:
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	bool FindAndReplace(FDungeonSpace& DungeonSpace, ADungeonRoom* Room, FRandomStream& Rng);

	TArray<FIntVector> FindPossibleReplacements(FDungeonSpace &DungeonSpace, int32 StartX, int32 StartY, int32 StartZ, int32 XSize, int32 YSize) const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	bool FindAndReplaceFloor(FDungeonSpace& DungeonSpace, int32 DungeonLevel, FRandomStream& Rng);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	float GetActualSelectionChance(ADungeonRoom* InputRoom) const;

	/**
	* Get any owned gameplay tags on the asset
	*
	* @param OutTags	[OUT] Set of tags on the asset
	*/
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = PatternTags;
	}

private:
	bool FindAndReplaceByLocation(const FIntVector& RoomTileSpaceLocation, const FIntVector& RoomSize, FDungeonSpace& DungeonSpace, FRandomStream &Rng);
	void UpdateFloorTiles(const FIntVector& ReplacementPosition, FDungeonSpace& DungeonSpace, int32 RotationAmount = 0);
};
