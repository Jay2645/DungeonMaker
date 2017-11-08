// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonTile.h"
#include "DungeonMissionGenerator.h"
#include "DungeonSpaceGenerator.h"
#include "GameFramework/Actor.h"
#include "Dungeon.generated.h"

UCLASS()
class DUNGEONMAKER_API ADungeon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeon();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonMissionGenerator* Mission;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonSpaceGenerator* Space;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Seed = 1234;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 DungeonSizeMultiplier = 3;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
