// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionSymbol.h"
#include "DungeonRoom.h"


UDungeonMissionSymbol::UDungeonMissionSymbol()
{
	bAllowedToHaveChildren = true;
}

TSubclassOf<ADungeonRoom> UDungeonMissionSymbol::GetRoomType(FRandomStream& Rng) const
{
	if (RoomTypes.Num() == 0)
	{
		return ADungeonRoom::StaticClass();
	}
	else
	{
		return RoomTypes[Rng.RandRange(0, RoomTypes.Num() - 1)];
	}
}
