

#include "NeighboringMissionSpaceHandler.h"


void UNeighboringMissionSpaceHandler::GenerateDungeonRooms(UDungeonMissionNode* Head, FIntVector StartLocation, FRandomStream &Rng, int32 SymbolCount)
{
	TMap<FIntVector, FIntVector> availableRooms;
	TSet<UDungeonMissionNode*> processedNodes;
	TSet<FIntVector> processedRooms;
	TMap<FIntVector, FIntVector> openRooms;

	availableRooms.Add(StartLocation, FIntVector(-1, -1, -1));
	openRooms.Add(StartLocation, FIntVector(-1, -1, -1));

	PairNodesToRooms(Head, availableRooms, Rng, processedNodes, processedRooms, StartLocation, openRooms, false, SymbolCount);
}

bool UNeighboringMissionSpaceHandler::PairNodesToRooms(UDungeonMissionNode* Node, TMap<FIntVector, FIntVector>& AvailableRooms,
	FRandomStream& Rng, TSet<UDungeonMissionNode*>& ProcessedNodes, TSet<FIntVector>& ProcessedRooms,
	FIntVector EntranceRoom, TMap<FIntVector, FIntVector>& AllOpenRooms,
	bool bIsTightCoupling, int32 TotalSymbolCount)
{
	if (ProcessedNodes.Contains(Node))
	{
		// Already processed this node
		return true;
	}
	if (Node == NULL)
	{
		// No rooms to pair
		UE_LOG(LogSpaceGen, Error, TEXT("Null node was provided to the Mission Space Handler!"));
		return true;
	}
	if (((UDungeonMissionSymbol*)Node->NodeType)->RoomTypes.Num() == 0)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Mission Space Handler tried handling %s, which had no room types defined!"), *Node->GetNodeTitle());
		return true;
	}

	for (int i = 0; i < Node->ParentNodes.Num(); i++)
	{
		if (!ProcessedNodes.Contains((UDungeonMissionNode*)Node->ParentNodes[i]))
		{
			// We haven't processed all our parent nodes yet!
			// We should be processed further on down the line, once our next parent node
			// finishes being processed.
			UE_LOG(LogSpaceGen, Log, TEXT("Deferring processing of %s because not all its parents have been processed yet."), *Node->GetSymbolDescription());
			return true;
		}
	}
	if (AvailableRooms.Num() == 0 && bIsTightCoupling)
	{
		// Out of leaves to process
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is tightly coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}
	if (AllOpenRooms.Num() == 0 && !bIsTightCoupling)
	{
		UE_LOG(LogSpaceGen, Warning, TEXT("%s is loosely coupled to its parent, but ran out of leaves to process."), *Node->GetSymbolDescription());
		return false;
	}

	UE_LOG(LogSpaceGen, Log, TEXT("Creating room for %s! Rooms available: %d, Room Children: %d"), *Node->GetSymbolDescription(), AvailableRooms.Num(), Node->ChildrenNodes.Num());
	// Find an open room to add this to
	TKeyValuePair<FIntVector, FIntVector> roomLocation;
	if (bIsTightCoupling)
	{
		roomLocation = GetOpenRoom(Node, AvailableRooms, Rng, ProcessedRooms);
	}
	else
	{
		roomLocation = GetOpenRoom(Node, AllOpenRooms, Rng, ProcessedRooms);
	}

	ProcessedRooms.Add(roomLocation.Key);
	ProcessedNodes.Add(Node);

	// Grab all our neighbor rooms, excluding those which have already been processed
	TSet<FIntVector> neighboringRooms = GetAvailableLocations(roomLocation.Key, ProcessedRooms);
	TMap<FIntVector, FIntVector> roomNeighborMap;
	// Map us to be the neighbor to all our neighbors
	for (FIntVector neighbor : neighboringRooms)
	{
		roomNeighborMap.Add(neighbor, roomLocation.Key);
	}

	// Find all the tightly coupled nodes attached to our current node
	TArray<UDungeonMissionNode*> nextToProcess;
	for (UDungeonMakerNode* neighborNode : Node->ChildrenNodes)
	{
		if (neighborNode->bTightlyCoupledToParent)
		{
			// If we're tightly coupled to our parent, ensure we get added to a neighboring leaf
			UE_LOG(LogSpaceGen, Log, TEXT("Placing tightly-coupled room %s next to parent %s."), *neighborNode->GetNodeTitle(), *Node->GetNodeTitle());
			bool bSuccesfullyPairedChild = PairNodesToRooms((UDungeonMissionNode*)neighborNode, roomNeighborMap, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, true, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child rooms to match our tightly-coupled rooms."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
			}
		}
		else
		{
			nextToProcess.Add((UDungeonMissionNode*)neighborNode);
		}
	}

	if (((UDungeonMissionSymbol*)Node->NodeType)->bAllowedToHaveChildren)
	{
		AvailableRooms.Append(roomNeighborMap);
	}
	AllOpenRooms.Append(AvailableRooms);
	TArray<UDungeonMissionNode*> deferredNodes;

	// Now we process all non-tightly coupled nodes
	UE_LOG(LogSpaceGen, Log, TEXT("%s has %d children to process."), *Node->GetNodeTitle(), nextToProcess.Num());
	for (int i = 0; i < nextToProcess.Num(); i++)
	{
		// If we're not tightly coupled, ensure that we have all our required parents generated
		bool bDeferProcessing = false;
		for (int j = 0; j < nextToProcess[i]->ParentNodes.Num(); j++)
		{
			if (!ProcessedNodes.Contains((UDungeonMissionNode*)nextToProcess[i]->ParentNodes[j]))
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

		UE_LOG(LogSpaceGen, Log, TEXT("Placing loosely-coupled room %s. Parent: %s."), *nextToProcess[i]->GetNodeTitle(), *Node->GetNodeTitle());
		bool bSuccesfullyPairedChild = PairNodesToRooms(nextToProcess[i], AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, false, TotalSymbolCount);
		if (!bSuccesfullyPairedChild)
		{
			// Failed to find a child leaf; back out
			ProcessedNodes.Remove(Node);
			// Restart -- next time, we'll select a different leaf
			UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
			return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
		}
	}

	// Attempt to place our deferred nodes
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
			if (!ProcessedNodes.Contains((UDungeonMissionNode*)currentNode->ParentNodes[j]))
			{
				bDeferProcessing = true;
				break;
			}
		}
		if (!bDeferProcessing)
		{
			bool bSuccesfullyPairedChild = PairNodesToRooms(currentNode, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, roomLocation.Key, AllOpenRooms, false, TotalSymbolCount);
			if (!bSuccesfullyPairedChild)
			{
				// Failed to find a child leaf; back out
				ProcessedNodes.Remove(Node);
				// Restart -- next time, we'll select a different leaf
				UE_LOG(LogSpaceGen, Warning, TEXT("Restarting processing for %s because we couldn't find enough child leaves."), *Node->GetSymbolDescription());
				return PairNodesToRooms(Node, AvailableRooms, Rng, ProcessedNodes, ProcessedRooms, EntranceRoom, AllOpenRooms, bIsTightCoupling, TotalSymbolCount);
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
	FFloorRoom room = MakeFloorRoom(Node, roomLocation.Key, Rng, TotalSymbolCount);
	SetRoom(room);
	
	// Don't bother setting neighbors if one of the neighbors would be invalid
	if (IsLocationValid(roomLocation.Key) && IsLocationValid(roomLocation.Value))
	{
		// Link the children
		if (bIsTightCoupling)
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z][roomLocation.Key.Y][roomLocation.Key.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z][roomLocation.Value.Y][roomLocation.Value.X].NeighboringTightlyCoupledRooms.Add(roomLocation.Key);
		}
		else
		{
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Key.Z][roomLocation.Key.Y][roomLocation.Key.X].NeighboringRooms.Add(roomLocation.Value);
			DungeonSpaceGenerator->DungeonSpace[roomLocation.Value.Z][roomLocation.Value.Y][roomLocation.Value.X].NeighboringRooms.Add(roomLocation.Key);
		}
	}

	return true;
}