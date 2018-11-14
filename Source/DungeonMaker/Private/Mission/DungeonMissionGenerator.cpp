// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonMissionGenerator.h"
#include "DungeonMaker.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "Grammar/Grammar.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UDungeonMissionGenerator::UDungeonMissionGenerator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonMissionGenerator::TryToCreateDungeon(FRandomStream& Stream)
{
	Head = NewObject<UDungeonMissionNode>();
	Head->NodeType = HeadSymbol.Symbol;
	Head->NodeID = HeadSymbol.SymbolID;
	Head->bTightlyCoupledToParent = false;
	UE_LOG(LogSpaceGen, Log, TEXT("Started with head node %s."), *Head->GetSymbolDescription());
	DungeonSize = 0;

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

		current->NodeID = currentID;
		currentID++;
		for (UDungeonMakerNode* node : current->ChildrenNodes)
		{
			nodes.Add((UDungeonMissionNode*)node);
		}
		processed.Add(current);

		DungeonSize++;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogMissionGen, Log, TEXT("Completed dungeon:"));
	PrintDebugDungeon();
#endif
}

void UDungeonMissionGenerator::FindNodeMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars, 
	UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars)
{
	bool bFoundMatches = OutAcceptableGrammars.Num() > 0;
	FGraphLink us;
	us.Symbol = StartingLocation->ToGraphSymbol();
	us.bIsTightlyCoupled = StartingLocation->bTightlyCoupledToParent;

	// We're only checking if us by ourselves is valid, so the array just needs to contain us.
	TArray<FGraphLink> links;
	links.Add(us);

	UE_LOG(LogMissionGen, Verbose, TEXT("Checking if %s is a valid input."), *us.Symbol.GetSymbolDescription());

	CheckGrammarMatches(AllowedGrammars, links, StartingLocation, bFoundMatches, OutAcceptableGrammars);
}

void UDungeonMissionGenerator::FindMatchesWithChildren(TArray<const UDungeonMissionGrammar*>& AllowedGrammars, 
	UDungeonMissionNode* StartingLocation, TArray<FGraphOutput>& OutAcceptableGrammars)
{
	// We have children; we should check to see if we have a grammar which accepts us and our children
	// Define us first
	FGraphLink us;
	us.Symbol = StartingLocation->ToGraphSymbol();
	us.bIsTightlyCoupled = StartingLocation->bTightlyCoupledToParent;

	UE_LOG(LogMissionGen, Verbose, TEXT("Trying to match childen of %s!"), *us.Symbol.GetSymbolDescription());

	// Iterate over each child
	for (UDungeonMakerNode* nextNode : StartingLocation->ChildrenNodes)
	{
		// Add ourselves to the array
		TArray<FGraphLink> links;
		links.Add(us);

		FGraphLink next;
		next.Symbol = nextNode->ToGraphSymbol();
		next.bIsTightlyCoupled = nextNode->bTightlyCoupledToParent;

		links.Add(next);

		UE_LOG(LogMissionGen, Verbose, TEXT("Checking coupling %s->%s"), *us.Symbol.GetSymbolDescription(), *next.Symbol.GetSymbolDescription());

		CheckGrammarMatches(AllowedGrammars, links, StartingLocation, false, OutAcceptableGrammars);
	}

#if !UE_BUILD_SHIPPING
	if (OutAcceptableGrammars.Num() == 0)
	{
		UE_LOG(LogMissionGen, Warning, TEXT("No symbols matched any child combination of %s."), *StartingLocation->GetSymbolDescription());
	}
#endif
}

void UDungeonMissionGenerator::CheckGrammarMatches(TArray<const UDungeonMissionGrammar*>& AllowedGrammars,
	const TArray<FGraphLink>& Links, UDungeonMissionNode* StartingLocation, bool bFoundMatches, 
	TArray<FGraphOutput>& OutAcceptableGrammars)
{
	for (int i = 0; i < AllowedGrammars.Num(); i++)
	{
		// Iterate over all grammars
		const UDungeonMissionGrammar* grammar = AllowedGrammars[i];

		EGrammarResultType resultType = grammar->MatchesGrammar(this, Links);
		if (resultType == EGrammarResultType::Accepted)
		{
			// We can replace ourselves with a new symbol!

			UDungeonMakerGraph* graph = ((UGraphGrammar*)grammar)->OutputGraph;
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
			UE_LOG(LogMissionGen, Verbose, TEXT("Matching grammar found! %s can be replaced by %s."), *linkString, *graph->ToString());
#endif
			// Make us less likely to be chosen if we've been chosen a lot before
			float weightModifier = 1.0f;
			FString outputString = graph->ToString();
			if (GrammarUsageCount.Contains(outputString))
			{
				weightModifier /= GrammarUsageCount[outputString];
			}
			if (bFoundMatches)
			{
				// We already have good matches which match more nodes, so it should be less likely to match these ones
				weightModifier *= 0.25f;
			}
			FGraphOutput replaceResult;
			replaceResult.Graph = graph;
			replaceResult.Weight = grammar->Weight * weightModifier;
			replaceResult.MatchedLinks = Links;
			// Add it to the list of things we can do to ourselves
			OutAcceptableGrammars.Add(replaceResult);
		}
	}
}

void UDungeonMissionGenerator::DrawDebugDungeon()
{
	check(Head != NULL && Head->NodeType != NULL);
	int32 dungeonDepth = Head->GetLevelCount();
	TMap<UDungeonMakerNode*, FIntVector> dungeonCoords;
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
			for (UDungeonMakerNode* node : next->ChildrenNodes)
			{
				int32 nodesBelow = ((UDungeonMissionNode*)node)->GetLevelCount();
				if (nodesBelow <= 1)
				{
					// Send to the bottom
					nodesToDraw.Enqueue((UDungeonMissionNode*)node);
					dungeonCoords.Add(node, FIntVector(xOffset + k, dungeonDepth - 1, 0));
					// Increment the x offset; the bottom of this one is already accounted for
					xOffset++;
					continue;
				}

				// Otherwise, our child has child nodes
				// This will never overflow because we already know how many children have child nodes
				allNodes[j + 1].Add((UDungeonMissionNode*)node);
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
		DrawDebugString(GetWorld(), drawLocation + FVector(0.0f, 0.0f, 100.0f), next->GetSymbolDescription());
		for (UDungeonMakerNode* node : next->ChildrenNodes)
		{
			FIntVector childLocation = dungeonCoords[node];
			FVector drawChildLocation = FVector(childLocation.X * 100.0f, childLocation.Y * 100.0f, 0.0f);
			FColor lineColor;
			if (node->bTightlyCoupledToParent)
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
	check(Head != NULL && Head->NodeType != NULL);
	UE_LOG(LogMissionGen, Log, TEXT("%s"), *Head->ToString(0));
}

void UDungeonMissionGenerator::TryToCreateDungeon(UDungeonMissionNode* StartingLocation, 
	TArray<const UDungeonMissionGrammar*> AllowedGrammars, FRandomStream& Rng, int32 RemainingMaxStepCount)
{
	checkf(StartingLocation != NULL, TEXT("Starting node for dungeon generation was null!"));
	checkf(StartingLocation->IsValidLowLevel(), TEXT("Starting node for dungeon generation was invalid!"));
	checkf(StartingLocation->NodeType != NULL, TEXT("Starting node for dungeon generation had no symbols!"));
	checkf(AllowedGrammars.Num() > 0, TEXT("There were no allowed grammars for dungeon generation!"));
	checkf(RemainingMaxStepCount >= 0, TEXT("Dungeon generation ran out of steps! You have an overflow issue."));

	if (StartingLocation->NodeType->bIsTerminalNode)
	{
		// This node has already been processed completely and turned into a terminal node
		for (UDungeonMakerNode* node : StartingLocation->ChildrenNodes)
		{
			// Try and process each child
			TryToCreateDungeon((UDungeonMissionNode*)node, AllowedGrammars, Rng, RemainingMaxStepCount - 1);
		}
		return;
	}

	UE_LOG(LogMissionGen, Log, TEXT("Trying to create a dungeon starting from %s."), *StartingLocation->GetSymbolDescription());

	TArray<FGraphOutput> acceptableGrammars;
	if (StartingLocation->ChildrenNodes.Num() > 0)
	{
		FindMatchesWithChildren(AllowedGrammars, StartingLocation, acceptableGrammars);
	}

	// Try to see if we have a grammar that accepts only us
	FindNodeMatches(AllowedGrammars, StartingLocation, acceptableGrammars);

	UE_LOG(LogMissionGen, Verbose, TEXT("Found %d acceptable grammars for %s."), acceptableGrammars.Num(), *StartingLocation->GetSymbolDescription());

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
		for (UDungeonMakerNode* node : StartingLocation->ChildrenNodes)
		{
			// Try and process each child
			TryToCreateDungeon((UDungeonMissionNode*)node, AllowedGrammars, Rng, RemainingMaxStepCount - 1);
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

	FString grammarString = grammarReplaceResult.Graph->ToString();
	if (GrammarUsageCount.Contains(grammarString))
	{
		GrammarUsageCount[grammarString] += 1;
	}
	else
	{
		GrammarUsageCount.Add(grammarString, 1);
	}

	// Actually do the replacement
	ReplaceNodes(StartingLocation, grammarReplaceResult);
}

void UDungeonMissionGenerator::ReplaceNodes(UDungeonMissionNode* StartingLocation, 
	const FGraphOutput& GrammarReplaceResult)
{
	// Find the matched nodes
	UDungeonMissionNode* startLocation = StartingLocation;
	UDungeonMissionNode* replaceLocation = NULL;
	if (GrammarReplaceResult.MatchedLinks.Num() > 1)
	{
		replaceLocation = (UDungeonMissionNode*)StartingLocation->FindChildNodeFromSymbol(GrammarReplaceResult.MatchedLinks[1].Symbol);
	}

	// Number the nodes
	startLocation->NodeID = 1;
	if (replaceLocation != NULL)
	{
		replaceLocation->NodeID = 2;
	}

	FString initialShape = startLocation->GetSymbolDescription();
	if (replaceLocation != NULL)
	{
		initialShape.Append("->");
		initialShape.Append(replaceLocation->GetSymbolDescription());
	}

	TMap<int32, UDungeonMissionNode*> nodeMap;
	nodeMap.Add(1, startLocation);
	if (replaceLocation != NULL)
	{
		nodeMap.Add(2, replaceLocation);
	}

	// Break their parent-child link
	UDungeonMakerGraph* graph = GrammarReplaceResult.Graph;
	if (graph->GetLevelNum() == 0)
	{
		UE_LOG(LogMissionGen, Error, TEXT("Replacement grammar was null! Nodes that were to be replaced: %s"), *initialShape);
		return;
	}

	graph->UpdateIDs();

	FString grammarChain = graph->ToString();
	UE_LOG(LogMissionGen, Log, TEXT("Replacing %s with %s (Total Length: %d)."), *initialShape, *grammarChain, graph->Num());

	TArray<UDungeonMakerNode*> toProcess;
	if (!graph->NodeIDLookup.Contains(1))
	{
		UE_LOG(LogMissionGen, Error, TEXT("No root symbol found when replacing %s with %s."), *initialShape, *grammarChain);
		return;
	}
	UDungeonMakerNode* head = graph->NodeIDLookup[1];
	if (head->NodeType == NULL)
	{
		UE_LOG(LogMissionGen, Error, TEXT("Encounted a null head symbol replacing %s with %s."), *initialShape, *grammarChain);
		return;
	}

	if (!startLocation->NodeType->bIsTerminalNode)
	{
		if (startLocation->NodeType == NULL)
		{
			UE_LOG(LogMissionGen, Log, TEXT("Changing head node %s into %s."), *startLocation->NodeType->Description.ToString(), *head->NodeType->Description.ToString());
		}
		startLocation->NodeType = head->NodeType;
	}

	if (graph->Num() == 2 && replaceLocation != NULL)
	{
		if (replaceLocation->NodeType != NULL)
		{
			UE_LOG(LogMissionGen, Log, TEXT("Changing %s into %s."), *replaceLocation->NodeType->Description.ToString(), *graph->AllNodes[1]->NodeType->Description.ToString());
		}
		replaceLocation->NodeType = graph->AllNodes[1]->NodeType;
		replaceLocation->bTightlyCoupledToParent = graph->AllNodes[1]->bTightlyCoupledToParent;
	}
	else if(graph->Num() > 2)
	{
		if (replaceLocation != NULL)
		{
			startLocation->BreakLinkWithNode(replaceLocation);
		}

		toProcess.Add(head);

		// Process the head and all its children
		while (toProcess.Num() > 0)
		{
			UDungeonMakerNode* node = toProcess[0];
			toProcess.RemoveAt(0);

			FNumberedGraphSymbol fromSymbol = node->ToGraphSymbol();
			if (fromSymbol.Symbol == NULL)
			{
				UE_LOG(LogMissionGen, Error, TEXT("Encounted a null symbol when replacing %s with %s."), *initialShape, *grammarChain);
				continue;
			}
			checkf(nodeMap.Contains(fromSymbol.SymbolID), TEXT("Shape did not contain symbol ID %d! Did you remember to add it to the output grammar?"), fromSymbol.SymbolID);

			// It is assumed that the from node is already in the map
			// It is also assumed that the from node has already replaced its symbol
			UDungeonMissionNode* fromNode = nodeMap[fromSymbol.SymbolID];

			TArray<UDungeonMakerNode*> children = node->ChildrenNodes;
			UE_LOG(LogMissionGen, Verbose, TEXT("Processing %s, with %d children."), *fromNode->ToString(0, false), children.Num());

			for (int i = 0; i < children.Num(); i++)
			{
				UDungeonMakerNode* child = children[i];
				if (child->NodeType == NULL)
				{
					UE_LOG(LogMissionGen, Error, TEXT("%s had a null child symbol."), *fromSymbol.GetSymbolDescription());
					continue;
				}

				// Create nodes for all children of this node
				UDungeonMissionNode* toNode;
				FNumberedGraphSymbol childSymbol = child->ToGraphSymbol();
				if (nodeMap.Contains(childSymbol.SymbolID))
				{
					toNode = nodeMap[childSymbol.SymbolID];
				}
				else
				{
					// Create a new node
					toNode = NewObject<UDungeonMissionNode>();
					UE_LOG(LogMissionGen, Log, TEXT("Adding node: %s"), *childSymbol.GetSymbolDescription());
				}
				// Change the symbol on the node
				if (toNode->NodeType == NULL || !toNode->NodeType->bIsTerminalNode)
				{
#if !UE_BUILD_SHIPPING
					if (toNode->NodeType != NULL)
					{
						UE_LOG(LogMissionGen, Log, TEXT("Converting %s (%d) into %s."), *toNode->NodeType->Description.ToString(), toNode->NodeID, *childSymbol.GetSymbolDescription());
					}
#endif
					toNode->NodeType = childSymbol.Symbol;
					toNode->NodeID = childSymbol.SymbolID;
				}

				fromNode->AddLinkToNode(toNode, child->bTightlyCoupledToParent);

				// Update the node lookup
				nodeMap.Add(child->NodeID, toNode);
				// Add this child to our list of nodes to process for more children
				toProcess.Add(child);
			}
		}

		if (replaceLocation != NULL && nodeMap.Contains(2))
		{
			if (nodeMap[2] != replaceLocation)
			{
				nodeMap[2]->AddLinkToNode(replaceLocation, replaceLocation->bTightlyCoupledToParent);
			}
		}
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogMissionGen, Log, TEXT("Dungeon after replacement:"));
	PrintDebugDungeon();
#endif
}
