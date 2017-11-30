#pragma once

#include "CoreMinimal.h"
#include "DungeonMakerNode.h"
#include "DungeonMakerGraph.generated.h"

#define LOG_WARNING(FMT, ...) UE_LOG(LogMissionGen, Warning, (FMT), ##__VA_ARGS__)

UCLASS(Blueprintable)
class DUNGEONMAKER_API UDungeonMakerGraph : public UObject
{
	GENERATED_BODY()

public:
	UDungeonMakerGraph();
	virtual ~UDungeonMakerGraph();

	//////////////////////////////////////////////////////////////////////////
	// uproperties
	UPROPERTY(EditAnywhere, Category = "DungeonMaker")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "DungeonMaker")
	TSubclassOf<UDungeonMakerNode> NodeType;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonMaker")
	TArray<UDungeonMakerNode*> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonMaker")
	TArray<UDungeonMakerNode*> AllNodes;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UEdGraph* EdGraph;
#endif

	//////////////////////////////////////////////////////////////////////////
	// ufunctions
	UFUNCTION(BlueprintCallable, Category = "DungeonMaker")
	void Print(bool ToConsole = true, bool ToScreen = true);

	UFUNCTION(BlueprintCallable, Category = "DungeonMaker")
	int GetLevelNum();

	UFUNCTION(BlueprintCallable, Category = "DungeonMaker")
	void GetNodesByLevel(int Level, TArray<UDungeonMakerNode*>& Nodes);

	void ClearGraph();
};
