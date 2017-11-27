#include "TrialRoomBase.h"
#include "TrapTrigger.h"
#include "Trap.h"

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

	RoomReplacementPhases.Insert(trapTileReplacements, 0);
}

TArray<AActor*> ATrialRoomBase::CreateTraps_Implementation(FRandomStream Rng)
{
	TArray<AActor*> traps;
	for (auto& kvp : TileTriggerObjects)
	{
		TArray<FIntVector> tileLocations = GetTileLocations(kvp.Key);
		for (int i = 0; i < tileLocations.Num(); i++)
		{
			AActor* triggerActor = GroundScatter->SpawnScatterActor(this, tileLocations[i], kvp.Value, Rng);
			if (triggerActor == NULL)
			{
				continue;
			}

			if (triggerActor->GetClass()->ImplementsInterface(UTrapTrigger::StaticClass()))
			{
				ITrapTrigger::Execute_OnTriggerPlaced(triggerActor, this);
			}
			traps.Add(triggerActor);
		}
	}

	for (auto& kvp : TileTrapObjects)
	{
		TArray<FIntVector> tileLocations = GetTileLocations(kvp.Key);
		for (int i = 0; i < tileLocations.Num(); i++)
		{
			AActor* trapActor = GroundScatter->SpawnScatterActor(this, tileLocations[i], kvp.Value, Rng);
			if (trapActor == NULL)
			{
				continue;
			}

			if (trapActor->GetClass()->ImplementsInterface(UTrap::StaticClass()))
			{
				ITrap::Execute_InitializeTrap(trapActor, this);
			}
			traps.Add(trapActor);
		}
	}
	TrapList = traps;
	return traps;
}
