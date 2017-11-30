// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DungeonMakerAssetTypeActions.h"
#include "DungeonMakerAssetEditor/DungeonMakerEdNode.h"
#include "DungeonMakerAssetEditor/SDungeonMakerEdNode.h"

DEFINE_LOG_CATEGORY(DungeonMakerEditor)

#define LOCTEXT_NAMESPACE "DungeonMakerEditor"

class FGraphPanelNodeFactory_DungeonMaker : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		if (UDungeonMakerEdNode* GraphEdNode = Cast<UDungeonMakerEdNode>(Node))
		{
			return SNew(SDungeonMakerEdNode, GraphEdNode);
		}
		return nullptr;
	}
};

TSharedPtr<FGraphPanelNodeFactory> GraphPanelNodeFactory_DungeonMaker;

class FDungeonMakerEditor : public IDungeonMakerEditor
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

private:
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	EAssetTypeCategories::Type DungeonMakerAssetCategoryBit;
};

IMPLEMENT_MODULE( FDungeonMakerEditor, UDungeonMakerEditor )

void FDungeonMakerEditor::StartupModule()
{
	GraphPanelNodeFactory_DungeonMaker = MakeShareable(new FGraphPanelNodeFactory_DungeonMaker());
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_DungeonMaker);

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	DungeonMakerAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("DungeonMaker")), LOCTEXT("DungeonMakerAssetCategory", "DungeonMaker"));

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FDungeonMakerAssetTypeActions(DungeonMakerAssetCategoryBit)));
}


void FDungeonMakerEditor::ShutdownModule()
{
	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}

	if (GraphPanelNodeFactory_DungeonMaker.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_DungeonMaker);
		GraphPanelNodeFactory_DungeonMaker.Reset();
	}
}

void FDungeonMakerEditor::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

