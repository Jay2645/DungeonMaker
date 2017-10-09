// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Grammar/GrammarAlphabet.h"
#include "RoomReplacementPattern.h"
#include "DungeonMissionSymbol.generated.h"

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FMissionSpaceData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "3"))
	int32 WallSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 CeilingHeight;

	FMissionSpaceData()
	{
		WallSize = 3;
		CeilingHeight = 1;
	}
};

/**
 * 
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonMissionSymbol : public UGraphNode
{
	GENERATED_BODY()
public:
	// What is the smallest this room could possibly be?
	// Only used if this is a terminal symbol.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMissionSpaceData MinimumRoomSize;
	// This is a set of replacement patterns that governs how the
	// room gets decorated.
	// Only used if this is a terminal symbol.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FRoomReplacements> RoomReplacementPhases;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAllowedToHaveChildren = true;
};
