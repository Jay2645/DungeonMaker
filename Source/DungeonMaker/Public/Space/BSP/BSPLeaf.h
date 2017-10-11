// BSP Leaf, used for random dungeon generation
// Based on https://gamedevelopment.tutsplus.com/tutorials/how-to-use-bsp-trees-to-generate-game-maps--gamedev-12268

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DungeonMissionNode.h"
#include "DungeonTile.h"
#include "BSPLeaf.generated.h"

class UBSPLeaf;

USTRUCT()
struct DUNGEONMAKER_API FBSPLink
{
	GENERATED_BODY()
public:
	// An open leaf, with no room assigned
	UPROPERTY()
	UBSPLeaf* AvailableLeaf;
	// The room this is coming from
	UPROPERTY()
	UBSPLeaf* FromLeaf;

	FBSPLink()
	{
		AvailableLeaf = NULL;
		FromLeaf = NULL;
	}

	bool operator==(const FBSPLink& Other) const
	{
		return AvailableLeaf == Other.AvailableLeaf;
	}

	friend uint32 GetTypeHash(const FBSPLink& Other)
	{
		return GetTypeHash(Other.AvailableLeaf);
	}
};

UCLASS()
class DUNGEONMAKER_API UBSPLeaf : public UObject
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBSPLeaf();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 YPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDungeonFloor LeafSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDungeonRoom Room;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntVector RoomOffset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UDungeonMissionNode* RoomSymbol;

	UPROPERTY()
	UBSPLeaf* LeftChild;
	UPROPERTY()
	UBSPLeaf* RightChild;
	UPROPERTY()
	UBSPLeaf* Parent;


	// The nearest leaves to us
	UPROPERTY()
	TSet<UBSPLeaf*> LeafNeighbors;
	// Any leaf which has a mission assigned and is connected to us
	UPROPERTY()
	TSet<UBSPLeaf*> MissionNeighbors;

protected:
	const uint8 MIN_LEAF_SIZE = 10;

public:
	UFUNCTION()
	void InitializeLeaf(int32 X, int32 Y, int32 Width, int32 Height, UBSPLeaf* ParentLeaf);

	UFUNCTION()
		bool Split(FRandomStream& Rng);
	UFUNCTION()
		void DetermineNeighbors();
	UFUNCTION()
		void SetMissionNode(UDungeonMissionNode* Node, const UDungeonTile* DefaultRoomTile, FRandomStream& Rng);

	UFUNCTION()
		bool HasChildren() const;
	UFUNCTION()
		bool SideIsLargerThan(int32 Size);
	UFUNCTION()
		bool ContainsPosition(int32 XPos, int32 YPos) const;
	UFUNCTION()
		void SetTile(int32 XPos, int32 YPos, const UDungeonTile* TileToSet);
	UFUNCTION()
		UBSPLeaf* GetLeaf(int32 XPos, int32 YPos);
	//UFUNCTION()
		const UDungeonTile* GetTile(int32 XPos, int32 YPos);
	UFUNCTION()
		int32 GetLeafCount() const;
	UFUNCTION()
		int32 GetChildLeafCount() const;
	UFUNCTION()
		FString ToString() const;
	UFUNCTION()
		void DrawDebugLeaf(AActor* ReferenceActor, float ZPos = 0.0f, bool bDebugLeaf = false);
	UFUNCTION()
		void AddMissionLeaf(UBSPLeaf* Neighbor);
	UFUNCTION()
	bool AreChildrenAllowed() const;
	UFUNCTION()
	bool HasConnectionTo(UBSPLeaf* Root);

	//UFUNCTION()
	void PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> ComponentLookup);
protected:
	bool HasConnectionTo(UBSPLeaf* Root, TSet<UBSPLeaf*>& Attempted);
	UFUNCTION()
	void CreateRoom(const UDungeonTile* DefaultRoomTile, FRandomStream &Rng);
};