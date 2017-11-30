#pragma once

#include "CoreMinimal.h"

class FDungeonMakerEditorCommands : public TCommands<FDungeonMakerEditorCommands>
{
public:
	/** Constructor */
	FDungeonMakerEditorCommands()
		: TCommands<FDungeonMakerEditorCommands>("DungeonMakerEditor", NSLOCTEXT("Contexts", "DungeonMakerEditor", "Generic Graph Editor"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	TSharedPtr<FUICommandInfo> GraphSettings;

	virtual void RegisterCommands() override;
};
