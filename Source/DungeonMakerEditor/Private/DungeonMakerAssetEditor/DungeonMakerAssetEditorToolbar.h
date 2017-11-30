
#pragma once

#include "CoreMinimal.h"

class FDungeonMakerAssetEditor;
class FExtender;
class FToolBarBuilder;

class FDungeonMakerAssetEditorToolbar : public TSharedFromThis<FDungeonMakerAssetEditorToolbar>
{
public:
	FDungeonMakerAssetEditorToolbar(TSharedPtr<FDungeonMakerAssetEditor> InDungeonMakerEditor)
		: DungeonMakerEditor(InDungeonMakerEditor) {}

	//void AddModesToolbar(TSharedPtr<FExtender> Extender);
	//void AddDebuggerToolbar(TSharedPtr<FExtender> Extender);
	void AddDungeonMakerToolbar(TSharedPtr<FExtender> Extender);

private:
	//void FillModesToolbar(FToolBarBuilder& ToolbarBuilder);
	//void FillDebuggerToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillDungeonMakerToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FDungeonMakerAssetEditor> DungeonMakerEditor;
};
