// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMissionGen, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSpaceGen, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStateMachine, Log, All);

class FDungeonMakerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};