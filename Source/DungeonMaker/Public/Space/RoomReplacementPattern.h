// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonTile.h"
#include "RoomReplacementPattern.generated.h"

class URoomReplacementPattern;

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
	FDungeonRoom Input;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoom Output;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;

	URoomReplacementPattern();
	UFUNCTION(BlueprintCallable)
	bool FindAndReplace(FDungeonRoom& ReplaceRoom);
protected:
	bool MatchesReplacement(FDungeonRoom& InputToCheck);
};
