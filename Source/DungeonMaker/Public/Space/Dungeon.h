// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonTile.h"
#include "DungeonMissionGenerator.h"
#include "BSP/BSPLeaf.h"
#include "GameFramework/Actor.h"
#include "Dungeon.generated.h"

UCLASS()
class DUNGEONMAKER_API ADungeon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeon();

	~ADungeon();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonMissionGenerator* Mission;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UDungeonTile* DefaultRoomTile;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void PairNodesToLeaves(TArray<UDungeonMissionNode*>& ToProcess, TSet<UBSPLeaf*>& AvailableLeaves, FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves);


	TMap<UDungeonMissionNode*, FDungeonRoom*> RoomMap;
};
