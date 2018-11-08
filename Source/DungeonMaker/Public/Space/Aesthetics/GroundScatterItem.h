

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "GameplayTagContainer.h"

#include "DungeonTile.h"

#include "GroundScatterItem.generated.h"

/**
* This is an object which gets placed in the level. It can be a
* simple mesh (which is very efficient, using instanced static mesh components),
* or it can be an actual actor which gets spawned in the level (for objects that
* need physics, animations, or that can be interacted with by the player).
*/
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

	// How likely it is that this object will be selected.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance;

	// Metadata about this scatter object.
	// Objects with tags that more closely match tags associated with the dungeon get more weight
	// when being considered for selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer ObjectTags;

	// An additive difficulty modifier which gets added to the selection chance
	// based on the difficulty of the room.
	// You can make an object more or less likely to spawn near the end of the dungeon.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float DifficultyModifier;

	FScatterObject()
	{
		ScatterObject = NULL;
		SelectionChance = 1.0f;
		DifficultyModifier = 0.0f;
	}
};

/**
* A scatter transform is a list of meshes and the common transform they share.
* If you have a lot of slightly different meshes that could all be placed using
* the same transform, you can list all those meshes here, rather than having to
* copy and paste the same exact transform for each one of them.
*/
USTRUCT(BlueprintType)
struct FScatterTransform
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Meshes")
	TArray<FScatterObject> ScatterMeshes;

	// Which edges of the room this ground scatter is valid at.
	// Center means anywhere which is not an edge.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transform")
	TMap<ETileDirection, FTransform> DirectionOffsets;

	// If we're using a random offset, what's the minimum offset that can be applied?
	// Ignored if we're not using a random offset.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transform")
	FTransform MinRandomOffset;

	// If we're using a random offset, what's the maximum offset that can be applied?
	// Ignored if we're not using a random offset.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transform")
	FTransform MaxRandomOffset;

	// If this is false, we will be placed on the ground.
	// If this is true, we will be placed on the ceiling.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Offset")
	bool bOffsetFromTop;

	// How far away this should be from the edge of any room.
	// Bear in mind that this doesn't make sense with any allowed directions other than Center.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Offset")
	FIntVector EdgeOffset;

	// Should a random offset be applied to our transform?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Offset")
	bool bShouldUseRandomTransformOffset;

	FScatterTransform()
	{
		EdgeOffset = FIntVector::ZeroValue;
		bShouldUseRandomTransformOffset = false;
		bOffsetFromTop = false;
	}
};

/**
 * A ground scatter item is a list of different objects which can be placed on a tile.
 * One of these objects may be selected for each tile (based on rules defined in the item)
 * and can be placed in the level.
 * This can be used to place props and scenery in a procedurally-generated level.
 */
UCLASS(BlueprintType)
class DUNGEONMAKER_API UGroundScatterItem : public UDataAsset
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
	// Should this object be placed in a grid (like the tiles are), or can it be placed anywhere?
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

public:
	UGroundScatterItem();
};

USTRUCT(BlueprintType)
struct FGroundScatterSet
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<const UGroundScatterItem*> GroundScatter;
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