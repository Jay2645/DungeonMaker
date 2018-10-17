

#include "DungeonFloorHelpers.h"

FRoomTile& UDungeonFloorHelpers::GetTileAtLocation(ADungeonRoom* Room, const FIntVector& Location)
{
	FDungeonSpace& dungeonSpace = Room->GetDungeon();
	return dungeonSpace.GetHighRes(Location.Z).Get(Location.X, Location.Y);
}
