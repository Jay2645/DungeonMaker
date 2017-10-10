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
	// The maximum size of any room in this dungeon, in meters.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxRoomSize = 24;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 DungeonSizeMultiplier = 3;
	UPROPERTY()
	UBSPLeaf* RootLeaf;

protected:
	// Where the leaf chain starts
	UPROPERTY()
	UBSPLeaf* StartLeaf;


protected:
	UPROPERTY()
	TSet<UBSPLeaf*> Leaves;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void CreateRoomMap();

	bool PairNodesToLeaves(UDungeonMissionNode* ToProcess, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves, UBSPLeaf* EntranceLeaf, TSet<FBSPLink>& AllOpenLeaves, bool bIsTightCoupling = false);

	FBSPLink GetOpenLeaf(UDungeonMissionNode* Node, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UBSPLeaf*>& ProcessedLeaves);

	TMap<UDungeonMissionNode*, FDungeonRoom*> RoomMap;
};
