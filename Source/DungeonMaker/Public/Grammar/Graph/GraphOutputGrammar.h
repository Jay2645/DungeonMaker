#pragma once
#include "CoreMinimal.h"
#include "OutputGrammar.h"
#include "GraphOutputGrammar.generated.h"

class UDungeonMakerGraph;

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FGraphLink
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FNumberedGraphSymbol Symbol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bIsTightlyCoupled;

	FGraphLink()
	{
		Symbol = FNumberedGraphSymbol();
		bIsTightlyCoupled = false;
	}

	bool operator==(const FGraphLink& Other) const
	{
		return Symbol == Other.Symbol && bIsTightlyCoupled == Other.bIsTightlyCoupled;
	}
	friend uint32 GetTypeHash(const FGraphLink& Link)
	{
		return GetTypeHash(Link.Symbol);
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FGraphOutput
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDungeonMakerGraph* Graph;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGraphLink> MatchedLinks;

	FGraphOutput()
	{
		Weight = 0.0f;
		MatchedLinks = TArray<FGraphLink>();
	}
};