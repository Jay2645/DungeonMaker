

#include "NeighboringMissionSpaceHandler.h"
#include "DungeonMissionSpaceHandler.h"
#include "DungeonMissionSymbol.h"
#include "DungeonMissionNode.h"
#include "DungeonSpaceGenerator.h"

#define INVALID_LOCATION FIntVector(-1, -1, -1)

void UNeighboringMissionSpaceHandler::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	FMissionSpaceHelper spaceHelper = FMissionSpaceHelper(Rng, StartLocation);
	TMap<FIntVector, FIntVector> availableRooms;
	availableRooms.Add(StartLocation, INVALID_LOCATION);
	PairNodesToRooms(Head, availableRooms, spaceHelper, false, SymbolCount);
}

bool UNeighboringMissionSpaceHandler::PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms,
	FMissionSpaceHelper& SpaceHelper, bool bIsTightCoupling, int32 TotalSymbolCount)
{
	// This is the real "meat and potatoes" of mapping the Dungeon Mission to the Dungeon Space.
	// It will take a given node and assign it to a room. It also will go and check all children
	// of that node, assigning them to rooms as well, if needed.
	
	// Check input
	if (CheckCanSkipProcessing(Node, SpaceHelper))
	{
		UE_LOG(LogSpaceGen, Verbose, TEXT("Skipping processing for %s."), *Node->GetSymbolDescription());
		return true;
	}
	if (!CheckInputIsValid(AvailableRooms, bIsTightCoupling, Node, SpaceHelper))
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("Could not place %s due to invalid input."), *Node->GetSymbolDescription());
		return false;
	}

	// Input valid; let's go

	UE_LOG(LogSpaceGen, Log, TEXT("Creating room for %s! Rooms available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableRooms.Num(), Node->ChildrenNodes.Num());
	// Find an open room to add this to
	FRoomPairing roomLocation;
	if (bIsTightCoupling)
	{
		// A tightly-coupled room must be placed adjacent to the last room we placed.
		roomLocation = GetOpenRoom(Node, AvailableRooms, SpaceHelper);
	}
	else
	{
		// This room can be placed alongside any room we have already placed.
		roomLocation = GetOpenRoom(Node, SpaceHelper.OpenRooms, SpaceHelper);
	}

	// The key of the roomLocation is where the next room will be placed
	// The value is what room is adjacent to it

	// Mark the next room location as invalid
	// That way, we don't reuse it if something goes wrong
	FIntVector nextLocation = roomLocation.ChildRoom;
	SpaceHelper.MarkAsProcessed(nextLocation);
	// Mark this node as processed ahead of time
	// If we go recursive depth-first instead of breadth-first, it will
	// mess with things if we haven't been marked as processed
	SpaceHelper.MarkAsProcessed(Node);

	// Grab all our neighbor rooms, excluding those which have already been processed
	TMap<FIntVector, FIntVector> roomNeighborMap = GetRoomNeighbors(nextLocation, SpaceHelper);

	// Process all our child nodes
	TArray<UDungeonMissionNode*> nextToProcess;
	for (UDungeonMakerNode* childNode : Node->ChildrenNodes)
	{
		if (childNode->bTightlyCoupledToParent)
		{
			// If we're tightly coupled to our parent, ensure we get added to a neighboring leaf
			// We do this by going depth-first
			UE_LOG(LogSpaceGen, Verbose, TEXT("Placing tightly-coupled room %s next to parent %s."), *childNode->GetNodeTitle(), *Node->GetNodeTitle());
			bool bSuccesfullyPairedChild = PairNodesToRooms((UDungeonMissionNode*)childNode, roomNeighborMap, SpaceHelper, true, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out of our current node
				SpaceHelper.MarkAsUnprocessed(Node);
				// Restart processing our current node -- now that the leaf we tried has been
				// marked as invalid, we're guaranteed to choose a different leaf next time
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child rooms to match our tightly-coupled rooms."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, SpaceHelper, bIsTightCoupling, TotalSymbolCount);
			}
		}
		else
		{
			// Doesn't matter where we get placed
			// Add our child to the list of nodes to process next
			// These will be processed breadth-first in the next step
			nextToProcess.Add((UDungeonMissionNode*)childNode);
		}
	}

	// If we're allowed to have children, mark our neighbors as available
	if (((UDungeonMissionSymbol*)Node->NodeType)->bAllowedToHaveChildren)
	{
		AvailableRooms.Append(roomNeighborMap);
	}
	// List all the rooms we have open
	SpaceHelper.AddOpenRooms(AvailableRooms);


	// Step 2: Process all non-tightly coupled nodes
	TArray<UDungeonMissionNode*> deferredNodes;
	UE_LOG(LogSpaceGen, Verbose, TEXT("%s has %d children to process."), *Node->GetNodeTitle(), nextToProcess.Num());
	for (int i = 0; i < nextToProcess.Num(); i++)
	{
		// If we're not tightly coupled, ensure that we have all our required parents generated
		bool bDeferProcessing = false;
		for (int j = 0; j < nextToProcess[i]->ParentNodes.Num(); j++)
		{
			if (!SpaceHelper.HasProcessed((UDungeonMissionNode*)nextToProcess[i]->ParentNodes[j]))
			{
				bDeferProcessing = true;
				break;
			}
		}
		if (bDeferProcessing)
		{
			// Defer processing a bit
			deferredNodes.Add(nextToProcess[i]);
			continue;
		}

		UE_LOG(LogSpaceGen, Verbose, TEXT("Placing loosely-coupled room %s. Parent: %s."), *nextToProcess[i]->GetNodeTitle(), *Node->GetNodeTitle());
		bool bSuccesfullyPairedChild = PairNodesToRooms(nextToProcess[i], AvailableRooms, SpaceHelper, false, TotalSymbolCount);
		if (!bSuccesfullyPairedChild)
		{
			// Failed to find a child leaf; back out
			SpaceHelper.MarkAsUnprocessed(Node);
			// Restart -- next time, we'll select a different leaf
			UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
			return PairNodesToRooms(Node, AvailableRooms, SpaceHelper, bIsTightCoupling, TotalSymbolCount);
		}
	}


	// Step 3: Attempt to place our deferred nodes
	TMap<UDungeonMissionNode*, uint8> attemptCount;
	const uint8 MAX_ATTEMPT_COUNT = 12;
	while (deferredNodes.Num() > 0)
	{
		UDungeonMissionNode* currentNode = deferredNodes[0];
		deferredNodes.RemoveAt(0);
		if (attemptCount.Contains(currentNode))
		{
			attemptCount[currentNode]++;
		}
		else
		{
			attemptCount.Add(currentNode, 1);
		}

		bool bDeferProcessing = false;
		for (int j = 0; j < currentNode->ParentNodes.Num(); j++)
		{
			if (!SpaceHelper.HasProcessed((UDungeonMissionNode*)currentNode->ParentNodes[j]))
			{
				bDeferProcessing = true;
				break;
			}
		}
		if (!bDeferProcessing)
		{
			bool bSuccesfullyPairedChild = PairNodesToRooms(currentNode, AvailableRooms, SpaceHelper, false, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				SpaceHelper.MarkAsUnprocessed(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, SpaceHelper, bIsTightCoupling, TotalSymbolCount);
			}
			else
			{
				// Stop keeping track of this
				attemptCount.Remove(currentNode);
			}
		}
		else
		{
			if (attemptCount[currentNode] < MAX_ATTEMPT_COUNT)
			{
				deferredNodes.Add(currentNode);
			}
		}
	}

	// Make the actual room
	FFloorRoom room = MakeFloorRoom(Node, nextLocation, SpaceHelper.Rng, TotalSymbolCount);
	SetRoom(room);
	
	// Update neighbors
	UpdateNeighbors(roomLocation, bIsTightCoupling);

	return true;
}

TMap<FIntVector, FIntVector> UNeighboringMissionSpaceHandler::GetRoomNeighbors(FIntVector RoomLocation, FMissionSpaceHelper &SpaceHelper)
{
	// Grab all our neighbor rooms, excluding those which have already been processed
	TSet<FIntVector> neighboringRooms = GetAvailableLocations(RoomLocation, SpaceHelper.GetProcessedRooms());
	TMap<FIntVector, FIntVector> roomNeighborMap;
	// Map us to be the neighbor to all our neighbors
	for (FIntVector neighbor : neighboringRooms)
	{
		roomNeighborMap.Add(neighbor, RoomLocation);
	}
	return roomNeighborMap;
}