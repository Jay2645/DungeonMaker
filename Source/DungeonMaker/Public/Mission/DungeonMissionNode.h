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

UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonMissionNode : public UObject
{
	GENERATED_BODY()
public:
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
};