// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DungeonMissionNode.h"
#include "DungeonMissionGrammar.h"
#include "DungeonMissionGenerator.generated.h"

USTRUCT(BlueprintType)
struct DUNGEONMAKER_API FDungeonNodeCoupling
{
	GENERATED_BODY()
public:
	FDungeonNodeCoupling()
	{
		Parent = FNumberedGraphSymbol();
		Child = FNumberedGraphSymbol();
		bIsTightlyCoupled = false;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	FNumberedGraphSymbol Parent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	FNumberedGraphSymbol Child;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	bool bIsTightlyCoupled;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DUNGEONMAKER_API UDungeonMissionGenerator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDungeonMissionGenerator();
	
	UPROPERTY(BlueprintReadWrite, Category = "Dungeon Grammar")
	UDungeonMissionNode* Head;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	FNumberedGraphSymbol HeadSymbol;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	TArray<const UDungeonMissionGrammar*> Grammars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Grammar")
	int32 DungeonSize;

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions")
	void TryToCreateDungeon(FRandomStream& Stream);

	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	void DrawDebugDungeon();
	UFUNCTION(BlueprintCallable, Category = "World Generation|Dungeons|Missions|Debug")
	void PrintDebugDungeon();


protected:
	void TryToCreateDungeon(UDungeonMissionNode* StartingLocation, TArray<const UDungeonMissionGrammar*> AllowedGrammars, 
		FRandomStream& Rng, int32 RemainingMaxStepCount);

	void FindNodeMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars,
		UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars);

	void CheckGrammarMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars,
		const TArray<FGraphLink>& Links, UDungeonMissionNode* StartingLocation, bool bFoundMatches,
		TArray<FGraphOutput>& OutAcceptableGrammars);

	void FindMatchesWithChildren(TArray<const UDungeonMissionGrammar*>& AllowedGrammars,
		UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars);

	void ReplaceDungeonNodes(UDungeonMissionNode* StartingLocation,
		TArray<FGraphOutput> AcceptableGrammars, FRandomStream& Rng);

	void ReplaceNodes(UDungeonMissionNode* StartingLocation,
		const FGraphOutput& GrammarReplaceResult);

	TMap<FString, int32> GrammarUsageCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	TArray<UDungeonMissionNode*> UnresolvedHooks;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Grammar")
	//TArray<const UDungeonMissionGrammar*> UsedGrammars;
};
