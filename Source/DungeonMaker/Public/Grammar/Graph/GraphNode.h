#pragma once
#include "CoreMinimal.h"
#include "GrammarAlphabet.h"
#include "GraphNode.generated.h"

UCLASS(BlueprintType)
class DUNGEONMAKER_API UGraphNode : public UGrammarAlphabet
{
	GENERATED_BODY()
};



// This associates graph nodes with a symbol ID.
// The idea is that graph nodes can be identified by their ID,
// even if the node's symbol changes to a different node.
// The ID must be set up ahead of time, either in code, blueprint, or in the editor.
// IDs are not automatically assigned.
USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FNumberedGraphSymbol
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UGraphNode* Symbol;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 SymbolID;

	bool operator==(const FNumberedGraphSymbol &Other) const
	{
		return SymbolID == Other.SymbolID;
	}

	FString GetSymbolDescription() const
	{
		checkf(Symbol != NULL && Symbol->IsValidLowLevel(), TEXT("Symbol was not assigned properly!"));
		checkf(Symbol->Description.IsValid(), TEXT("Symbol did not have valid name!"));
		return Symbol->Description.ToString();
	}

	friend uint32 GetTypeHash(const FNumberedGraphSymbol& symbol)
	{
		return GetTypeHash(symbol.SymbolID);
	}
};