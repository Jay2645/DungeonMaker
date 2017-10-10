#pragma once
#include "CoreMinimal.h"
#include "OutputGrammar.h"
#include "GraphOutputGrammar.generated.h"

class UGraphOutputGrammar;

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
		const UGraphOutputGrammar* Grammar;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Weight;

	UPROPERTY(BlueprintReadOnly)
		TArray<FGraphLink> MatchedLinks;

	FGraphOutput()
	{
		Grammar = NULL;
		Weight = 0.0f;
		MatchedLinks = TArray<FGraphLink>();
	}
};

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FNodeChildren
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<FGraphLink> Children;

	FNodeChildren()
	{
		Children = TSet<FGraphLink>();
	}
};

/**
* This is a "replacement" that gets added to the overall grammar "space".
* Future grammar iterations go through this output and try to find grammars in this output,
* and so on until no more matches can be found.
* The Grammar class is in charge of matching "alphabet" symbols to the OutputGrammars that replace them.
*/
UCLASS(BlueprintType)
class DUNGEONMAKER_API UGraphOutputGrammar : public UOutputGrammar
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGraphLink Head;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FNumberedGraphSymbol, FNodeChildren> Links;

	int32 Num() const;
	TArray<FNumberedGraphSymbol> GetSymbolArray() const;
	TSet<FGraphLink> GetSymbolChildren(const FNumberedGraphSymbol& Symbol) const;
	void Add(FNumberedGraphSymbol Parent, FGraphLink& Link);
	FString ToString() const;
};

