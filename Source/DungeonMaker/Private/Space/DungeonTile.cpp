// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonTile.h"
#include "DungeonMissionSymbol.h"


FDungeonRoom* FDungeonRoom::GenerateDungeonRoom(const UDungeonMissionSymbol* Symbol, const UDungeonTile* DefaultRoomTile)
{
	FMissionSpaceData averageRoom = Symbol->MinimumRoomSize;
	int32 dungeonXYSize = averageRoom.WallSize;
	int32 dungeonZSize = averageRoom.CeilingHeight;

	FDungeonRoom* room = new FDungeonRoom(dungeonXYSize, dungeonXYSize);
	for (int x = 0; x < room->XSize(); x++)
	{
		for (int y = 0; y < room->YSize(); y++)
		{
			room->Set(x, y, DefaultRoomTile);
		}
	}

	room->Symbol = Symbol;

	FRandomStream rng;
	TArray<FRoomReplacements> roomReplacementPhases = Symbol->RoomReplacementPhases;
	for (int i = 0; i < roomReplacementPhases.Num(); i++)
	{
		while (roomReplacementPhases[i].ReplacementPatterns.Num() > 0)
		{
			int j = rng.RandRange(0, roomReplacementPhases[i].ReplacementPatterns.Num() - 1);
			URoomReplacementPattern* replacement = roomReplacementPhases[i].ReplacementPatterns[j];
			if (replacement->SelectionChance < rng.GetFraction())
			{
				continue;
			}
			if (!replacement->FindAndReplace(*room))
			{
				roomReplacementPhases[i].ReplacementPatterns.RemoveAt(j);
			}
		}
	}
	return room;
}
