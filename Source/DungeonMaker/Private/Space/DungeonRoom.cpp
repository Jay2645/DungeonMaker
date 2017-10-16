// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonRoom.h"
#include <DrawDebugHelpers.h>
#include "../../Public/Space/BSP/BSPLeaf.h"
#include "GameFramework/Character.h"


// Sets default values for this component's properties
ADungeonRoom::ADungeonRoom()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	RoomTiles = FDungeonRoomMetadata();
	Symbol = NULL;
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Room Trigger"));
	RoomTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FCollisionResponseContainer collisonChannels;
	collisonChannels.SetAllChannels(ECR_Ignore);
	collisonChannels.SetResponse(ECollisionChannel::ECC_Pawn, ECR_Overlap);
	RoomTrigger->SetCollisionResponseToChannels(collisonChannels);
	RoomTrigger->AttachToComponent(DummyRoot, FAttachmentTransformRules::KeepWorldTransform);
	RoomTrigger->bGenerateOverlapEvents = true;
	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginTriggerOverlap);
}


TArray<FIntVector> ADungeonRoom::GetTileLocations(const UDungeonTile* Tile)
{
	TArray<FIntVector> locations;
	for (int x = 0; x < RoomTiles.XSize(); x++)
	{
		for (int y = 0; y < RoomTiles.YSize(); y++)
		{
			if (RoomTiles[y][x] == Tile)
			{
				locations.Add(FIntVector(x, y, 0));
			}
		}
	}
	return locations;
}

void ADungeonRoom::OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
		{
			// This controller belongs to the player
			OnPlayerEnterRoom();
			for (ADungeonRoom* neighbor : MissionNeighbors)
			{
				neighbor->OnPlayerEnterNeighborRoom();
			}
		}
	}
}

void ADungeonRoom::InitializeRoomFromPoints(
	const UDungeonTile* DefaultRoomTile, const UDungeonMissionSymbol* RoomSymbol,
	FIntVector StartLocation, FIntVector EndLocation, int32 Width)
{
	FRandomStream rng;
	if (StartLocation.X == EndLocation.X)
	{
		// This is a super-hacky way to make sure that we don't put wall endcaps on the end of our hallways
		// TODO: FIX THIS AT SOME POINT
		InitializeRoom(DefaultRoomTile, Width, FMath::Abs(EndLocation.Y - StartLocation.Y) + (Width * 2),
			StartLocation.X, StartLocation.Y - Width, StartLocation.Z,
			RoomSymbol, rng, false);
	}
	else if (StartLocation.Y == EndLocation.Y)
	{
		InitializeRoom(DefaultRoomTile, FMath::Abs(EndLocation.X - StartLocation.X) + (Width * 2), Width,
			StartLocation.X - Width, StartLocation.Y, StartLocation.Z,
			RoomSymbol, rng, false);
	}
}

void ADungeonRoom::InitializeRoom(const UDungeonTile* DefaultRoomTile,
	int32 MaxXSize, int32 MaxYSize,
	int32 XPosition, int32 YPosition, int32 ZPosition,
	const UDungeonMissionSymbol* RoomSymbol, FRandomStream &Rng,
	bool bUseRandomDimensions)
{
	Symbol = RoomSymbol;

	FMissionSpaceData minimumRoomSize = Symbol->MinimumRoomSize;
	int32 xDimension = FMath::Abs(MaxXSize);
	int32 yDimension = FMath::Abs(MaxYSize);
	if (bUseRandomDimensions)
	{
		xDimension = Rng.RandRange(minimumRoomSize.WallSize, xDimension);
		yDimension = Rng.RandRange(minimumRoomSize.WallSize, yDimension);
	}
	// Initialize the room with the default tiles
	RoomTiles = FDungeonRoomMetadata(xDimension, yDimension);
	for (int y = 0; y < yDimension; y++)
	{
		for (int x = 0; x < xDimension; x++)
		{
			Set(x, y, DefaultRoomTile);
		}
	}

	// X Offset can be anywhere from our current X position to the start of the room
	// That way we have enough space to place the room
	//int32 xOffset = Rng.RandRange(XPosition, MaxXSize - xDimension);
	//int32 yOffset = Rng.RandRange(YPosition, MaxYSize - yDimension);

	int32 xOffset = XPosition;
	int32 yOffset = YPosition;

	FVector worldPosition = FVector(
		xOffset * UDungeonTile::TILE_SIZE, 
		yOffset * UDungeonTile::TILE_SIZE, 
		ZPosition * UDungeonTile::TILE_SIZE);

	FVector halfExtents = FVector((xDimension * 250.0f), (yDimension * 250.0f), 500.0f);
	SetActorLocation(worldPosition);
	RoomTrigger->SetRelativeLocation(halfExtents - FVector(0.0f, 500.0f, 500.0f));
	RoomTrigger->SetBoxExtent(halfExtents);

	OnRoomInitialized();
}

void ADungeonRoom::DoTileReplacement(FDungeonFloor& DungeonFloor, FRandomStream &Rng)
{
	// Replace them based on our replacement rules
	TArray<FRoomReplacements> replacementPhases = RoomReplacementPhases;
	for (int i = 0; i < replacementPhases.Num(); i++)
	{
		TArray<URoomReplacementPattern*> replacementPatterns = replacementPhases[i].ReplacementPatterns;
		while (replacementPatterns.Num() > 0)
		{
			int32 rngIndex = Rng.RandRange(0, replacementPatterns.Num() - 1);
			if (!replacementPatterns[rngIndex]->FindAndReplace(RoomTiles))
			{
				// Couldn't find a replacement in this room
				replacementPatterns.RemoveAt(rngIndex);
			}
		}
	}

	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);

	for (int y = 0; y < YSize(); y++)
	{
		for (int x = 0; x < XSize(); x++)
		{
			const UDungeonTile* tile = GetTile(x, y);
			FIntVector currentLocation = FIntVector(x + xPosition, y + yPosition, 0);
			if (DungeonFloor.TileIsWall(currentLocation))
			{
				ADungeonRoom* otherRoom = DungeonFloor.GetRoom(currentLocation);
				if (otherRoom != NULL)
				{
					otherRoom->SetTileGridCoordinates(currentLocation, tile);
				}
				DungeonFloor.PlaceNewTile(currentLocation, this, tile);
			}
			else
			{
				Set(x, y, DungeonFloor.GetTileAt(currentLocation));
			}
		}
	}

	OnRoomTilesReplaced();
}

TSet<ADungeonRoom*> ADungeonRoom::MakeHallways(FRandomStream& Rng, const UDungeonTile* DefaultTile, const UDungeonMissionSymbol* HallwaySymbol)
{
	TSet<ADungeonRoom*> newHallways;
	TSet<ADungeonRoom*> allNeighbors = MissionNeighbors;
	for (ADungeonRoom* neighbor : allNeighbors)
	{
		newHallways.Append(ConnectRooms(this, neighbor, Rng, HallwaySymbol, DefaultTile));
	}
	return newHallways;
}

void ADungeonRoom::PlaceRoomTiles(TMap<const UDungeonTile*, UHierarchicalInstancedStaticMeshComponent*> ComponentLookup)
{
	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			const UDungeonTile* tile = GetTile(x, y);
			if (tile->TileMesh == NULL)
			{
				continue;
			}
			FTransform tileTfm = GetActorTransform();
			FVector offset = FVector(x * UDungeonTile::TILE_SIZE, y * UDungeonTile::TILE_SIZE, 0.0f);
			tileTfm.AddToTranslation(offset);
			UHierarchicalInstancedStaticMeshComponent* meshComponent = ComponentLookup[tile];
			meshComponent->AddInstance(tileTfm);
		}
	}
}



TSet<const UDungeonTile*> ADungeonRoom::FindAllTiles()
{
	return RoomTiles.FindAllTiles();
}

void ADungeonRoom::Set(int32 X, int32 Y, const UDungeonTile* Tile)
{
	RoomTiles.Set(X, Y, Tile);
}

const UDungeonTile* ADungeonRoom::GetTile(int32 X, int32 Y) const
{
	return RoomTiles.DungeonRows[Y].DungeonTiles[X];
}

int32 ADungeonRoom::XSize() const
{
	return RoomTiles.XSize();
}

int32 ADungeonRoom::YSize() const
{
	return RoomTiles.YSize();
}


int32 ADungeonRoom::ZSize() const
{
	// TODO: Support multiple levels
	return 1;
}

FString ADungeonRoom::ToString() const
{
	return RoomTiles.ToString();
}

void ADungeonRoom::DrawDebugRoom()
{
	if (XSize() == 0 || YSize() == 0)
	{
		UE_LOG(LogDungeonGen, Error, TEXT("%s had no room defined!"), *Symbol->Description.ToString());
	}

	float halfX = XSize() / 2.0f;
	float halfY = YSize() / 2.0f;

	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	int32 zPosition = FMath::RoundToInt(position.Z / UDungeonTile::TILE_SIZE);

	float midX = (xPosition + halfX) * UDungeonTile::TILE_SIZE;
	float midY = (yPosition + halfY) * UDungeonTile::TILE_SIZE;
	FColor randomColor = FColor::MakeRandomColor();

	for (int x = 0; x < XSize(); x++)
	{
		for (int y = 0; y < YSize(); y++)
		{
			int32 xOffset = x + xPosition;
			int32 yOffset = y + yPosition;
			FVector startingLocation(xOffset * UDungeonTile::TILE_SIZE, yOffset * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			FVector endingLocation(xOffset * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);

			// Draw a square
			DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector((xOffset + 1) * UDungeonTile::TILE_SIZE, yOffset * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true);
			startingLocation = FVector((xOffset + 1) * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true);
			endingLocation = FVector(xOffset * UDungeonTile::TILE_SIZE, (yOffset + 1) * UDungeonTile::TILE_SIZE, zPosition * UDungeonTile::TILE_SIZE);
			DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true);

			// Label the center with the type of tile this is
			FVector midpoint((xOffset + 0.5f) * UDungeonTile::TILE_SIZE, (yOffset + 0.5f) * UDungeonTile::TILE_SIZE, (zPosition * UDungeonTile::TILE_SIZE) + 100.0f);
			const UDungeonTile* tile = GetTile(x, y);
			if (tile != NULL)
			{
				DrawDebugString(GetWorld(), midpoint, tile->TileID.ToString());
			}
		}
	}

	// Draw lines connecting to our neighbors
	FVector startingLocation = FVector(midX, midY, zPosition);
	for (ADungeonRoom* neighbor : MissionNeighbors)
	{
		if (neighbor->Symbol == NULL)
		{
			continue;
		}
		float neighborHalfX = neighbor->XSize() / 2.0f;
		float neighborHalfY = neighbor->YSize() / 2.0f;


		FVector neighborPosition = neighbor->GetActorLocation();
		int32 neighborXPosition = FMath::RoundToInt(neighborPosition.X / UDungeonTile::TILE_SIZE);
		int32 neighborYPosition = FMath::RoundToInt(neighborPosition.Y / UDungeonTile::TILE_SIZE);
		int32 neighborZPosition = FMath::RoundToInt(neighborPosition.Z / UDungeonTile::TILE_SIZE);

		float neighborMidX = (neighborXPosition + neighborHalfX) * UDungeonTile::TILE_SIZE;
		float neighborMidY = (neighborYPosition + neighborHalfY) * UDungeonTile::TILE_SIZE;
		FVector endingLocation = FVector(neighborMidX, neighborMidY, zPosition * UDungeonTile::TILE_SIZE);
		DrawDebugLine(GetWorld(), startingLocation, endingLocation, randomColor, true, -1.0f, (uint8)'\000', 100.0f);
	}

	if (Symbol != NULL)
	{
		DrawDebugString(GetWorld(), FVector(midX, midY, (zPosition * UDungeonTile::TILE_SIZE) + 200.0f), GetName());
	}
}

bool ADungeonRoom::IsChangedAtRuntime() const
{
	if (Symbol == NULL)
	{
		return true;
	}
	return Symbol->bChangedAtRuntime;
}

TSet<ADungeonRoom*> ADungeonRoom::ConnectRooms(ADungeonRoom* A, ADungeonRoom* B, FRandomStream& Rng,
	const UDungeonMissionSymbol* HallwaySymbol, const UDungeonTile* DefaultTile)
{
	const int32 HALLWAY_WIDTH = 3;
	const int32 HALLWAY_EDGE_OFFSET = 2;

	TSet<ADungeonRoom*> hallways;
	// Convert room's world-space coordinates to grid coordinates
	FVector aComponentLocation = A->GetActorLocation();
	FVector bComponentLocation = B->GetActorLocation();
	FIntVector aLocation = FIntVector(FMath::RoundToInt(aComponentLocation.X / UDungeonTile::TILE_SIZE),
		FMath::RoundToInt(aComponentLocation.Y / UDungeonTile::TILE_SIZE),
		FMath::RoundToInt(aComponentLocation.Z / UDungeonTile::TILE_SIZE));
	FIntVector bLocation = FIntVector(FMath::RoundToInt(bComponentLocation.X / UDungeonTile::TILE_SIZE),
		FMath::RoundToInt(bComponentLocation.Y / UDungeonTile::TILE_SIZE),
		FMath::RoundToInt(bComponentLocation.Z / UDungeonTile::TILE_SIZE));

	// Each location corresponds to the top-left of the room
	// a|bLocation ______
	//            |  A|B |
	//            |______|
	//
	//    b|aLocation _________
	//               |   B|A   |
	//               |_________|

	// Sort them into top and bottom
	if (aLocation.Y > bLocation.Y)
	{
		// B is above A, reverse them
		FIntVector bottom = aLocation;
		aLocation = bLocation;
		bLocation = bottom;

		// Reverse the pointers as well
		ADungeonRoom* bottomRoom = A;
		A = B;
		B = bottomRoom;
	}
	// New:
	// aLocation ______
	//          |  A   |
	//          |______|
	//
	//    bLocation _________
	//             |    B    |
	//             |_________|

	// Edge case:
	// Rooms aligned on Y axis
	// bLocation _________     aLocation ______
	//          |    B    |             |   A  |
	//          |         |             |______|
	//          |_________|

	if (aLocation.Y == bLocation.Y)
	{
		// Set A to be the shorter of the two rooms
		if (B->YSize() < A->YSize())
		{
			FIntVector tempVector = aLocation;
			aLocation = bLocation;
			bLocation = tempVector;

			ADungeonRoom* tempRoom = A;
			A = B;
			B = tempRoom;
		}

		// New:
		// aLocation _________     bLocation ______
		//          |    A    |             |   B  |
		//          |         |             |______|
		//          |_________|
	}

	// If they share a border, we don't need to do any processing at all
	if (bLocation.X == aLocation.X + A->XSize())
	{
		// Share border along Y axis
		//  _________ ______
		// |    A    |   B  |
		// |         |______|
		// |_________|

		// Ensure that there is actually overlap between their edges
		// A is guaranteed to be above or on the same level as B
		if (bLocation.Y > aLocation.Y + A->YSize())
		{
			return hallways;
		}
	}
	if (bLocation.Y == aLocation.Y + A->YSize())
	{
		//  _________ 
		// |    A    |
		// |         |
		// |_________|
		// |     |
		// |  B  |
		// |_____|
		if (bLocation.X + B->XSize() > aLocation.X && bLocation.X < aLocation.X + A->XSize())
		{
			return hallways;
		}
	}

	FString hallwayName = A->GetName() + " - " + B->GetName();

	// We can select any point between aLocation.X to bLocation.X + b width (XSize())
	// This int is an X coordinate in world space
	int32 randomLocation = -1;
	// If there's an overlap between rooms, limit it to that overlap
	// This prevents overly-winding hallways
	if (bLocation.X > aLocation.X && bLocation.X < aLocation.X + A->XSize())
	{
		// B starts somewhere between start of A and end of A
		int32 endRange = FMath::Min(bLocation.X + B->XSize(), aLocation.X + A->XSize());
		if (endRange - bLocation.X - (HALLWAY_EDGE_OFFSET * 2) >= HALLWAY_WIDTH)
		{
			randomLocation = Rng.RandRange(bLocation.X + HALLWAY_EDGE_OFFSET, endRange - HALLWAY_EDGE_OFFSET - 1);
		}
	}
	else if (aLocation.X > bLocation.X && aLocation.X < bLocation.X + B->XSize())
	{
		// A starts somewhere between start of B and end of B
		int32 endRange = FMath::Min(bLocation.X + B->XSize(), aLocation.X + A->XSize());
		if (endRange - bLocation.X - (HALLWAY_EDGE_OFFSET * 2) >= HALLWAY_WIDTH)
		{
			randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, endRange - HALLWAY_EDGE_OFFSET - 1);
		}
	}
	if(randomLocation == -1)
	{
		randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, bLocation.X + B->XSize() - HALLWAY_EDGE_OFFSET - 1);
	}
	// Edge cases:
	// Point not in either room
	// aLocation ______
	//          |  A   |
	//          |______|
	//                     ^
	//                     Point
	//
	//                    bLocation _________
	//                             |    B    |
	//                             |_________|
	if (randomLocation > aLocation.X + A->XSize() && randomLocation < bLocation.X ||
		randomLocation > bLocation.X + B->XSize() && randomLocation < aLocation.X)
	{
		// Limit it to be somewhere along only room A
		randomLocation = Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, aLocation.X + A->XSize() - HALLWAY_EDGE_OFFSET - 1);
		// New:
		// aLocation ______
		//          |  A   |
		//          |______|
		//              ^
		//              Point
		//
		//                    bLocation _________
		//                             |    B    |
		//                             |_________|
	}

	// Edge case:
	// Rooms aligned on Y axis (no top or bottom)
	// aLocation ______     bLocation _________
	//          |  A   |             |    B    |
	//          |______|             |_________|
	//           ^
	//           Point
	if (aLocation.Y == bLocation.Y)
	{
		hallwayName.Append(" (Y Coordinates are the Same)");
		// A is guaranteed to be the same size or smaller on the Y axis
		randomLocation = Rng.RandRange(aLocation.Y + HALLWAY_EDGE_OFFSET, aLocation.Y + A->YSize() - HALLWAY_EDGE_OFFSET - 1);

		// aLocation ______                   bLocation _________
		//          |  A   |<-Point                    |    B    |
		//          |______|                           |_________|
		FIntVector hallwayStart; 
		FIntVector hallwayEnd;
		if (aLocation.X < bLocation.X)
		{
			// A is to the left of B
			hallwayStart = FIntVector(aLocation.X + A->XSize(), randomLocation, aLocation.Z);
			hallwayEnd = FIntVector(bLocation.X, randomLocation, bLocation.Z);
		}
		else
		{
			// B is to the left of A
			hallwayStart = FIntVector(bLocation.X + B->XSize(), randomLocation, aLocation.Z);
			hallwayEnd = FIntVector(aLocation.X, randomLocation, bLocation.Z);
		}
		// aLocation ______                   bLocation _________
		//          |  A   |<-hallwayStart hallwayEnd->|    B    |
		//          |______|                           |_________|
		
		// Goal:
		//           ______                             _________
		//          |  A   |___________________________|    B    |
		//          |______|                           |_________|

		//ADungeonRoom* hallway = NewObject<ADungeonRoom>(A->GetOwner(), FName(*hallwayName));
		ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
		hallway->Rename(*hallwayName);
		hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
			hallwayStart, hallwayEnd, HALLWAY_WIDTH);
		//hallway->InitializeRoom(
		//	DefaultTile, hallwayEnd.X - hallwayStart.X, HALLWAY_WIDTH, 
		//	hallwayStart.X, hallwayStart.Y, hallwayStart.Z, 
		//	HallwaySymbol, Rng, false);
		hallways.Add(hallway);

		if (A->MissionNeighbors.Contains(B))
		{
			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
		else
		{
			B->MissionNeighbors.Remove(A);
			B->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(A);
		}
	}
	else
	{
		// There are now 3 different possibilities:
		// A (Point only touching room A):
		// aLocation ______
		//          |  A   |
		//          |______|
		//           ^
		//           Point
		//
		//    bLocation _________
		//             |    B    |
		//             |_________|
		//
		// B (Point in the intersection of both rooms:
		// aLocation ______
		//          |  A   |
		//          |______|
		//               ^
		//               Point
		//
		//    bLocation _________
		//             |    B    |
		//             |_________|
		//
		// C (Point only touching room B):
		// aLocation ______
		//          |  A   |
		//          |______|
		//           
		//                   Point
		//                   v
		//    bLocation _________
		//             |    B    |
		//             |_________|

		// Solutions are as follows:
		// A:
		// aLocation ______
		//          |  A   |
		//          |______|
		//           |
		//           |
		//           |
		//    bLocation _________
		//           |_|    B    |
		//             |_________|
		//
		// B:
		// aLocation ______
		//          |  A   |
		//          |______|
		//               |
		//               |
		//               |
		//    bLocation _|_______
		//             |    B    |
		//             |_________|
		//
		// C:
		// aLocation ______
		//          |  A   |_
		//          |______| |
		//                   |
		//                   |
		//                   |
		//    bLocation _____|___
		//             |    B    |
		//             |_________|

		if (randomLocation < bLocation.X || 
			randomLocation > aLocation.X && randomLocation > bLocation.X + B->XSize())
		{
			// Case A (Point only touching room A)
			// aLocation ______
			//          |  A   |
			//          |______|
			//           ^
			//           Point
			//
			//    bLocation _________
			//             |    B    |
			//             |_________|
			FString hallwayAName = hallwayName + " (Case A) A";
			FString hallwayBName = hallwayName + " (Case A) B";

			// Select a random location along the Y direction of room B
			//    bLocation _________
			//   Point B ->|    B    |
			//             |_________|
			int32 pointB = Rng.RandRange(bLocation.Y - HALLWAY_EDGE_OFFSET, bLocation.Y + B->YSize() - HALLWAY_EDGE_OFFSET - 1);
			// Define points
			FIntVector aHallwayStart = FIntVector(randomLocation, aLocation.Y + A->YSize(), aLocation.Z);
			FIntVector hallwayIntersection = FIntVector(randomLocation, pointB, bLocation.Z);

			// Create hallway A
			//ADungeonRoom* hallwayA = NewObject<ADungeonRoom>(A->GetOwner(), FName(*hallwayAName));
			ADungeonRoom* hallwayA = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayA->Rename(*hallwayAName);

			hallwayA->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, aHallwayStart, hallwayIntersection, HALLWAY_WIDTH);
			hallways.Add(hallwayA);

			FIntVector bHallwayStart;

			if (aLocation.X > bLocation.X)
			{
				// Edge case: B is to the left of A
				//                            aLocation ______
				//                                     |  A   |
				//                                     |______|
				//                                aHallway->|
				//                                          |
				//                                          |
				//        bLocation _________               |
				//                 |    B    |______________|<-intersection
				//                 |_________|^ Point B
				hallwayBName += " Edge Case";
				bHallwayStart = FIntVector(bLocation.X + B->XSize(), pointB, bLocation.Z);
			}
			else
			{
				//     aLocation ______
				//              |  A   |
				//              |______|
				//     aHallway->|
				//               |
				//               |
				//        bLocation _________
				// intersection->|_|    B    |
				//                ^|_________|
				//         bHallway

				bHallwayStart = FIntVector(bLocation.X, pointB, bLocation.Z);
			}

			ADungeonRoom* hallwayB = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayB->Rename(*hallwayBName);
			hallwayB->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, bHallwayStart, hallwayIntersection, HALLWAY_WIDTH);
			hallways.Add(hallwayB);

			// Fix the mission neighbors array
			if (A->MissionNeighbors.Contains(B))
			{
				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				B->MissionNeighbors.Remove(A);
				B->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(A);
			}
		}
		else if (	randomLocation > bLocation.X && randomLocation < bLocation.X + B->XSize() &&
					randomLocation < aLocation.X + A->XSize())
		{
			// Case B (Point intersects both rooms)
			// aLocation ______
			//          |  A   |
			//          |______|
			//               ^
			//               Point
			//
			//    bLocation _________
			//             |    B    |
			//             |_________|
			hallwayName.Append(" (Case B)");

			FIntVector hallwayStart = FIntVector(randomLocation, aLocation.Y + A->YSize(), aLocation.Z);
			FIntVector hallwayEnd = FIntVector(randomLocation, bLocation.Y, bLocation.Z);

			// aLocation ______
			//          |  A   |
			//          |______|
			//               |
			//               |
			//               |
			//    bLocation _|_______
			//             |    B    |
			//             |_________|

			ADungeonRoom* hallway = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallway->Rename(*hallwayName);
			//hallway->InitializeRoom(
			//	DefaultTile, hallwayEnd.X - hallwayStart.X, HALLWAY_WIDTH,
			//	hallwayStart.X, hallwayStart.Y, hallwayStart.Z,
			//	HallwaySymbol, Rng, false);
			hallway->InitializeRoomFromPoints(DefaultTile, HallwaySymbol,
				hallwayStart, hallwayEnd, HALLWAY_WIDTH);
			hallways.Add(hallway);

			// Fix the mission neighbors
			if (A->MissionNeighbors.Contains(B))
			{
				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallway);
				hallway->MissionNeighbors.Add(B);
			}
			else
			{
				B->MissionNeighbors.Remove(A);
				B->MissionNeighbors.Add(hallway);
				hallway->MissionNeighbors.Add(A);
			}
		}
		else
		{
			// Case C (Point only touching room B)
			// aLocation ______
			//          |  A   |
			//          |______|
			//           
			//                   
			//                   Point
			//    bLocation _____v___
			//             |    B    |
			//             |_________|
			FString hallwayAName = hallwayName + " (Case C) A";
			FString hallwayBName = hallwayName + " (Case C) B";

			// Select a random position along the Y direction of room A
			// aLocation ______
			//          |  A   |<-Point A
			//          |______|
			int32 pointA = Rng.RandRange(aLocation.Y - HALLWAY_EDGE_OFFSET, aLocation.Y + A->YSize() - HALLWAY_EDGE_OFFSET - 1);

			// Create a vector pointing to the top of room B, at the random location we selected
			FIntVector bHallwayStart = FIntVector(randomLocation, bLocation.Y, bLocation.Z);
			FIntVector intersection = FIntVector(randomLocation, pointA, aLocation.Z);

			// Create hallway A
			ADungeonRoom* hallwayB = (ADungeonRoom*)B->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayB->Rename(*hallwayBName);
			hallwayB->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, intersection, bHallwayStart, HALLWAY_WIDTH);
			hallways.Add(hallwayB);

			FIntVector aHallwayStart;
			if (aLocation.X < bLocation.X)
			{
				// Edge case: B is to the left of A
				//                            aLocation ______
				//                            Point A->|  A   |
				//                                     |______|
				//
				//
				//                      Point
				//        bLocation ____v____
				//                 |    B    |
				//                 |_________|
				aHallwayStart = FIntVector(aLocation.X, pointA, aLocation.Z);
			}
			else
			{
				// aLocation ______
				//          |  A   |_
				//          |______| |
				//                   |
				//                   |
				//                   |
				//    bLocation _____|___
				//             |    B    |
				//             |_________|

				aHallwayStart = FIntVector(aLocation.X + A->XSize(), pointA, bLocation.Z);
			}
			
			ADungeonRoom* hallwayA = (ADungeonRoom*)A->GetWorld()->SpawnActor(HallwaySymbol->GetRoomType(Rng));
			hallwayA->Rename(*hallwayAName);
			
			hallwayA->InitializeRoomFromPoints(DefaultTile, HallwaySymbol, aHallwayStart, intersection, HALLWAY_WIDTH);
			hallways.Add(hallwayA);

			// Fix the mission neighbors array
			if (A->MissionNeighbors.Contains(B))
			{
				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				B->MissionNeighbors.Remove(A);
				B->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(A);
			}
		}
	}

	// Randomly generate the spawn position for each hallway, in grid coordinates
	// Each hallway will have:
	// * X -> A location somewhere on the width of the hallway, padded by HALLWAY_EDGE_OFFSET
	// * Y -> A location somewhere on the height of the hallway, padded by HALLWAY_EDGE_OFFSET
	/*FIntVector aHallwayLocation = FIntVector(
		Rng.RandRange(aLocation.X + HALLWAY_EDGE_OFFSET, aLocation.X + A->XSize() - HALLWAY_EDGE_OFFSET - 1),
		Rng.RandRange(aLocation.Y + HALLWAY_EDGE_OFFSET, aLocation.Y + A->YSize() - HALLWAY_EDGE_OFFSET - 1),
		Rng.RandRange(aLocation.Z + HALLWAY_EDGE_OFFSET, aLocation.Z + A->ZSize() - HALLWAY_EDGE_OFFSET - 1));
	FIntVector bHallwayLocation = FIntVector(
		Rng.RandRange(bLocation.X + HALLWAY_EDGE_OFFSET, bLocation.X + B->XSize() - HALLWAY_EDGE_OFFSET - 1),
		Rng.RandRange(bLocation.Y + HALLWAY_EDGE_OFFSET, bLocation.Y + B->YSize() - HALLWAY_EDGE_OFFSET - 1),
		Rng.RandRange(bLocation.Z + HALLWAY_EDGE_OFFSET, bLocation.Z + B->ZSize() - HALLWAY_EDGE_OFFSET - 1));


	float width = bHallwayLocation.X - aHallwayLocation.X;
	float height = bHallwayLocation.Y - aHallwayLocation.Y;

	FString hallwayName = A->GetName() + " - " + B->GetName();
	if (width < 0)
	{
		if (height < 0)
		{
			FString hallway1Name = hallwayName;
			FString hallway2Name = hallwayName;

			hallway1Name.AppendInt(1);
			hallway2Name.AppendInt(2);
			if (Rng.GetFraction() < 0.5f)
			{

				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, bHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), aHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
		}
		else if(height > 0)
		{
			FString hallway1Name = hallwayName;
			FString hallway2Name = hallwayName;

			hallway1Name.AppendInt(1);
			hallway2Name.AppendInt(2);
			if (Rng.GetFraction() < 0.5f)
			{

				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, bHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), bHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
		}
		else // height == 0
		{
			UDungeonRoom* hallway = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallwayName));
			hallway->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
			hallways.Add(hallway);

			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
	}
	else if (width > 0)
	{
		if (height < 0)
		{
			FString hallway1Name = hallwayName;
			FString hallway2Name = hallwayName;

			hallway1Name.AppendInt(1);
			hallway2Name.AppendInt(2);
			if (Rng.GetFraction() < 0.5f)
			{

				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, aHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), aHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
		}
		else if (height > 0)
		{
			FString hallway1Name = hallwayName;
			FString hallway2Name = hallwayName;

			hallway1Name.AppendInt(1);
			hallway2Name.AppendInt(2);
			if (Rng.GetFraction() < 0.5f)
			{

				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), bHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
			else
			{
				UDungeonRoom* hallwayA = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway1Name));
				hallwayA->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, aHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
				UDungeonRoom* hallwayB = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallway2Name));
				hallwayB->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);

				hallways.Add(hallwayA);
				hallways.Add(hallwayB);

				A->MissionNeighbors.Remove(B);
				A->MissionNeighbors.Add(hallwayA);
				hallwayA->MissionNeighbors.Add(hallwayB);
				hallwayB->MissionNeighbors.Add(B);
			}
		}
		else // height == 0
		{
			UDungeonRoom* hallway = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallwayName));
			hallway->InitializeRoom(DefaultTile, FMath::Abs(width), HALLWAY_WIDTH, aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
			hallways.Add(hallway);

			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
	}
	else // width == 0
	{
		if (height < 0)
		{
			UDungeonRoom* hallway = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallwayName));
			hallway->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), bHallwayLocation.X, bHallwayLocation.Y, 0, HallwaySymbol, Rng);
			hallways.Add(hallway);

			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
		else if (height > 0)
		{
			UDungeonRoom* hallway = NewObject<UDungeonRoom>(A->GetOwner(), FName(*hallwayName));
			hallway->InitializeRoom(DefaultTile, HALLWAY_WIDTH, FMath::Abs(height), aHallwayLocation.X, aHallwayLocation.Y, 0, HallwaySymbol, Rng);
			hallways.Add(hallway);

			A->MissionNeighbors.Remove(B);
			A->MissionNeighbors.Add(hallway);
			hallway->MissionNeighbors.Add(B);
		}
	}*/

	return hallways;
}

void ADungeonRoom::SetTileGridCoordinates(FIntVector CurrentLocation, const UDungeonTile* Tile)
{

	FVector position = GetActorLocation();
	int32 xPosition = FMath::RoundToInt(position.X / UDungeonTile::TILE_SIZE);
	int32 yPosition = FMath::RoundToInt(position.Y / UDungeonTile::TILE_SIZE);
	Set(CurrentLocation.X - xPosition, CurrentLocation.Y - yPosition, Tile);
}
