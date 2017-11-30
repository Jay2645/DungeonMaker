#pragma once

#include "CoreMinimal.h"
#include "DungeonMakerEdGraph.generated.h"

UCLASS()
class UDungeonMakerEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	UDungeonMakerEdGraph();
	virtual ~UDungeonMakerEdGraph();

	virtual void RebuildDungeonMaker();

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
};
