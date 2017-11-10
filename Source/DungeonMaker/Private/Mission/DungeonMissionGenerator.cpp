// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionGenerator.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "Grammar/Grammar.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UDungeonMissionGenerator::UDungeonMissionGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	/*StartNode = FDungeonNode();
	StartNode.Symbol = FName(TEXT("Start"));*/
}

void UDungeonMissionGenerator::TryToCreateDungeon(FRandomStream& Stream)
{
	Head = NewObject<UDungeonMissionNode>();
	Head->Symbol.Symbol = HeadSymbol.Symbol;
	Head->bTightlyCoupledToParent = false;
	DungeonSize = 1;

	GrammarUsageCount.Empty();
	TryToCreateDungeon(Head, Grammars, Stream, 255);

	// Relabel all the node IDs with their (hopefully final) IDs
	int32 currentID = 1;
	TArray<UDungeonMissionNode*> nodes;
	nodes.Add(Head);
	TSet<UDungeonMissionNode*> processed;
	while (nodes.Num() > 0)
	{
		UDungeonMissionNode* current = nodes[0];
		nodes.RemoveAt(0);
		if (processed.Contains(current))
		{
			continue;
		}

		current->Symbol.SymbolID = currentID;
		currentID++;
		for (FMissionNodeData& node : current->NextNodes)
		{
			nodes.Add(node.Node);
		}
		processed.Add(current);

		DungeonSize++;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogMissionGen, Log, TEXT("Completed dungeon:"));
	PrintDebugDungeon();
#endif
}

void UDungeonMissionGenerator::FindNodeMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars, UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars)
{
	bool bFoundMatches = OutAcceptableGrammars.Num() > 0;
	FGraphLink us;
	us.Symbol = StartingLocation->Symbol;
	us.bIsTightlyCoupled = StartingLocation->bTightlyCoupledToParent;

	// We're only checking if us by ourselves is valid, so the array just needs to contain us.
	TArray<FGraphLink> links;
	links.Add(us);

	UE_LOG(LogMissionGen, Log, TEXT("Checking if %s is a valid input."), *us.Symbol.GetSymbolDescription());

	CheckGrammarMatches(AllowedGrammars, links, StartingLocation, bFoundMatches, OutAcceptableGrammars);

}

void UDungeonMissionGenerator::FindMatchesWithChildren(TArray<const UDungeonMissionGrammar *>& AllowedGrammars, UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars)
{
	// We have children; we should check to see if we have a grammar which accepts us and our children
	// Define us first
	FGraphLink us;
	us.Symbol = StartingLocation->Symbol;
	us.bIsTightlyCoupled = StartingLocation->bTightlyCoupledToParent;

	UE_LOG(LogMissionGen, Log, TEXT("Trying to match childen of %s!"), *us.Symbol.GetSymbolDescription());

	// Iterate over each child
	for (FMissionNodeData& nextNode : StartingLocation->NextNodes)
	{
		// Add ourselves to the array
		TArray<FGraphLink> links;
		links.Add(us);

		FGraphLink next;
		next.Symbol = nextNode.Node->Symbol;
		next.bIsTightlyCoupled = nextNode.Node->bTightlyCoupledToParent;

		links.Add(next);

		UE_LOG(LogMissionGen, Log, TEXT("Checking coupling %s->%s"), *us.Symbol.GetSymbolDescription(), *next.Symbol.GetSymbolDescription());

		CheckGrammarMatches(AllowedGrammars, links, StartingLocation, false, OutAcceptableGrammars);
	}

#if !UE_BUILD_SHIPPING
	if (OutAcceptableGrammars.Num() == 0)
	{
		UE_LOG(LogMissionGen, Warning, TEXT("No symbols matched any child combination of %s."), *StartingLocation->GetSymbolDescription());
	}
#endif
}

void UDungeonMissionGenerator::CheckGrammarMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars, const TArray<FGraphLink>& Links, UDungeonMissionNode* StartingLocation, bool bFoundMatches, TArray<FGraphOutput>& OutAcceptableGrammars)
{
	for (int i = 0; i < AllowedGrammars.Num(); i++)
	{
		// Iterate over all grammars
		const UDungeonMissionGrammar* grammar = AllowedGrammars[i];

		EGrammarResultType resultType = grammar->MatchesGrammar(this, Links);
		if (resultType == EGrammarResultType::Accepted)
		{
			// We can replace ourselves with a new symbol!
			// Grab our output
			UGraphOutputGrammar* output = (UGraphOutputGrammar*)grammar->RuleOutput;
#if !UE_BUILD_SHIPPING
			FString linkString = "";
			for (int i = 0; i < Links.Num(); i++)
			{
				linkString.Append(Links[i].Symbol.GetSymbolDescription());
				if (i + 1 < Links.Num())
				{
					if (Links[i + 1].bIsTightlyCoupled)
					{
						linkString.Append("=>");
					}
					else
					{
						linkString.Append("->");
					}
				}
			}
			UE_LOG(LogMissionGen, Log, TEXT("Replacing %s with %s."), *linkString, *output->ToString());
#endif
			// Make us less likely to be chosen if we've been chosen a lot before
			float weightModifier = 1.0f;
			if (GrammarUsageCount.Contains(output->ToString()))
			{
				weightModifier /= GrammarUsageCount[output->ToString()];
			}
			if (bFoundMatches)
			{
				// We already have good matches which match more nodes, so it should be less likely to match these ones
				weightModifier *= 0.25f;
			}
			FGraphOutput replaceResult;
			replaceResult.Grammar = output;
			replaceResult.Weight = grammar->Weight * weightModifier;
			replaceResult.MatchedLinks = Links;
			// Add it to the list of things we can do to ourselves
			OutAcceptableGrammars.Add(replaceResult);
		}
	}
}

void UDungeonMissionGenerator::DrawDebugDungeon()
{
	check(Head != NULL && Head->Symbol.Symbol != NULL);
	int32 dungeonDepth = Head->GetLevelCount();
	TMap<UDungeonMissionNode*, FIntVector> dungeonCoords;
	int xOffset = 0;
	int yOffset = 0;

	TArray<TArray<UDungeonMissionNode*>> allNodes;
	allNodes.AddDefaulted(dungeonDepth);
	TArray<UDungeonMissionNode*> startNodes;
	startNodes.Add(Head);
	allNodes[0] = startNodes;

	TQueue<UDungeonMissionNode*> nodesToDraw;
	
	for (int j = 0; j < allNodes.Num(); j++)
	{
		for (int k = 0; k < allNodes[j].Num(); k++)
		{
			UDungeonMissionNode* next = allNodes[j][k];
			nodesToDraw.Enqueue(next);
			dungeonCoords.Add(next, FIntVector(xOffset + k, yOffset, 0));
			for (FMissionNodeData& node : next->NextNodes)
			{
				int32 nodesBelow = node.Node->GetLevelCount();
				if (nodesBelow <= 1)
				{
					// Send to the bottom
					nodesToDraw.Enqueue(node.Node);
					dungeonCoords.Add(node.Node, FIntVector(xOffset + k, dungeonDepth - 1, 0));
					// Increment the x offset; the bottom of this one is already accounted for
					xOffset++;
					continue;
				}

				// Otherwise, our child has child nodes
				// This will never overflow because we already know how many children have child nodes
				allNodes[j + 1].Add(node.Node);
			}
		}
		yOffset++;
	}

	while (!nodesToDraw.IsEmpty())
	{
		UDungeonMissionNode* next;
		nodesToDraw.Dequeue(next);

		FIntVector nodeLocation = dungeonCoords[next];
		FVector drawLocation = FVector(nodeLocation.X * 100.0f, nodeLocation.Y * 100.0f, 0.0f);

		DrawDebugSphere(GetWorld(), drawLocation, 15.0f, 8, FColor(255, 0, 255), true);
		DrawDebugString(GetWorld(), drawLocation + FVector(0.0f, 0.0f, 100.0f), next->Symbol.GetSymbolDescription());
		for (FMissionNodeData& node : next->NextNodes)
		{
			FIntVector childLocation = dungeonCoords[node.Node];
			FVector drawChildLocation = FVector(childLocation.X * 100.0f, childLocation.Y * 100.0f, 0.0f);
			FColor lineColor;
			if (node.bTightlyCoupledToParent)
			{
				lineColor = FColor(0, 0, 255);
			}
			else
			{
				lineColor = FColor(255, 0, 255);
			}
			DrawDebugLine(GetWorld(), drawLocation, drawChildLocation, lineColor, true);
		}
	}
}

void UDungeonMissionGenerator::PrintDebugDungeon()
{
	check(Head != NULL && Head->Symbol.Symbol != NULL);
	Head->PrintNode(0);
}

void UDungeonMissionGenerator::TryToCreateDungeon(UDungeonMissionNode* StartingLocation, 
	TArray<const UDungeonMissionGrammar*> AllowedGrammars, FRandomStream& Rng, int32 RemainingMaxStepCount)
{
	checkf(StartingLocation != NULL, TEXT("Starting node for dungeon generation was null!"));
	checkf(StartingLocation->IsValidLowLevel(), TEXT("Starting node for dungeon generation was invalid!"));
	checkf(StartingLocation->Symbol.Symbol != NULL, TEXT("Starting node for dungeon generation had no symbols!"));
	checkf(AllowedGrammars.Num() > 0, TEXT("There were no allowed grammars for dungeon generation!"));
	checkf(RemainingMaxStepCount >= 0, TEXT("Dungeon generation ran out of steps! You have an overflow issue."));

	UE_LOG(LogMissionGen, Log, TEXT("Trying to create a dungeon starting from %s."), *StartingLocation->GetSymbolDescription());

	if (StartingLocation->Symbol.Symbol->bIsTerminalNode)
	{
		// This node has already been processed completely and turned into a terminal node
		for (FMissionNodeData& node : StartingLocation->NextNodes)
		{
			// Try and process each child
			TryToCreateDungeon(node.Node, AllowedGrammars, Rng, RemainingMaxStepCount - 1);
		}
		return;
	}

	TArray<FGraphOutput> acceptableGrammars;
	if (StartingLocation->NextNodes.Num() > 0)
	{
		FindMatchesWithChildren(AllowedGrammars, StartingLocation, acceptableGrammars);
	}

	// Try to see if we have a grammar that accepts only us
	FindNodeMatches(AllowedGrammars, StartingLocation, acceptableGrammars);

	UE_LOG(LogMissionGen, Log, TEXT("Found %d acceptable grammars for %s."), acceptableGrammars.Num(), *StartingLocation->GetSymbolDescription());

	// Replace and look again
	if (acceptableGrammars.Num() > 0)
	{
		ReplaceDungeonNodes(StartingLocation, acceptableGrammars, Rng);
		TryToCreateDungeon(StartingLocation, AllowedGrammars, Rng, RemainingMaxStepCount - 1);
	}
	else
	{
		// No matching grammars; turn into a hook
		UE_LOG(LogMissionGen, Error, TEXT("%s had no matching grammars."), *StartingLocation->GetSymbolDescription());
		UnresolvedHooks.Add(StartingLocation);
		for (FMissionNodeData& node : StartingLocation->NextNodes)
		{
			// Try and process each child
			TryToCreateDungeon(node.Node, AllowedGrammars, Rng, RemainingMaxStepCount - 1);
		}
	}
}

void UDungeonMissionGenerator::ReplaceDungeonNodes(UDungeonMissionNode* StartingLocation, 
	TArray<FGraphOutput> AcceptableGrammars, FRandomStream& Rng)
{
	checkf(AcceptableGrammars.Num() > 0, TEXT("There weren't any accepted grammars!"));
	int32 index = Rng.RandRange(0, AcceptableGrammars.Num() - 1);
	FGraphOutput grammarReplaceResult = AcceptableGrammars[index];
	if (grammarReplaceResult.Weight <= 0.0f)
	{
		AcceptableGrammars.RemoveAt(index);
		ReplaceDungeonNodes(StartingLocation, AcceptableGrammars, Rng);
		return;
	}
	else if(AcceptableGrammars.Num() > 1)
	{
		// Higher weights are more likely to be kept
		float replaceChance = 1.0f / (grammarReplaceResult.Weight + 1);
		float rngAmount = Rng.GetFraction();
		if (rngAmount < replaceChance)
		{
			// Toss it back and get a new one
			ReplaceDungeonNodes(StartingLocation, AcceptableGrammars, Rng);
			return;
		}
	}

	if (GrammarUsageCount.Contains(grammarReplaceResult.Grammar->ToString()))
	{
		GrammarUsageCount[grammarReplaceResult.Grammar->ToString()] += 1;
	}
	else
	{
		GrammarUsageCount.Add(grammarReplaceResult.Grammar->ToString(), 1);
	}

	ReplaceNodes(StartingLocation, grammarReplaceResult);

}

void UDungeonMissionGenerator::ReplaceNodes(UDungeonMissionNode* StartingLocation, const FGraphOutput& GrammarReplaceResult)
{
	// Find the matched nodes
	UDungeonMissionNode* startLocation = StartingLocation;
	UDungeonMissionNode* replaceLocation = NULL;
	if (GrammarReplaceResult.MatchedLinks.Num() > 1)
	{
		replaceLocation = StartingLocation->FindChildNodeFromSymbol(GrammarReplaceResult.MatchedLinks[1].Symbol);
	}

	// Number the nodes
	startLocation->Symbol.SymbolID = 1;
	if (replaceLocation != NULL)
	{
		replaceLocation->Symbol.SymbolID = 2;
	}
	TMap<int32, UDungeonMissionNode*> nodeMap;
	nodeMap.Add(1, startLocation);
	if (replaceLocation != NULL)
	{
		nodeMap.Add(2, replaceLocation);
	}

	// Break their parent-child link
	startLocation->BreakLinkWithNode(replaceLocation);

	FString initialShape = startLocation->GetSymbolDescription();
	if (replaceLocation != NULL)
	{
		initialShape.Append("->");
		initialShape.Append(replaceLocation->GetSymbolDescription());
	}
	const UGraphOutputGrammar* shape = GrammarReplaceResult.Grammar;
	FString grammarChain = shape->ToString();
	UE_LOG(LogMissionGen, Log, TEXT("Replacing %s with %s."), *initialShape, *grammarChain);

	TArray<FGraphLink> toProcess;
	FGraphLink head = shape->Head;
	if (!startLocation->Symbol.Symbol->bIsTerminalNode)
	{
		startLocation->Symbol = head.Symbol;
	}

	toProcess.Add(shape->Head);
	while(toProcess.Num() > 0)
	{
		FNumberedGraphSymbol fromSymbol = toProcess[0].Symbol;
		toProcess.RemoveAt(0);
		if (fromSymbol.Symbol == NULL)
		{
			continue;
		}
		checkf(nodeMap.Contains(fromSymbol.SymbolID), TEXT("Shape did not contain symbol ID %d! Did you set it?"), fromSymbol.SymbolID);
		// It is assumed that the from node is already in the map
		// It is also assumed that the from node has already replaced its symbol
		UDungeonMissionNode* fromNode = nodeMap[fromSymbol.SymbolID];

		TSet<FGraphLink> children = shape->GetSymbolChildren(fromSymbol);
		for (FGraphLink& child : children)
		{
			// Create nodes for all children of this node
			UDungeonMissionNode* toNode;
			
			/*if (child.Symbol.SymbolID == 2 && replaceLocation != NULL)
			{
				// This is the replacement for the "last" node in our shape
				// This may not necessarily be the actual last node, but it's
				// still replacing one of the initial nodes we started with.
				toNode = replaceLocation;
			}
			else */if (nodeMap.Contains(child.Symbol.SymbolID))
			{
				toNode = nodeMap[child.Symbol.SymbolID];
			}
			else
			{
				// Create a new node
				toNode = NewObject<UDungeonMissionNode>();
				if (child.Symbol.SymbolID == 2)
				{
					// This is what would be the "last" node in our shape.
					// However, we only needed to match one node.
					replaceLocation = toNode;
				}
			}
			// Change the symbol on the node
			if (toNode->Symbol.Symbol == NULL || !toNode->Symbol.Symbol->bIsTerminalNode)
			{
				toNode->Symbol = child.Symbol;
			}
			toNode->ParentNodes.Add(fromNode);
			toNode->bTightlyCoupledToParent = child.bIsTightlyCoupled;

			// Update parent node metadata
			FMissionNodeData nodeMetadata;
			nodeMetadata.bTightlyCoupledToParent = child.bIsTightlyCoupled;
			nodeMetadata.Node = toNode;

			check(nodeMetadata.Node->Symbol.Symbol);
			// Add it to the parent
			fromNode->NextNodes.Add(nodeMetadata);

			// Update the node lookup
			nodeMap.Add(child.Symbol.SymbolID, toNode);
			// Add this child to our list of nodes to process for more children
			toProcess.Add(child);
		}
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogMissionGen, Log, TEXT("Dungeon after replacement:"));
	PrintDebugDungeon();
#endif
}
