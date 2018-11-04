// Fill out your copyright notice in the Description page of Project Settings.

#include "DungeonRoom.h"
#include "DungeonSpaceGenerator.h"
#include "DungeonFloorManager.h"
#include "DungeonTile.h"
#include "GroundScatterManager.h"
#include <DrawDebugHelpers.h>
#include "GameFramework/Character.h"
#include "Trials/TrialRoom.h"
#include "LockedRoom.h"
#include "KeyRoom.h"
#include "Components/RoomTileComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/RoomMeshComponent.h"

// Sets default values for this component's properties
ADungeonRoom::ADungeonRoom()
{
	Symbol = NULL;
	DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	GroundScatter = CreateDefaultSubobject<UGroundScatterManager>(TEXT("Ground Scatter"));
	RoomMeshes = CreateDefaultSubobject<URoomMeshComponent>(TEXT("Room Mesh Manager"));
	RoomTiles = CreateDefaultSubobject<URoomTileComponent>(TEXT("Room Tile Manager"));

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Room Trigger"));
	RoomTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	NorthEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("North Entrance Trigger"));
	NorthEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	SouthEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("South Entrance Trigger"));
	SouthEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	WestEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("West Entrance Trigger"));
	WestEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	EastEntranceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("East Entrance Trigger"));
	EastEntranceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FCollisionResponseContainer collisonChannels;
	collisonChannels.SetAllChannels(ECR_Ignore);
	collisonChannels.SetResponse(ECollisionChannel::ECC_Pawn, ECR_Overlap);

	RoomTrigger->SetCollisionResponseToChannels(collisonChannels);
	RoomTrigger->SetupAttachment(DummyRoot);
	RoomTrigger->SetGenerateOverlapEvents(true);
	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginTriggerOverlap);
	RoomTrigger->OnComponentEndOverlap.AddDynamic(this, &ADungeonRoom::OnEndTriggerOverlap);

	NorthEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	NorthEntranceTrigger->SetupAttachment(DummyRoot);
	NorthEntranceTrigger->SetGenerateOverlapEvents(true);
	NorthEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	SouthEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	SouthEntranceTrigger->SetupAttachment(DummyRoot);
	SouthEntranceTrigger->SetGenerateOverlapEvents(true);
	SouthEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	WestEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	WestEntranceTrigger->SetupAttachment(DummyRoot);
	WestEntranceTrigger->SetGenerateOverlapEvents(true);
	WestEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	EastEntranceTrigger->SetCollisionResponseToChannels(collisonChannels);
	EastEntranceTrigger->SetupAttachment(DummyRoot);
	EastEntranceTrigger->SetGenerateOverlapEvents(true);
	EastEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::OnBeginEntranceOverlap);

	HallwayRoomClass = ADungeonRoom::StaticClass();

	DebugRoomMaxExtents = FIntVector(16, 16, 1);
	RoomDifficultyModifier = 1.0f;

	bIsStandaloneRoomForDebug = false;
	bRoomShouldBeRandomlySized = false;
	bSpawnInterfaces = true;
}

void ADungeonRoom::BeginPlay()
{
	if (bIsStandaloneRoomForDebug)
	{
		// @TODO
	}
}

void ADungeonRoom::OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerEnterRoom();
				for (ADungeonRoom* neighbor : AllNeighbors)
				{
					neighbor->OnPlayerEnterNeighborRoom();
				}
			}
		}
	}
}

void ADungeonRoom::OnEndTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerLeaveRoom();
			}
		}
	}
}

void ADungeonRoom::OnBeginEntranceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->IsA(ACharacter::StaticClass()))
	{
		ACharacter* otherCharacter = (ACharacter*)OtherActor;
		if (otherCharacter->GetController() != NULL)
		{
			if (otherCharacter->GetController()->IsA(APlayerController::StaticClass()))
			{
				// This controller belongs to the player
				OnPlayerEnterRoomEntrance();
			}
		}
	}
}

void ADungeonRoom::InitializeRoom(UDungeonSpaceGenerator* SpaceGenerator, UDungeonFloorManager* FloorManager,
	const UDungeonTile* DefaultFloorTile, const UDungeonTile* DefaultWallTile, const UDungeonTile* DefaultEntranceTile,
	const UDungeonTile* DefaultExitTile, const FIntVector& RoomDimensions, const FIntVector& RoomPosition,
	FFloorRoom RoomData, FRandomStream &Rng, bool bIsDeterminedFromPoints)
{
	const float TILE_SIZE = UDungeonTile::TILE_SIZE;
	const float HALF_TILE_SIZE = TILE_SIZE * 0.5f;

	DungeonSpace = SpaceGenerator;
	DungeonFloor = FloorManager;
	RoomMetadata = RoomData;
	DebugSeed = Rng.GetCurrentSeed();
	Symbol = (const UDungeonMissionSymbol*)RoomData.DungeonSymbol.Symbol;

	RoomTiles->InitializeTileComponent(this, RoomDimensions, RoomPosition, DefaultFloorTile, DefaultWallTile, DefaultEntranceTile, DefaultExitTile, Rng, bRoomShouldBeRandomlySized);
	RoomMeshes->InitializeMeshComponent(this, GroundScatter, SpaceGenerator);

	FVector halfExtents = FVector((RoomDimensions.X * HALF_TILE_SIZE), (RoomDimensions.Y * HALF_TILE_SIZE), TILE_SIZE);
	RoomTrigger->SetRelativeLocation(halfExtents - FVector(0.0f, TILE_SIZE, TILE_SIZE));
	RoomTrigger->SetBoxExtent(halfExtents - FVector(TILE_SIZE, TILE_SIZE, 0.0f));

	NorthEntranceTrigger->SetRelativeLocation(FVector(RoomDimensions.X * HALF_TILE_SIZE, (RoomDimensions.Y - 1) * TILE_SIZE - HALF_TILE_SIZE, TILE_SIZE));
	NorthEntranceTrigger->SetBoxExtent(FVector(RoomDimensions.X * HALF_TILE_SIZE, HALF_TILE_SIZE, TILE_SIZE));

	SouthEntranceTrigger->SetRelativeLocation(FVector(RoomDimensions.X * HALF_TILE_SIZE, -HALF_TILE_SIZE, TILE_SIZE));
	SouthEntranceTrigger->SetBoxExtent(FVector(RoomDimensions.X * HALF_TILE_SIZE, HALF_TILE_SIZE, TILE_SIZE));

	EastEntranceTrigger->SetRelativeLocation(FVector(HALF_TILE_SIZE, (RoomDimensions.Y - 1) * HALF_TILE_SIZE - HALF_TILE_SIZE, TILE_SIZE));
	EastEntranceTrigger->SetBoxExtent(FVector(HALF_TILE_SIZE, RoomDimensions.Y * HALF_TILE_SIZE, TILE_SIZE));

	WestEntranceTrigger->SetRelativeLocation(FVector(RoomDimensions.X * TILE_SIZE - HALF_TILE_SIZE, (RoomDimensions.Y - 1) * HALF_TILE_SIZE - HALF_TILE_SIZE, TILE_SIZE));
	WestEntranceTrigger->SetBoxExtent(FVector(HALF_TILE_SIZE, RoomDimensions.Y * HALF_TILE_SIZE, TILE_SIZE));

	OnRoomInitialized();

	UE_LOG(LogSpaceGen, Verbose, TEXT("Initialized %s to dimensions %d x %d."), *GetName(), DebugRoomMaxExtents.X, DebugRoomMaxExtents.Y);
}

void ADungeonRoom::DoTileReplacement(FRandomStream &Rng)
{
	OnPreRoomTilesReplaced();
	DoTileReplacementPreprocessing(Rng);

	RoomTiles->DoTileReplacement(Rng);

	OnRoomTilesReplaced();

#if !UE_BUILD_SHIPPING
	if (bSpawnInterfaces)
	{
#endif
		SpawnInterfaces(Rng);
#if !UE_BUILD_SHIPPING
	}
#endif
}

FIntVector ADungeonRoom::GetRoomLocation() const
{
	return RoomTiles->RoomLocation;
}

FIntVector ADungeonRoom::GetRoomSize() const
{
	return RoomTiles->RoomSize;
}

FDungeonSpace& ADungeonRoom::GetDungeon() const
{
	return DungeonSpace->DungeonSpace;
}

bool ADungeonRoom::IsChangedAtRuntime() const
{
	if (Symbol == NULL)
	{
		return true;
	}
	return Symbol->bChangedAtRuntime;
}

float ADungeonRoom::GetRoomDifficulty() const
{
	return RoomMetadata.Difficulty * RoomDifficultyModifier;
}

void ADungeonRoom::DoTileReplacementPreprocessing(FRandomStream& Rng)
{
	UE_LOG(LogSpaceGen, Log, TEXT("Doing pre-processing of %s (%s) with RNG seed %d."), *GetName(), *GetClass()->GetName(), Rng.GetInitialSeed());
}

void ADungeonRoom::SpawnInterfaces(FRandomStream &Rng)
{
	// Place traps
	if (GetClass()->ImplementsInterface(UTrialRoom::StaticClass()))
	{
		UE_LOG(LogSpaceGen, Log, TEXT("Creating triggers!"));
		FRandomStream nextRng = FRandomStream(Rng.RandRange(MIN_int32, MAX_int32));
		TArray<AActor*> spawnedTriggers = ITrialRoom::Execute_CreateTriggers(this, nextRng);
#if WITH_EDITOR
		for (AActor* trigger : spawnedTriggers)
		{
			FString folderPath = "Rooms/Triggers/";
			folderPath.Append(trigger->GetClass()->GetName());
			trigger->SetFolderPath(FName(*folderPath));
		}
#endif

		UE_LOG(LogSpaceGen, Log, TEXT("Creating traps!"));
		nextRng = FRandomStream(Rng.RandRange(MIN_int32, MAX_int32));
		TArray<AActor*> spawnedTraps = ITrialRoom::Execute_CreateTraps(this, nextRng);
#if WITH_EDITOR
		for (AActor* trap : spawnedTraps)
		{
			FString folderPath = "Rooms/Traps/";
			folderPath.Append(trap->GetClass()->GetName());
			trap->SetFolderPath(FName(*folderPath));
		}
#endif
	}

	if (GetClass()->ImplementsInterface(ULockedRoom::StaticClass()))
	{
		UE_LOG(LogSpaceGen, Log, TEXT("Spawning lock!"));
		FRandomStream nextRng = FRandomStream(Rng.RandRange(MIN_int32, MAX_int32));
		AActor* lock = ILockedRoom::Execute_SpawnLock(this, nextRng);
		if (lock == NULL)
		{
			UE_LOG(LogSpaceGen, Error, TEXT("%s did not return a lock actor from its SpawnLock method!"), *GetName());
		}
#if WITH_EDITOR
		else
		{
			FString folderPath = "Rooms/Locks/";
			folderPath.Append(lock->GetClass()->GetName());
			lock->SetFolderPath(FName(*folderPath));
		}
#endif
	}

	if (GetClass()->ImplementsInterface(UKeyRoom::StaticClass()))
	{
		FRandomStream nextRng = FRandomStream(Rng.RandRange(MIN_int32, MAX_int32));
		UE_LOG(LogSpaceGen, Log, TEXT("Spawning key!"));
		AActor* key = IKeyRoom::Execute_SpawnKey(this, nextRng);
		if(key == NULL)
		{
			UE_LOG(LogSpaceGen, Error, TEXT("%s did not return a key actor from its SpawnKey method!"), *GetName());
		}
#if WITH_EDITOR
		else
		{
			FString folderPath = "Rooms/Keys/";
			folderPath.Append(key->GetClass()->GetName());
			key->SetFolderPath(FName(*folderPath));
		}
#endif
	}
}

void ADungeonRoom::PlaceNeighbors(FRandomStream& Rng)
{
	FDungeonSpace& dungeon = GetDungeon();

	TSet<FIntVector> neighbors = RoomMetadata.NeighboringRooms;
	for (FIntVector neighbor : neighbors)
	{
		ADungeonRoom* room = dungeon.GetLowRes(neighbor).SpawnedRoom;
		if (room == NULL || AllNeighbors.Contains(room))
		{
			continue;
		}
		RoomTiles->ConnectToRoom(room, HallwayRoomClass, Rng);
		room->AllNeighbors.Add(this);
		AllNeighbors.Add(room);
	}

	// Now process any tightly-coupled neighbors
	neighbors = RoomMetadata.NeighboringTightlyCoupledRooms;
	for (FIntVector neighbor : neighbors)
	{
		ADungeonRoom* room = dungeon.GetLowRes(neighbor).SpawnedRoom;
		if (room == NULL || AllNeighbors.Contains(room))
		{
			continue;
		}
		RoomTiles->ConnectToRoom(room, HallwayRoomClass, Rng);
		room->AllNeighbors.Add(this);
		AllNeighbors.Add(room);
		room->TightlyCoupledNeighbors.Add(this);
		TightlyCoupledNeighbors.Add(room);
	}

	UE_LOG(LogSpaceGen, Log, TEXT("Created entrances to %d rooms."), AllNeighbors.Num());
}

bool ADungeonRoom::IsChildOf(ADungeonRoom* ParentRoom) const
{
	if (ParentRoom == NULL)
	{
		UE_LOG(LogSpaceGen, Error, TEXT("Can't check if %s is a child of a null room!"), *GetName());
		return false;
	}
	return RoomMetadata.RoomNode->IsChildOf(ParentRoom->RoomMetadata.RoomNode);
}
