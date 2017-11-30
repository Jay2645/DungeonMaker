#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "DungeonMakerEdNode.generated.h"

class UDungeonMakerNode;

UCLASS(MinimalAPI)
class UDungeonMakerEdNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, instanced, Category = "DungeonMaker")
	UDungeonMakerNode* DungeonMakerNode;

	virtual void AllocateDefaultPins() override;
	virtual void NodeConnectionListChanged() override;

	UDungeonMakerEdGraph* GetDungeonMakerEdGraph();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const;

	void SetDungeonMakerNode(UDungeonMakerNode* InNode);

	virtual FText GetDescription() const;

	virtual FLinearColor GetBackgroundColor() const;
};
