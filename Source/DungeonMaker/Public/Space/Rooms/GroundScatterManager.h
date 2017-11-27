

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GroundScatterManager.generated.h"

class ADungeonRoom;

USTRUCT(BlueprintType)
struct FScatterObject
{
	GENERATED_BODY()
public:
	// An actor that will be scattered in the level.
	// You can choose an actor OR a mesh -- if both are chosen,
	// there is a 50/50 shot of choosing either one.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> ScatterObject;
	// A mesh that will be scattered in the level.
	// You can choose an actor OR a mesh -- if both are chosen,
	// there is a 50/50 shot of choosing either one.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* ScatterMesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;
	// An additive difficulty modifier which gets added to the selection chance
	// based on the difficulty of the room.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float DifficultyModifier;

	FScatterObject()
	{
		ScatterObject = NULL;
		SelectionChance = 1.0f;
		DifficultyModifier = 0.0f;
	}

};

USTRUCT(BlueprintType)
struct FScatterTransform
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FScatterObject> ScatterMeshes;

	// Which edges of the room this ground scatter is valid at.
	// Center means anywhere which is not an edge.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TMap<ETileDirection, FTransform> DirectionOffsets;

	// If this is false, we will be placed on the ground.
	// If this is true, we will be placed on the ceiling.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bOffsetFromTop;

	// How far away this should be from the edge of any room.
	// Bear in mind that this doesn't make sense with any allowed directions other than Center.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FIntVector EdgeOffset;

	FScatterTransform()
	{
		EdgeOffset = FIntVector::ZeroValue;
	}
};

USTRUCT(BlueprintType)
struct FGroundScatter
{
	GENERATED_BODY()
public:
	// A list of all objects we should scatter on this tile.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FScatterTransform> ScatterObjects;
	// Whether we should use a different object each time we want to place
	// some ground scatter from the ScatterObject list, or if we should keep
	// using the same object over and over. Useful for placing the same type
	// of trim around the room.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlwaysUseSameObjectForThisInstance;

	// Whether we should be able to place this adjacent to next rooms.
	// Next rooms are determined by our current mission.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlaceAdjacentToNextRooms;
	// Whether we should be able to place this adjacent to previous rooms.
	// Next rooms are determined by our current mission.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlaceAdjacentToPriorRooms;

	// Should we keep track of how many objects we place at all, or should we place as many as we want?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseRandomCount;
	// Should we place this object randomly in the room?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseRandomLocation;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bConformToGrid;
	// What's the minimum count of objects we should place?
	// Only used if we're using a random count.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MinCount;
	// What's the maximum count of objects we should place?
	// Only used if we're using a random count.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 MaxCount;
	// Skip every n tiles when placing this.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 SkipTiles;

	FGroundScatter()
	{
		bUseRandomCount = false;
		bUseRandomLocation = true;
		bConformToGrid = true;
		bPlaceAdjacentToNextRooms = true;
		bPlaceAdjacentToPriorRooms = true;
		MinCount = 0;
		MaxCount = 255;
		SkipTiles = 0;
	}
};

USTRUCT(BlueprintType)
struct FGroundScatterSet
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FGroundScatter> GroundScatter;
};

USTRUCT(BlueprintType)
struct FGroundScatterPairing
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<const UDungeonTile*, FGroundScatterSet> Pairings;

	void CombinePairings(const FGroundScatterPairing& Other)
	{
		for (auto& kvp : Other.Pairings)
		{
			if (Pairings.Contains(kvp.Key))
			{
				Pairings[kvp.Key].GroundScatter.Append(kvp.Value.GroundScatter);
			}
			else
			{
				Pairings.Add(kvp.Key, kvp.Value);
			}
		}
	}
};

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
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Props")
	TArray<AActor*> SpawnedGroundScatter;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Props")
	TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> StaticMeshes;

public:
	void DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations,
		FRandomStream& Rng, ADungeonRoom* Room);

	AActor* SpawnScatterActor(ADungeonRoom* Room, const FIntVector& LocalLocation,
		FGroundScatter& Scatter, FRandomStream& Rng);
private:
	void ProcessScatterItem(FGroundScatter& Scatter, const TArray<FIntVector>& TileLocations, 
		FRandomStream& Rng, const UDungeonTile* Tile, ADungeonRoom* Room);

	AActor* CreateScatterObject(ADungeonRoom* Room, const FIntVector& Location,
		FGroundScatter &Scatter, FRandomStream& Rng, FScatterTransform& SelectedObject,
		ETileDirection Direction, TSubclassOf<AActor> SelectedActor);

	int32 CreateScatterObject(ADungeonRoom* Room, const FIntVector& Location,
		FGroundScatter &Scatter, FRandomStream& Rng, FScatterTransform& SelectedObject,
		ETileDirection Direction, UStaticMesh* SelectedMesh);

	bool IsAdjacencyOkay(ETileDirection Direction, FGroundScatter& Scatter,
		ADungeonRoom* Room, FIntVector& Location);

	FScatterObject FindScatterObject(FGroundScatter& Scatter, FRandomStream& Rng,
		FScatterTransform& SelectedObject, ADungeonRoom* Room, 
		const FIntVector& LocalPosition, ETileDirection Direction);
};
