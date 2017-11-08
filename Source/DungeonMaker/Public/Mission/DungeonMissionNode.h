// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GraphNode.h"
#include "DungeonMissionNode.generated.h"

class UDungeonMissionNode;

USTRUCT(BlueprintType)
struct FMissionNodeData
{
	GENERATED_BODY()
public:
	FMissionNodeData()
	{
		Node = NULL;
		bTightlyCoupledToParent = false;
	}

	UPROPERTY(EditAnywhere)
	UDungeonMissionNode* Node;
	UPROPERTY(EditAnywhere)
	bool bTightlyCoupledToParent;

	bool operator==(const FMissionNodeData& Other) const
	{
		return Node == Other.Node;
	}

	friend uint32 GetTypeHash(const FMissionNodeData& Other)
	{
		return GetTypeHash(Other.Node);
	}
};

/*
* This is essentially an implementation of a doubly-linked list for Dungeon Mission Symbols.
* This helps represent the graph of associations within rooms in the dungeon itself.
* It differs from a standard doubly-linked list in that a node can be "tightly coupled"
* to a parent, which means they have a significant relationship to their parent node.
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonMissionNode : public UObject
{
	GENERATED_BODY()
public:
	// The symbol associated with this node.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNumberedGraphSymbol Symbol;
	// True if we are tightly coupled to any of our parents
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTightlyCoupledToParent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSet<UDungeonMissionNode*> ParentNodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FMissionNodeData> NextNodes;

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeons|Missions")
	UDungeonMissionNode* FindChildNodeFromSymbol(FNumberedGraphSymbol ChildSymbol) const;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions")
	void BreakLinkWithNode(const UDungeonMissionNode* Child);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	void PrintNode(int32 IndentLevel = 4);
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	FString GetSymbolDescription();

	int32 GetLevelCount();
	bool IsChildOf(UDungeonMissionNode* ParentSymbol) const;

	static TArray<UDungeonMissionNode*> GetDepthFirstSortedNodes(UDungeonMissionNode* Head, bool bOnlyTightlyCoupled);
	static TArray<UDungeonMissionNode*> GetTopologicalSortedNodes(UDungeonMissionNode* Head);

private:
	static void TopologicalVisit(UDungeonMissionNode* Node, TSet<UDungeonMissionNode*>& Marked,
		TSet<UDungeonMissionNode*>& TemporaryMarked, TArray<UDungeonMissionNode*>& SortedList);
	static TArray<UDungeonMissionNode*> DepthVisit(UDungeonMissionNode* Node, TSet<UDungeonMissionNode*>& Visited, bool bOnlyTightlyCoupled);
};