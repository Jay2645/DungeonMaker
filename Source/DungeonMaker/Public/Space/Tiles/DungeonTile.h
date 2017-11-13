// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "DungeonTile.generated.h"

class UDungeonMissionSymbol;
class ADungeonRoom;

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
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* Mesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTransform Transform;
};

/**
*
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonTile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETileType TileType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName TileID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonTileMesh GroundMesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FDungeonTileMesh CeilingMesh;

	static const float TILE_SIZE;
};