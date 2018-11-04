#include "TrialRoomBase.h"
#include "TrapTrigger.h"
#include "Trap.h"
#include "DungeonSpaceGenerator.h"
#include "RoomTileComponent.h"

void ATrialRoomBase::OnBeginTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ADungeonRoom::OnBeginTriggerOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	if (bIncludeOnPlayerEnterAsTrigger)
	{
		for (int i = 0; i < TrapList.Num(); i++)
		{
			if (TrapList[i]->GetClass()->ImplementsInterface(UTrap::StaticClass()))
			{
				ITrap::Execute_ActivateTrap(TrapList[i]);
			}
		}
	}
}

void ATrialRoomBase::DoTileReplacementPreprocessing(FRandomStream& Rng)
{
	ADungeonRoom::DoTileReplacementPreprocessing(Rng);

	TArray<FRoomReplacements> trapTileReplacements;

	if (TriggerTileReplacements.Num() > 0)
	{
		if (bIncludeOnPlayerEnterAsTrigger)
		{
			int32 triggerIndex = Rng.RandRange(0, TriggerTileReplacements.Num());
			bIncludeOnPlayerEnterAsTrigger = triggerIndex == TriggerTileReplacements.Num();
		}
		if(!bIncludeOnPlayerEnterAsTrigger)
		{
			int32 triggerIndex = Rng.RandRange(0, TriggerTileReplacements.Num() - 1);
			FRoomReplacements triggers = TriggerTileReplacements[triggerIndex];
			trapTileReplacements.Add(triggers);
		}
	}
	if (TrapTileReplacements.Num() > 0)
	{
		int32 trapIndex = Rng.RandRange(0, TrapTileReplacements.Num() - 1);
		FRoomReplacements traps = TrapTileReplacements[trapIndex];
		trapTileReplacements.Add(traps);
	}

	RoomTiles->AddReplacementPhases(trapTileReplacements);
}

TArray<AActor*> ATrialRoomBase::CreateTriggers_Implementation(FRandomStream Rng)
{
	TArray<AActor*> triggers;
	for (auto& kvp : TileTriggerObjects)
	{
		TSet<FIntVector> tileLocations = DungeonSpace->DungeonSpace.GetTileLocations(kvp.Key);
		for (FIntVector tileLocation : tileLocations)
		{
			AActor* triggerActor = GroundScatter->SpawnScatterActor(this, tileLocation, kvp.Value, Rng);
			if (triggerActor == NULL)
			{
				continue;
			}

			if (triggerActor->GetClass()->ImplementsInterface(UTrapTrigger::StaticClass()))
			{
				ITrapTrigger::Execute_OnTriggerPlaced(triggerActor, this);
			}
			triggers.Add(triggerActor);
		}
	}
	TriggerList = triggers;
	UE_LOG(LogSpaceGen, Log, TEXT("%s (%s) created %d triggers."), *GetName(), *GetClass()->GetName(), triggers.Num());
	return triggers;
}

TArray<AActor*> ATrialRoomBase::CreateTraps_Implementation(FRandomStream Rng)
{
	TArray<AActor*> traps;
	for (auto& kvp : TileTrapObjects)
	{
		TSet<FIntVector> tileLocations = DungeonSpace->DungeonSpace.GetTileLocations(kvp.Key);
		for (FIntVector tileLocation : tileLocations)
		{
			AActor* trapActor = GroundScatter->SpawnScatterActor(this, tileLocation, kvp.Value, Rng);
			if (trapActor == NULL)
			{
				continue;
			}

			if (trapActor->GetClass()->ImplementsInterface(UTrap::StaticClass()))
			{
				ITrap::Execute_InitializeTrap(trapActor, this);
			}
			traps.Add(trapActor);
			for (int j = 0; j < TriggerList.Num(); j++)
			{
				if (TriggerList[j]->GetClass()->ImplementsInterface(UTrapTrigger::StaticClass()))
				{
					ITrapTrigger::Execute_AddAssociatedTrap(TriggerList[j], trapActor);
				}
			}
		}
	}
	TrapList = traps;
	UE_LOG(LogSpaceGen, Log, TEXT("%s (%s) created %d traps."), *GetName(), *GetClass()->GetName(), traps.Num());
	return traps;
}