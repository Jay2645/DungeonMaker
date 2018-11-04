

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GroundScatterItem.h"
#include "GroundScatterManager.generated.h"

class ADungeonRoom;

/*
* This class is dedicated to spawning in ground scatter in a room.
* Tiles are defined, with sets of ground scatter that should be spawned on that particular tile.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UGroundScatterManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGroundScatterManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FGroundScatterPairing GroundScatter;

	// All ground scatter we have spawned
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Props")
	TArray<AActor*> SpawnedGroundScatter;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Props")
	TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> StaticMeshes;

public:
	void DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations,
		FRandomStream& Rng, ADungeonRoom* Room);

	AActor* SpawnScatterActor(ADungeonRoom* Room, const FIntVector& Location,
		const UGroundScatterItem* Scatter, FRandomStream& Rng);
private:
	void ProcessScatterItem(const UGroundScatterItem* Scatter, const TArray<FIntVector>& TileLocations, 
		FRandomStream& Rng, const UDungeonTile* Tile, ADungeonRoom* Room);

	AActor* CreateScatterObject(ADungeonRoom* Room, const FIntVector& Location,
		const UGroundScatterItem* Scatter, FRandomStream& Rng, FScatterTransform& SelectedObject,
		ETileDirection Direction, TSubclassOf<AActor> SelectedActor);

	int32 CreateScatterObject(ADungeonRoom* Room, const FIntVector& Location,
		const UGroundScatterItem* Scatter, FRandomStream& Rng, FScatterTransform& SelectedObject,
		ETileDirection Direction, UStaticMesh* SelectedMesh);

	bool IsAdjacencyOkay(ETileDirection Direction, const UGroundScatterItem* Scatter,
		ADungeonRoom* Room, FIntVector& Location);

	FScatterObject FindScatterObject(const UGroundScatterItem* Scatter, FRandomStream& Rng,
		FScatterTransform& SelectedObject, ADungeonRoom* Room, 
		const FIntVector& Position, ETileDirection Direction);

	FTransform GetObjectTransform(ADungeonRoom* Room, const FIntVector& Location,
		const UGroundScatterItem* Scatter, FRandomStream& Rng, const FScatterTransform& SelectedObject,
		ETileDirection Direction);
};
