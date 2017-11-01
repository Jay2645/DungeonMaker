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

	// Dungeons grow exponentially; we need to create leaves to match
	// Equation is based on fitting {{16, 54}, {43, 81}, {69, 108}}
	// x^2/1378 + (1319 x)/1378 + 26526/689
	Space->CreateDungeonSpace(Mission->Head, Mission->DungeonSize, rng);
	//Space->DrawDebugSpace();
}