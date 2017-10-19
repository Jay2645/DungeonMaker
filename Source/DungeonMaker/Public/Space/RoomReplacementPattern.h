// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonTile.h"
#include "RoomReplacementPattern.generated.h"

class URoomReplacementPattern;
class ADungeonRoom;

USTRUCT(BlueprintType)
struct FRoomReplacements
{
	GENERATED_BODY()
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoomMetadata Input;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FDungeonRoomMetadata Output;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;

	URoomReplacementPattern();
	UFUNCTION(BlueprintCallable)
	bool FindAndReplace(FDungeonRoomMetadata& ReplaceRoom);
	UFUNCTION(BlueprintCallable)
	bool FindAndReplaceFloor(FDungeonFloor& ReplaceFloor);
protected:
	bool MatchesReplacement(FDungeonRoomMetadata& InputToCheck);
};
