#include "DungeonMakerAssetEditorToolbar.h"
#include "DungeonMakerAssetEditor.h"
#include "DungeonMakerEditorCommands.h"

#define LOCTEXT_NAMESPACE "DungeonMakerAssetEditorToolbar"

void FDungeonMakerAssetEditorToolbar::AddDungeonMakerToolbar(TSharedPtr<FExtender> Extender)
{
	check(DungeonMakerEditor.IsValid());
	TSharedPtr<FDungeonMakerAssetEditor> DungeonMakerEditorPtr = DungeonMakerEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, DungeonMakerEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FDungeonMakerAssetEditorToolbar::FillDungeonMakerToolbar ));
	DungeonMakerEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FDungeonMakerAssetEditorToolbar::FillDungeonMakerToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(DungeonMakerEditor.IsValid());
	TSharedPtr<FDungeonMakerAssetEditor> DungeonMakerEditorPtr = DungeonMakerEditor.Pin();

	ToolbarBuilder.BeginSection("Generic Graph");
	{

		const FText GraphSettingsLabel = LOCTEXT("GraphSettings_Label", "Graph Settings");
		const FText GraphSettingsTip = LOCTEXT("GraphSettings_ToolTip", "Show the Graph Settings");
		const FSlateIcon GraphSettingsIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.GameSettings");
		ToolbarBuilder.AddToolBarButton(FDungeonMakerEditorCommands::Get().GraphSettings, 
			NAME_None,
			GraphSettingsLabel,
			GraphSettingsTip,
			GraphSettingsIcon);
	}
	ToolbarBuilder.EndSection();

}


#undef LOCTEXT_NAMESPACE
