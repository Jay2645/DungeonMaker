

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Map.h"

#include "DungeonRoom.h"
#include "DungeonTile.h"
#include "SpaceMeshActor.h"

#include "RoomMeshComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API URoomMeshComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	TArray<TPair<const UDungeonTile*, FTransform>> TileBacklog;

protected:
	UPROPERTY()
	ADungeonRoom* ParentRoom;
	UPROPERTY()
	UGroundScatterManager* GroundScatter;
	UPROPERTY()
	UDungeonSpaceGenerator* DungeonSpace;
	UPROPERTY()
	bool bHasPlacedMeshes;

public:
	// These all determine which tiles we have selected, to ensure consistency
	// throughout the room
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, int32> FloorTileMeshSelections;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, int32> CeilingTileMeshSelections;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tiles")
	TMap<const UDungeonTile*, FDungeonTileInteractionOptions> InteractionOptions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDoGroundScatter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bPlaceTileMeshes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bSpawnInteractions;

public:	
	// Sets default values for this component's properties
	URoomMeshComponent();

private:
	void ClearTileBacklog();

protected:
	void PlaceTile(TMap<const UDungeonTile*, ASpaceMeshActor*>& ComponentLookup,
		const UDungeonTile* Tile, int32 MeshID, const FTransform& MeshTransformOffset, const FIntVector& Location);
	void CreateAllRoomTiles(TMap<const UDungeonTile*, TArray<FIntVector>>& TileLocations,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
		FRandomStream& Rng);

	void SpawnInteractions(TMap<const UDungeonTile *, TArray<FIntVector>> &TileLocations, FRandomStream& Rng);

public:
	void InitializeMeshComponent(ADungeonRoom* Room, UGroundScatterManager* GroundScatterManager, UDungeonSpaceGenerator* Space);

	void PlaceRoomTiles(TMap<const UDungeonTile*, ASpaceMeshActor*>& FloorComponentLookup,
		TMap<const UDungeonTile*, ASpaceMeshActor*>& CeilingComponentLookup,
		FRandomStream& Rng);

	void DetermineGroundScatter(TMap<const UDungeonTile*, TArray<FIntVector>> TileLocations,
		FRandomStream& Rng);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	FTransform CreateMeshTransform(const FTransform &MeshTransformOffset, const FIntVector &Location) const;
	
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	AActor* SpawnInteraction(const UDungeonTile* Tile, FDungeonTileInteractionOptions TileInteractionOptions, const FIntVector& Location, FRandomStream& Rng);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeon Generation|Rooms|Tiles")
	void CreateNewTileMesh(const UDungeonTile* Tile, const FTransform& Location);

	UFUNCTION(BlueprintPure, Category = "World Generation|Dungeon Generation|Rooms")
	FDungeonSpace& GetDungeon() const;
};
