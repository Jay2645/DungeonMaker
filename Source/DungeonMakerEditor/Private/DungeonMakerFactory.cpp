#include "DungeonMakerFactory.h"

#define LOCTEXT_NAMESPACE "DungeonMaker"

UDungeonMakerFactory::UDungeonMakerFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UDungeonMakerGraph::StaticClass();
}

UDungeonMakerFactory::~UDungeonMakerFactory()
{

}

UObject* UDungeonMakerFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}

#undef LOCTEXT_NAMESPACE
