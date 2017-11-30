#include "DungeonMakerEditorCommands.h"

#define LOCTEXT_NAMESPACE "DungeonMakerEditorCommands"

void FDungeonMakerEditorCommands::RegisterCommands()
{
	UI_COMMAND(GraphSettings, "Graph Settings", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
