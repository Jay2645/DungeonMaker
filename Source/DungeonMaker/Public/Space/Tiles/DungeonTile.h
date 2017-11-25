#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "DungeonTile.generated.h"

class UDungeonMissionSymbol;
class ADungeonRoom;

UENUM(BlueprintType)
enum class ETileDirection : uint8
{
	Center,
	North,
	South,
	East,
	West,
	Northeast,
	Northwest,
	Southeast,
	Southwest
};

UENUM(BlueprintType)
enum class ETileType : uint8
{
	Floor,
	Wall
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonTileMesh
{
	GENERATED_BODY()
public:
	// What static mesh should be used for this tile.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* Mesh;
	// The offset of our static mesh.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTransform Transform;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance = 1.0f;
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonTileInteraction
{
	GENERATED_BODY()
public:
	// An actor that should be spawned on this tile.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> InteractionActor;
	
	// This actor's base offset from the tile.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTransform BaseTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelectionChance = 1.0f;
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonTileInteractionOptions
{
	GENERATED_BODY()
public:
	// All possible options that we can select from
	// to interact with.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<FDungeonTileInteraction> Options;
	
	// The offset of our actor. Different directions can have
	// different offsets. Center means anywhere which is not an edge.
	// If no direction is specified, then the transform will default
	// to the base transform.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<ETileDirection, FTransform> DirectionOffsets;
};

/**
* A Dungeon Tile, as implied by its name, is the basic tile
* which makes up the entirety of the dungeon. These tiles
* represent playable and non-playable areas, and can be used
* to spawn traps, items, or anything you can think of.
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonTile : public UDataAsset
{
	GENERATED_BODY()

public:
	// Is this tile something which obstructs movement, or
	// is movement allowed on this tile?
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETileType TileType;
	// A unique ID for this tile. Non-unique IDs may crash
	// the editor!
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName TileID;
	// A list of all meshes which go on the ground
	// (floors, walls, bottomless pits, etc.)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDungeonTileMesh> GroundMesh;
	// Should this room always choose the same ground tile, or
	// should it try and pick a new ground tile every time?
	// Note that different rooms will have a chance to use a 
	// different tile regardless.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bGroundMeshShouldAlwaysBeTheSame = true;
	// A list of all meshes which go on the ceiling,
	// above the player
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDungeonTileMesh> CeilingMesh;

	// Should this room always choose the same ceiling tile, or
	// should it try and pick a new ceiling tile every time?
	// Note that different rooms will have a chance to use a 
	// different tile regardless.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCeilingMeshShouldAlwaysBeTheSame = true;

	// A list of all actors which will be spawned on this 
	// tile. This can be in addition to or instead of a tile 
	// mesh. Things with special behavior (like keys) can be spawned here.
	//
	// Exactly ONE (1) interaction will be spawned on this tile.
	//
	// You can also specify different variants for an
	// interaction, one of which will be spawned at random.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDungeonTileInteractionOptions> Interactions;

	static const float TILE_SIZE;
};