// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomHelpers.h"
#include "RoomReplacementPattern.generated.h"

class URoomReplacementPattern;
class UDungeonFloorManager;

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
class DUNGEONMAKER_API URoomReplacementPattern : public UDataAsset
{
	GENERATED_BODY()
public:
	// A series of tiles that serve as input.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoomMetadata Input;
	// A series of tiles that serve as output (the replacement).
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoomMetadata Output;
	// Whether this replacement is placed randomly, or if it'll be the first potential replacement we
	// come across. Random replacement is MUCH slower.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomlyPlaced;
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

	URoomReplacementPattern();
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	bool FindAndReplace(FDungeonRoomMetadata& ReplaceRoom, FRandomStream& Rng);


	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	bool FindAndReplaceFloor(UDungeonFloorManager* ReplaceFloor, FRandomStream& Rng);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles|Replacement")
	float GetActualSelectionChance(ADungeonRoom* InputRoom) const;

private:
	void UpdateFloorTiles(int ReplacementXSize, int ReplacementYSize,
		int XOffset, int YOffset, int Width, int Height, uint8 ReplacementOutput,
		UDungeonFloorManager* ReplaceFloor, FDungeonRoomMetadata &ReplaceRoom);
	uint8 MatchesReplacement(FDungeonRoomMetadata& InputToCheck);
};
