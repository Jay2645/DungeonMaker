// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonTile.generated.h"

class UDungeonMissionSymbol;

/**
*
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UDungeonTile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName TileID;
};


USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<const UDungeonTile*> DungeonTiles;

	FDungeonRow() {}
	FDungeonRow(int Size)
	{
		DungeonTiles.SetNum(Size);
		for (int i = 0; i < Size; i++)
		{
			DungeonTiles[i] = NULL;
		}
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
struct DUNGEONMAKER_API FDungeonRoom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDungeonRow> DungeonRows;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	const UDungeonMissionSymbol* Symbol;

	FDungeonRoom()
	{
		
	}
	FDungeonRoom(int SizeX, int SizeY)
	{
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

	static FDungeonRoom* GenerateDungeonRoom(const UDungeonMissionSymbol* Symbol, const UDungeonTile* DefaultRoomTile);
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonFloor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDungeonRow> DungeonRows;

	FDungeonFloor() {}
	FDungeonFloor(int SizeX, int SizeY)
	{
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
};