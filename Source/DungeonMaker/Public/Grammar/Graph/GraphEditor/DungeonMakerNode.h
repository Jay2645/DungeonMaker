#pragma once

#include "CoreMinimal.h"
#include "Grammar/Graph/GraphNode.h"
#include "DungeonMakerNode.generated.h"

class UDungeonMakerGraph;

UCLASS(Blueprintable)
class DUNGEONMAKER_API UDungeonMakerNode : public UObject
{
	GENERATED_BODY()

public:
	UDungeonMakerNode();
	virtual ~UDungeonMakerNode();

	// The type of node represented by this graph.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Graph Node")
	UGraphNode* NodeType;

	// The ID of this node after replacement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Graph Node", meta = (ClampMin = "1"))
	int32 NodeID;

	// The ID of the input node this node will replace, or 0 if it doesn't replace any nodes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Graph Node")
	int32 InputNodeID;

	// Is this node "tightly coupled" to its parent node?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Graph Node")
	bool bTightlyCoupledToParent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Graph Node")
	FString CustomNodeTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Graph Node")
	TArray<UDungeonMakerNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Graph Node")
	TArray<UDungeonMakerNode*> ChildrenNodes;

#if WITH_EDITOR
	UPROPERTY()
	FLinearColor DefaultBackgroundColor;
	UPROPERTY()
	FLinearColor TightlyCoupledBackgroundColor;

	FLinearColor GetBackgroundColor() const;
#endif

	//////////////////////////////////////////////////////////////////////////
	// ufunctions
	UFUNCTION(BlueprintPure, Category = "DungeonMakerNode")
	FString GetNodeTitle();

	UFUNCTION(BlueprintPure, Category = "DungeonMakerNode")
	FNumberedGraphSymbol ToGraphSymbol() const;
	
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	FString ToString(int32 IndentLevel = 4, bool bPrintChildren = true);

	bool IsChildOf(UDungeonMakerNode* ParentSymbol) const;
	//////////////////////////////////////////////////////////////////////////
	UDungeonMakerGraph* GetGraph();
};
