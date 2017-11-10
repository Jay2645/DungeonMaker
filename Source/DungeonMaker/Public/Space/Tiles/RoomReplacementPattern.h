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
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoomMetadata Input;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonRoomMetadata Output;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MaxReplacementCount;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;

	URoomReplacementPattern();
	UFUNCTION(BlueprintCallable)
	bool FindAndReplace(FDungeonRoomMetadata& ReplaceRoom);
	UFUNCTION(BlueprintCallable)
	bool FindAndReplaceFloor(UDungeonFloorManager* ReplaceFloor);
protected:
	uint8 MatchesReplacement(FDungeonRoomMetadata& InputToCheck);
};
