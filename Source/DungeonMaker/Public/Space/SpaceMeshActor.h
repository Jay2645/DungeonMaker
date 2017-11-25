

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Tiles/DungeonTile.h"
#include "SpaceMeshActor.generated.h"

UCLASS()
class DUNGEONMAKER_API ASpaceMeshActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpaceMeshActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room")
	USceneComponent* DummyRoot;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
	const UDungeonTile* MeshTile;
public:
	void SetStaticMesh(const UDungeonTile* Tile, TArray<FDungeonTileMesh> Mesh);
	int32 AddInstance(int32 MeshIndex, const FTransform& Transform);
};
