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


USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<const UDungeonTile*> DungeonTiles;

	FDungeonRow()
	{
		DungeonTiles = TArray<const UDungeonTile*>();
	}
	FDungeonRow(int Size)
	{
		DungeonTiles.SetNum(Size);
		for (int i = 0; i < Size; i++)
		{
			DungeonTiles[i] = NULL;
		}
	}

	TSet<const UDungeonTile*> FindAllTiles()
	{
		TSet<const UDungeonTile*> tiles;
		tiles.Append(DungeonTiles);
		tiles.Remove(NULL);
		return tiles;
	}

	void Set(int Index, const UDungeonTile* Tile)
	{
		DungeonTiles[Index] = Tile;
	}

	const UDungeonTile* operator[] (int Index)
	{
		return DungeonTiles[Index];
	}

	int Num() const
	{
		return DungeonTiles.Num();
	}

	FString ToString() const
	{
		FString output;
		for (int i = 0; i < DungeonTiles.Num(); i++)
		{
			if (DungeonTiles[i] == NULL)
			{
				output += 'X';
			}
			else
			{
				output += DungeonTiles[i]->TileID.ToString();
			}
		}
		return output;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonRoomMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDungeonRow> DungeonRows;

	FDungeonRoomMetadata()
	{
		DungeonRows = TArray<FDungeonRow>();
	}

	FDungeonRoomMetadata(int SizeX, int SizeY)
	{
		check(SizeX >= 0);
		check(SizeY >= 0);
		DungeonRows.SetNum(SizeY);
		for (int i = 0; i < SizeY; i++)
		{
			DungeonRows[i] = FDungeonRow(SizeX);
		}
	}

	FDungeonRow& operator[] (int Index)
	{
		return DungeonRows[Index];
	}

	TSet<const UDungeonTile*> FindAllTiles()
	{
		TSet<const UDungeonTile*> tiles;
		for (int i = 0; i < DungeonRows.Num(); i++)
		{
			tiles.Append(DungeonRows[i].FindAllTiles());
		}
		return tiles;
	}

	void Set(int X, int Y, const UDungeonTile* Tile)
	{
		DungeonRows[Y].Set(X, Tile);
	}

	int XSize() const
	{
		if (DungeonRows.Num() == 0)
		{
			return 0;
		}
		else
		{
			return DungeonRows[0].Num();
		}
	}

	int YSize() const
	{
		return DungeonRows.Num();
	}

	FString ToString() const
	{
		FString output = "";
		for (int i = 0; i < YSize(); i++)
		{
			output += DungeonRows[i].ToString();
			output += "\n";
		}
		return output;
	}
	bool IsNotNull()
	{
		if (YSize() == 0 || XSize() == 0)
		{
			return false;
		}
		for (int x = 0; x < XSize(); x++)
		{
			for (int y = 0; y < YSize(); y++)
			{
				if (DungeonRows[y][x] != NULL)
				{
					return true;
				}
			}
		}
		return false;
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonFloorTile
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	const UDungeonTile* Tile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ADungeonRoom* Room;
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonFloor
{
	GENERATED_BODY()
public:
	FDungeonFloor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FIntVector, FDungeonFloorTile> TileLocations;

	void PlaceNewTile(FIntVector CurrentLocation, ADungeonRoom* Room, const UDungeonTile* Tile);
	void UpdateTile(FIntVector CurrentLocation, const UDungeonTile* NewTile);

	bool TileIsWall(FIntVector Location) const;
	const UDungeonTile* GetTileAt(FIntVector CurrentLocation);
	ADungeonRoom* GetRoom(FIntVector CurrentLocation);
	int32 YSize() const;
	int32 XSize() const;
private:
	FIntVector MaxExtents;
};