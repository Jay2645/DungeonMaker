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
	UStaticMesh* TileMesh;

	static const float TILE_SIZE;
};