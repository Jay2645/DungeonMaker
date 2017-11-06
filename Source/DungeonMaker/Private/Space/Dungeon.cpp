// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon.h"
#include "Grammar/Grammar.h"
#include <DrawDebugHelpers.h>

// Sets default values
ADungeon::ADungeon()
{
	PrimaryActorTick.bCanEverTick = false;
	Mission = CreateDefaultSubobject<UDungeonMissionGenerator>(TEXT("Dungeon Mission"));
	Space = CreateDefaultSubobject<UDungeonSpaceGenerator>(TEXT("Dungeon Space"));
}

// Called when the game starts or when spawned
void ADungeon::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream rng(Seed);

	Mission->TryToCreateDungeon(rng);

	Space->CreateDungeonSpace(Mission->Head, Mission->DungeonSize, rng);
}