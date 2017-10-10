// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BSPLeaf.h"
#include "DungeonSpaceGenerator.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonSpaceGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDungeonSpaceGenerator();

	UBSPLeaf* RootLeaf;
	UBSPLeaf* StartLeaf;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UDungeonTile* DefaultRoomTile;

	// The maximum size of any room in this dungeon, in meters.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxRoomSize = 24;

public:	
	void CreateDungeonSpace(int32 DungeonSize, UDungeonMissionNode* Head, FRandomStream& Rng);
	void DrawDebugSpace(AActor* ReferenceActor) const;

protected:
	bool PairNodesToLeaves(UDungeonMissionNode* Node, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<UBSPLeaf*>& ProcessedLeaves, UBSPLeaf* EntranceLeaf, TSet<FBSPLink>& AllOpenLeaves, bool bIsTightCoupling = false);
	FBSPLink GetOpenLeaf(UDungeonMissionNode* Node, TSet<FBSPLink>& AvailableLeaves, FRandomStream& Rng, TSet<UBSPLeaf*>& ProcessedLeaves);
};
