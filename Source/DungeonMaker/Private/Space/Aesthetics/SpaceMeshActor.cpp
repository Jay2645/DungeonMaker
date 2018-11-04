

#include "SpaceMeshActor.h"
#include "Engine/CollisionProfile.h"


// Sets default values
ASpaceMeshActor::ASpaceMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bCanBeDamaged = false;

	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	/*bCanBeDamaged = false;
	UHierarchicalInstancedStaticMeshComponent* meshComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Meshes"));

	meshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	meshComponent->Mobility = EComponentMobility::Movable;
	meshComponent->bGenerateOverlapEvents = false;
	meshComponent->bUseDefaultCollision = true;

	RootComponent = meshComponent;
	MeshComponents.Add(meshComponent);*/
}

void ASpaceMeshActor::SetStaticMesh(const UDungeonTile* Tile, TArray<FDungeonTileMesh> Meshes)
{
	MeshTile = Tile;
	UE_LOG(LogSpaceGen, Log, TEXT("Creating %d meshes for tile %s."), Meshes.Num(), *Tile->TileID.ToString());
	for (int i = 0; i < Meshes.Num(); i++)
	{
		if (Meshes[i].Mesh == NULL)
		{
			continue;
		}
		FString meshName = Tile->TileID.ToString() + " Mesh ";
		meshName.AppendInt(i);
		UHierarchicalInstancedStaticMeshComponent* meshComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, FName(*meshName));

		meshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		meshComponent->Mobility = EComponentMobility::Movable;
		meshComponent->SetGenerateOverlapEvents(false);
		meshComponent->bUseDefaultCollision = true;

		meshComponent->SetStaticMesh(Meshes[i].Mesh);
		meshComponent->RegisterComponent();
		MeshComponents.Add(meshComponent);
	}
	/*if (Meshes.Num() == 0)
	{
		return;
	}
	MeshComponents[0]->SetStaticMesh(Meshes[0].Mesh);*/
}

int32 ASpaceMeshActor::AddInstance(int32 MeshID, const FTransform& Transform)
{
	verify(MeshComponents.IsValidIndex(MeshID));
	return MeshComponents[MeshID]->AddInstance(Transform);
}
