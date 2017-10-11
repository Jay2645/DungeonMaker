// BSP Leaf, used for random dungeon generation
// Based on https://gamedevelopment.tutsplus.com/tutorials/how-to-use-bsp-trees-to-generate-game-maps--gamedev-12268

#include "BSPLeaf.h"
#include "DungeonMissionSymbol.h"
#include <DrawDebugHelpers.h>

// Sets default values for this component's properties
UBSPLeaf::UBSPLeaf()
{
	LeftChild = NULL;
	RightChild = NULL;
	RoomSymbol = NULL;
	Parent = NULL;

	XPosition = 0;
	YPosition = 0;

	LeafSize = FDungeonFloor(MIN_LEAF_SIZE, MIN_LEAF_SIZE);
	Room = FDungeonRoom();
	RoomOffset = FIntVector();
	LeafNeighbors = TSet<UBSPLeaf*>();
	MissionNeighbors = TSet<UBSPLeaf*>();
}

void  UBSPLeaf::InitializeLeaf(int32 X, int32 Y, int32 Width, int32 Height, UBSPLeaf* ParentLeaf)
{
	XPosition = X;
	YPosition = Y;
	LeafSize = FDungeonFloor(Width, Height);
	Room = FDungeonRoom(Width, Height);
	Parent = ParentLeaf;
}

bool UBSPLeaf::Split(FRandomStream& Rng)
{
	if (HasChildren())
	{
		// Already split
		return false;
	}

	// Determine direction of split
	// If the width is >25% larger than height, we split vertically
	// If the height is >25% larger than the width, we split horizontally
	// Otherwise we split randomly
	bool splitHorizontal = Rng.GetFraction() > 0.5f;
	if (splitHorizontal && LeafSize.XSize() > LeafSize.YSize() && ((float)LeafSize.XSize() / (float)LeafSize.YSize() >= 1.25))
	{
		splitHorizontal = false;
	}
	else if(!splitHorizontal && LeafSize.XSize() < LeafSize.YSize() && ((float)LeafSize.YSize() / (float)LeafSize.XSize() >= 1.25))
	{
		splitHorizontal = true;
	}

	// Determine the maximum height or width
	int32 max = (splitHorizontal ? LeafSize.YSize() : LeafSize.XSize()) - (int32)MIN_LEAF_SIZE;
	if (max < MIN_LEAF_SIZE)
	{
		// Too small to continue splitting
		return false;
	}

	int32 splitLocation = Rng.RandRange(MIN_LEAF_SIZE, max);
	// Do the actual split, based on the direction we determined
	LeftChild = NewObject<UBSPLeaf>();
	RightChild = NewObject<UBSPLeaf>();
	if (splitHorizontal)
	{
		LeftChild->InitializeLeaf(XPosition, YPosition, LeafSize.XSize(), splitLocation, this);
		RightChild->InitializeLeaf(XPosition, YPosition + splitLocation, LeafSize.XSize(), LeafSize.YSize() - splitLocation, this);
	}
	else
	{
		LeftChild->InitializeLeaf(XPosition, YPosition, splitLocation, LeafSize.YSize(), this);
		RightChild->InitializeLeaf(XPosition + splitLocation, YPosition, LeafSize.XSize() - splitLocation, LeafSize.YSize(), this);
	}
	// Done splitting
	return true;
}

void UBSPLeaf::DetermineNeighbors()
{
	UBSPLeaf* northNeighbor = GetLeaf(XPosition, YPosition + LeafSize.YSize() + 1);
	UBSPLeaf* southNeighbor = GetLeaf(XPosition, YPosition - 1);
	UBSPLeaf* eastNeighbor = GetLeaf(XPosition + LeafSize.XSize() + 1, YPosition);
	UBSPLeaf* westNeighbor = GetLeaf(XPosition - 1, YPosition);

	if (northNeighbor != NULL)
	{
		LeafNeighbors.Add(northNeighbor);
	}
	if (southNeighbor != NULL)
	{
		LeafNeighbors.Add(southNeighbor);
	}
	if (eastNeighbor != NULL)
	{
		LeafNeighbors.Add(eastNeighbor);
	}
	if (westNeighbor != NULL)
	{
		LeafNeighbors.Add(westNeighbor);
	}
}

void UBSPLeaf::SetMissionNode(UDungeonMissionNode* Node, const UDungeonTile* DefaultRoomTile, FRandomStream& Rng)
{
	RoomSymbol = Node;
	if (Node != NULL)
	{
		CreateRoom(DefaultRoomTile, Rng);
	}
}

void UBSPLeaf::CreateRoom(const UDungeonTile* DefaultRoomTile, FRandomStream &Rng)
{
	FMissionSpaceData minimumRoomSize = ((UDungeonMissionSymbol*)RoomSymbol->Symbol.Symbol)->MinimumRoomSize;
	int32 xDimension = Rng.RandRange(minimumRoomSize.WallSize, LeafSize.XSize());
	int32 yDimension = Rng.RandRange(minimumRoomSize.WallSize, LeafSize.YSize());
	Room = FDungeonRoom(xDimension, yDimension);
	Room.Symbol = (UDungeonMissionSymbol*)RoomSymbol->Symbol.Symbol;

	// Initialize the room with the default tiles
	for (int x = 0; x < Room.XSize(); x++)
	{
		for (int y = 0; y < Room.YSize(); y++)
		{
			Room.Set(x, y, DefaultRoomTile);
		}
	}
	// Replace them based on our replacement rules
	TArray<FRoomReplacements> replacementPhases = Room.Symbol->RoomReplacementPhases;
	for (int i = 0; i < replacementPhases.Num(); i++)
	{
		TArray<URoomReplacementPattern*> replacementPatterns = replacementPhases[i].ReplacementPatterns;
		while (replacementPatterns.Num() > 0)
		{
			int32 rngIndex = Rng.RandRange(0, replacementPatterns.Num() - 1);
			if (!replacementPatterns[rngIndex]->FindAndReplace(Room))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}

	// X Offset can be anywhere from our current X position to the start of the room
	// That way we have enough space to place the room
	int32 xOffset = Rng.RandRange(XPosition, LeafSize.XSize() - xDimension);
	int32 yOffset = Rng.RandRange(YPosition, LeafSize.YSize() - yDimension);

	RoomOffset = FIntVector(xOffset, yOffset, 0);
}

bool UBSPLeaf::HasChildren() const
{
	return LeftChild != NULL && RightChild != NULL;
}


bool UBSPLeaf::SideIsLargerThan(int32 Size)
{
	return LeafSize.XSize() > Size || LeafSize.YSize() > Size;
}

bool UBSPLeaf::ContainsPosition(int32 XPos, int32 YPos) const
{
	// Check X first
	bool inX = XPos >= XPosition && XPos < XPosition + LeafSize.XSize();
	if (!inX)
	{
		return false;
	}
	// Now check Y
	return YPos >= YPosition && YPos < YPosition + LeafSize.YSize();
}

void UBSPLeaf::SetTile(int32 XPos, int32 YPos, const UDungeonTile* TileToSet)
{
	if (HasChildren())
	{
		if (LeftChild->ContainsPosition(XPos, YPos))
		{
			LeftChild->SetTile(XPos, YPos, TileToSet);
		}
		else if (RightChild->ContainsPosition(XPos, YPos))
		{
			RightChild->SetTile(XPos, YPos, TileToSet);
		}
		else
		{
			checkNoEntry();
		}
	}
	else
	{
		LeafSize.Set(XPos, YPos, TileToSet);
	}
}

UBSPLeaf* UBSPLeaf::GetLeaf(int32 XPos, int32 YPos)
{
	if (HasChildren())
	{
		if (LeftChild->ContainsPosition(XPos, YPos))
		{
			return LeftChild->GetLeaf(XPos, YPos);
		}
		else if (RightChild->ContainsPosition(XPos, YPos))
		{
			return RightChild->GetLeaf(XPos, YPos);
		}
		else if (Parent != NULL)
		{
			return Parent->GetLeaf(XPos, YPos);
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		if (ContainsPosition(XPos, YPos))
		{
			return this;
		}
		else if (Parent != NULL)
		{
			return Parent->GetLeaf(XPos, YPos);
		}
		else
		{
			return NULL;
		}
	}
}

const UDungeonTile* UBSPLeaf::GetTile(int32 XPos, int32 YPos)
{
	if (HasChildren())
	{
		if (LeftChild->ContainsPosition(XPos, YPos))
		{
			return LeftChild->GetTile(XPos, YPos);
		}
		else if (RightChild->ContainsPosition(XPos, YPos))
		{
			return RightChild->GetTile(XPos, YPos);
		}
		else
		{
			checkNoEntry();
			return NULL;
		}
	}
	else
	{
		return LeafSize[YPos][XPos];
	}
}

int32 UBSPLeaf::GetLeafCount() const
{
	// Count ourselves
	int splitCount = 1;

	// Count our children
	if (LeftChild != NULL)
	{
		splitCount += LeftChild->GetLeafCount();
	}
	if (RightChild != NULL)
	{
		splitCount += RightChild->GetLeafCount();
	}
	return splitCount;
}

int32 UBSPLeaf::GetChildLeafCount() const
{
	// If we are a child leaf, return ourselves
	if (!HasChildren())
	{
		return 1;
	}

	// If we are a parent, count our children
	int splitCount = 0;
	if (LeftChild != NULL)
	{
		splitCount += LeftChild->GetLeafCount();
	}
	if (RightChild != NULL)
	{
		splitCount += RightChild->GetLeafCount();
	}
	return splitCount;
}

FString UBSPLeaf::ToString() const
{
	FString output = "";

	if (HasChildren())
	{
		bool bHorizontalSplit = LeftChild->XPosition == RightChild->XPosition;
		if (bHorizontalSplit)
		{
			FString leftOutput = LeftChild->ToString();
			FString rightOutput = RightChild->ToString();
			int32 currentRightIndex = 0;
			for (int i = 0; i < leftOutput.Len(); i++)
			{
				if (leftOutput[i] == '\n')
				{
					// Found a newline
					// Iterate over all characters in the right string
					output.AppendChar('_');

					for (int j = currentRightIndex; j < rightOutput.Len(); j++)
					{
						output.AppendChar(rightOutput[j]);
						if (rightOutput[j] == '\n')
						{
							// Done with this line of the right output
							// Next time, start at the beginning of the next line
							currentRightIndex = j + 1;
							// Output already contains a newline
							break;
						}
					}
				}
				else
				{
					// Add the current character to the output
					output.AppendChar(leftOutput[i]);
				}
			}
		}
		else
		{
			output = LeftChild->ToString();
			output += "\n\n";
			output += RightChild->ToString();
		}
	}
	else
	{
		output = LeafSize.ToString();
	}
	return output;
}

void UBSPLeaf::DrawDebugLeaf(AActor* ReferenceActor, float ZPos, bool bDebugLeaf)
{
	if (HasChildren())
	{
		LeftChild->DrawDebugLeaf(ReferenceActor, ZPos + 100.0f, bDebugLeaf);
		RightChild->DrawDebugLeaf(ReferenceActor, ZPos + 100.0f, bDebugLeaf);
	}
	else
	{
		if (RoomSymbol == NULL || RoomSymbol->Symbol.Symbol == NULL)
		{
			return;
		}
		float halfX = LeafSize.XSize() / 2.0f;
		float halfY = LeafSize.YSize() / 2.0f;

		float midX = (XPosition + halfX) * 500.0f;
		float midY = (YPosition + halfY) * 500.0f;
		FColor randomColor = FColor::MakeRandomColor();
		if (bDebugLeaf || RoomOffset.IsZero())
		{
			for (int x = XPosition; x < XPosition + LeafSize.XSize(); x++)
			{
				for (int y = YPosition; y < YPosition + LeafSize.YSize(); y++)
				{
					FVector startingLocation(x * 500.0f, y * 500.0f, ZPos);
					FVector endingLocation(x * 500.0f, (y + 1) * 500.0f, ZPos);

					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					endingLocation = FVector((x + 1) * 500.0f, y * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					startingLocation = FVector((x + 1) * 500.0f, (y + 1) * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					endingLocation = FVector(x * 500.0f, (y + 1) * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
				}
			}
		}
		else
		{
			for (int x = 0; x < 0 + Room.XSize(); x++)
			{
				for (int y = 0; y < 0 + Room.YSize(); y++)
				{
					int32 xOffset = x + XPosition;
					int32 yOffset = y + YPosition;
					FVector startingLocation(xOffset * 500.0f, yOffset * 500.0f, ZPos);
					FVector endingLocation(xOffset * 500.0f, (yOffset + 1) * 500.0f, ZPos);

					// Draw a square
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					endingLocation = FVector((xOffset + 1) * 500.0f, yOffset * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					startingLocation = FVector((xOffset + 1) * 500.0f, (yOffset + 1) * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);
					endingLocation = FVector(xOffset * 500.0f, (yOffset + 1) * 500.0f, ZPos);
					DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true);

					// Label the center with the type of tile this is
					FVector midpoint((xOffset + 0.5f) * 500.0f, (yOffset + 0.5f) * 500.0f, ZPos + 100.0f);
					const UDungeonTile* tile = Room[y][x];
					if (tile != NULL)
					{
						DrawDebugString(ReferenceActor->GetWorld(), midpoint, tile->TileID.ToString());
					}
				}
			}
			
			// Draw lines connecting to our neighbors
			FVector startingLocation = FVector(midX, midY, ZPos);
			for (UBSPLeaf* neighbor : MissionNeighbors)
			{
				if (neighbor->RoomSymbol == NULL || neighbor->RoomSymbol->Symbol.Symbol == NULL)
				{
					continue;
				}
				float neighborHalfX = neighbor->LeafSize.XSize() / 2.0f;
				float neighborHalfY = neighbor->LeafSize.YSize() / 2.0f;

				float neighborMidX = (neighbor->XPosition + neighborHalfX) * 500.0f;
				float neighborMidY = (neighbor->YPosition + neighborHalfY) * 500.0f;
				FVector endingLocation = FVector(neighborMidX, neighborMidY, ZPos);
				DrawDebugLine(ReferenceActor->GetWorld(), startingLocation, endingLocation, randomColor, true, -1.0f, (uint8)'\000', 100.0f);
			}
		}

		if (RoomSymbol != NULL && RoomSymbol->Symbol.Symbol != NULL)
		{
			DrawDebugString(ReferenceActor->GetWorld(), FVector(midX, midY, ZPos + 200.0f), RoomSymbol->GetSymbolDescription());
		}
	}
}

void UBSPLeaf::AddMissionLeaf(UBSPLeaf* Neighbor)
{
	//check(LeafNeighbors.Contains(Neighbor));
	if (Neighbor == NULL)
	{
		return;
	}
	MissionNeighbors.Add(Neighbor);
}

bool UBSPLeaf::HasConnectionTo(UBSPLeaf* Root)
{
	TSet<UBSPLeaf*> attempted;
	return HasConnectionTo(Root, attempted);
}

bool UBSPLeaf::HasConnectionTo(UBSPLeaf* Root, TSet<UBSPLeaf*>& Attempted)
{
	if (Attempted.Contains(this))
	{
		return false;
	}

	// Make sure we don't get processed again
	Attempted.Add(this);

	if (Root == this)
	{
		// Hey, that's us!
		return true;
	}
	if (HasChildren())
	{
		return false;
	}

	// Get all neighbors we haven't processed yet
	TSet<UBSPLeaf*> neighbors = MissionNeighbors.Difference(Attempted);
	if (neighbors.Num() == 0)
	{
		// Out of neighbors
		return false;
	}
	if (neighbors.Contains(Root))
	{
		// We're directly connected to the root!
		return true;
	}
	else
	{
		// See if one of our neighbors is connected to the root
		for (UBSPLeaf* neighbor : neighbors)
		{
			if (neighbor->HasConnectionTo(Root))
			{
				return true;
			}
		}
		return false;
	}
}


void UBSPLeaf::PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> ComponentLookup)
{
	for (int x = 0; x < Room.XSize(); x++)
	{
		for (int y = 0; y < Room.YSize(); y++)
		{
			const UDungeonTile* tile = Room[y][x];
			if (tile->TileMesh == NULL)
			{
				continue;
			}
			FVector startingLocation((x + XPosition) * 500.0f, (y + YPosition) * 500.0f, 0.0f);
			FTransform tileTfm;
			tileTfm.SetLocation(startingLocation);
			UHierarchicalInstancedStaticMeshComponent* meshComponent = ComponentLookup[tile];
			meshComponent->AddInstance(tileTfm);
		}
	}
}

bool UBSPLeaf::AreChildrenAllowed() const
{
	if (RoomSymbol == NULL)
	{
		return false;
	}
	else
	{
		return ((UDungeonMissionSymbol*)RoomSymbol->Symbol.Symbol)->bAllowedToHaveChildren;
	}
}
