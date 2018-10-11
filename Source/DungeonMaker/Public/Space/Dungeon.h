// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DungeonTile.h"
#include "DungeonMissionGenerator.h"
#include "DungeonSpaceGenerator.h"

#include "Dungeon.generated.h"

UCLASS()
class DUNGEONMAKER_API ADungeon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeon();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Mission")
	UDungeonMissionGenerator* Mission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Space")
	UDungeonSpaceGenerator* Space;

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	int32 Seed = 1234;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	bool bDebugMission = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dungeon")
	bool bChooseRandomSeedAtRuntime = false;

public:
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	TSet<FIntVector> GetAllTilesOfType(ETileType Type) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
