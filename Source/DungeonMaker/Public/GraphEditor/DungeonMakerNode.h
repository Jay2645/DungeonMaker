#pragma once

#include "CoreMinimal.h"
#include "DungeonMakerNode.generated.h"

class UDungeonMakerGraph;

UCLASS(Blueprintable)
class DUNGEONMAKER_API UDungeonMakerNode : public UObject
{
	GENERATED_BODY()

public:
	UDungeonMakerNode();
	virtual ~UDungeonMakerNode();

	//////////////////////////////////////////////////////////////////////////
	// uproperties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonMakerNode")
	TSubclassOf<UObject> NodeType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonMakerNode")
	FString CustomNodeTitle;


	UPROPERTY(BlueprintReadOnly, Category = "DungeonMakerNode")
	TArray<UDungeonMakerNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonMakerNode")
	TArray<UDungeonMakerNode*> ChildrenNodes;

#if WITH_EDITOR
	UPROPERTY(EditDefaultsOnly, Category = "DungeonMakerNode")
	FLinearColor BackgroundColor;
#endif

	//////////////////////////////////////////////////////////////////////////
	// ufunctions
	UFUNCTION(BlueprintCallable, Category = "DungeonMakerNode")
	FString GetNodeTitle();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void EnterNode(AActor* OwnerActor);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ExitNode(AActor* OwnerActor);

	//////////////////////////////////////////////////////////////////////////
	UDungeonMakerGraph* GetGraph();
};
