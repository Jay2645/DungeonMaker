#include "GroundScatterItem.h"

UGroundScatterItem::UGroundScatterItem()

{
	bUseRandomCount = false;
	bUseRandomLocation = true;
	bConformToGrid = true;
	bPlaceAdjacentToNextRooms = true;
	bPlaceAdjacentToPriorRooms = true;
	MinCount = 0;
	MaxCount = 255;
	SkipTiles = 0;
}