// BSP Leaf, used for random dungeon generation
// Based on https://gamedevelopment.tutsplus.com/tutorials/how-to-use-bsp-trees-to-generate-game-maps--gamedev-12268

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DungeonMissionNode.h"
#include "DungeonTile.h"
#include "BSPLeaf.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UBSPLeaf : public USceneComponent
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
	int32 ID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDungeonRoom Room;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntVector RoomOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FDungeonRoom> Hallways;

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
	TSet<UBSPLeaf*> MissionNeighbors;

protected:
	const uint8 MIN_LEAF_SIZE = 10;
	static int32 NextId;

public:
	UFUNCTION()
	static UBSPLeaf* CreateLeaf(UObject* Outer, UBSPLeaf* ParentLeaf, FName Name, int32 X, int32 Y, int32 Width, int32 Height);

	UFUNCTION()
		bool Split(FRandomStream& Rng);
	UFUNCTION()
		void DetermineNeighbors();
	UFUNCTION()
		void SetMissionNode(UDungeonMissionNode* Node, FRandomStream& Rng);
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
		void DrawDebugLeaf(float ZPos = 0.0f, bool bDebugLeaf = false) const;
	UFUNCTION()
		void AddMissionLeaf(UBSPLeaf* Neighbor);
	UFUNCTION()
		bool AreChildrenAllowed() const;
};