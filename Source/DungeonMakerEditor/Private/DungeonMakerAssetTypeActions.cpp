#include "DungeonMakerAssetTypeActions.h"
#include "DungeonMakerAssetEditor/DungeonMakerAssetEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FDungeonMakerAssetTypeActions::FDungeonMakerAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FDungeonMakerAssetTypeActions::GetName() const
{
	return LOCTEXT("FDungeonMakerAssetTypeActionsName", "Generic Graph");
}

FColor FDungeonMakerAssetTypeActions::GetTypeColor() const
{
	return FColor(129, 196, 115);
}

UClass* FDungeonMakerAssetTypeActions::GetSupportedClass() const
{
	return UDungeonMakerGraph::StaticClass();
}

void FDungeonMakerAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UDungeonMakerGraph* Graph = Cast<UDungeonMakerGraph>(*ObjIt))
		{
			TSharedRef<FDungeonMakerAssetEditor> NewGraphEditor(new FDungeonMakerAssetEditor());
			NewGraphEditor->InitDungeonMakerAssetEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}

uint32 FDungeonMakerAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Animation | MyAssetCategory;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE