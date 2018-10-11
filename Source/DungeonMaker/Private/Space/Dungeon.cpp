// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon.h"
#include "DungeonMaker.h"
#include "Grammar/Grammar.h"
#include <DrawDebugHelpers.h>

// Sets default values
ADungeon::ADungeon()
{
	PrimaryActorTick.bCanEverTick = false;
	Mission = CreateDefaultSubobject<UDungeonMissionGenerator>(TEXT("Dungeon Mission"));
	Space = CreateDefaultSubobject<UDungeonSpaceGenerator>(TEXT("Dungeon Space"));
}

TSet<FIntVector> ADungeon::GetAllTilesOfType(ETileType Type) const
{
	TSet<FIntVector> tileTypes;
	for (int i = 0; i < Space->Floors.Num(); i++)
	{
		tileTypes.Append(Space->Floors[i]->GetAllTilesOfType(Type));
	}
	return tileTypes;
}

// Called when the game starts or when spawned
void ADungeon::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream rng;
	if (bChooseRandomSeedAtRuntime)
	{
		FDateTime now = FDateTime::UtcNow();
		rng = FRandomStream(now.ToUnixTimestamp());
	}
	else
	{
		rng = FRandomStream(Seed);
	}
	UE_LOG(LogMissionGen, Log, TEXT("Creating dungeon out of seed %d."), Seed);
	bool bSuccessfullyMadeDungeon = false;

	do 
	{
		Mission->TryToCreateDungeon(rng);
		bSuccessfullyMadeDungeon = Space->CreateDungeonSpace(Mission->Head, Mission->DungeonSize, rng);
	} while (!bSuccessfullyMadeDungeon);
}