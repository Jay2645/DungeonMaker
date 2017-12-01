// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GraphNode.h"
#include "DungeonMakerNode.h"
#include "DungeonMissionNode.generated.h"

/*
* This is essentially an implementation of a doubly-linked list for Dungeon Mission Symbols.
* This helps represent the graph of associations within rooms in the dungeon itself.
* It differs from a standard doubly-linked list in that a node can be "tightly coupled"
* to a parent, which means they have a significant relationship to their parent node.
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonMissionNode : public UDungeonMakerNode
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeons|Missions")
	UDungeonMakerNode* FindChildNodeFromSymbol(FNumberedGraphSymbol ChildSymbol) const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions")
	void BreakLinkWithNode(const UDungeonMissionNode* Child);
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	FString GetSymbolDescription();

	void AddLinkToNode(UDungeonMissionNode* NewChild, bool bTightlyCoupled);
	int32 GetLevelCount();

	/*static TArray<UDungeonMissionNode*> GetDepthFirstSortedNodes(UDungeonMissionNode* Head, bool bOnlyTightlyCoupled);
	static TArray<UDungeonMissionNode*> GetTopologicalSortedNodes(UDungeonMissionNode* Head);

private:
	static void TopologicalVisit(UDungeonMissionNode* Node, TSet<UDungeonMissionNode*>& Marked,
		TSet<UDungeonMissionNode*>& TemporaryMarked, TArray<UDungeonMissionNode*>& SortedList);
	static TArray<UDungeonMissionNode*> DepthVisit(UDungeonMissionNode* Node, TSet<UDungeonMissionNode*>& Visited, bool bOnlyTightlyCoupled);*/
};