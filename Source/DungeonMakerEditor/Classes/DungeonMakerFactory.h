#pragma once

#include "DungeonMakerFactory.generated.h"

UCLASS()
class DUNGEONMAKEREDITOR_API UDungeonMakerFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonMakerFactory();
	virtual ~UDungeonMakerFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
