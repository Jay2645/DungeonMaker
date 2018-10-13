#include "DungeonMakerAssetEditor.h"
#include "DungeonMakerEditorPrivatePCH.h"
#include "DungeonMakerAssetEditorToolbar.h"
#include "DungeonMakerAssetGraphSchema.h"
#include "DungeonMakerEditorCommands.h"
#include "DungeonMakerEdGraph.h"
#include "AssetToolsModule.h"
#include "GenericPlatform/GenericPlatformApplicationMisc.h"
#include "GenericCommands.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "EdGraphUtilities.h"

#define LOCTEXT_NAMESPACE "DungeonMakerAssetEditor"

const FName DungeonMakerEditorAppName = FName(TEXT("DungeonMakerEditorApp"));

struct FDungeonMakerAssetEditorTabs
{
	// Tab identifiers
	static const FName DetailsID;
	static const FName ViewportID;
};

//////////////////////////////////////////////////////////////////////////

const FName FDungeonMakerAssetEditorTabs::DetailsID(TEXT("Details"));
const FName FDungeonMakerAssetEditorTabs::ViewportID(TEXT("Viewport"));

//////////////////////////////////////////////////////////////////////////

FDungeonMakerAssetEditor::FDungeonMakerAssetEditor()
{
	EditingGraph = nullptr;

	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FDungeonMakerAssetEditor::OnPackageSaved);
}

FDungeonMakerAssetEditor::~FDungeonMakerAssetEditor()
{
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
}

void FDungeonMakerAssetEditor::InitDungeonMakerAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UDungeonMakerGraph* Graph)
{

	EditingGraph = Graph;
	CreateEdGraph();

	FGenericCommands::Register();
	FGraphEditorCommands::Register();
	FDungeonMakerEditorCommands::Register();

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FDungeonMakerAssetEditorToolbar(SharedThis(this)));
	}

	BindCommands();

	CreateInternalWidgets();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarBuilder->AddDungeonMakerToolbar(ToolbarExtender);

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_DungeonMakerEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.225f)
					->AddTab(FDungeonMakerAssetEditorTabs::ViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(FDungeonMakerAssetEditorTabs::DetailsID, ETabState::OpenedTab)->SetHideTabWell(true)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, DungeonMakerEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingGraph, false);

	RegenerateMenusAndToolbars();
}

void FDungeonMakerAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SoundCueEditor", "Sound Cue Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FDungeonMakerAssetEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FDungeonMakerAssetEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FDungeonMakerAssetEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FDungeonMakerAssetEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FDungeonMakerAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FDungeonMakerAssetEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FDungeonMakerAssetEditorTabs::DetailsID);
}

FName FDungeonMakerAssetEditor::GetToolkitFName() const
{
	return FName("FDungeonMakerEditor");
}

FText FDungeonMakerAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("DungeonMakerEditorAppLabel", "Dungeon Mission Editor");
}

FText FDungeonMakerAssetEditor::GetToolkitName() const
{
	const bool bDirtyState = EditingGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("DungeonMakerName"), FText::FromString(EditingGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("DungeonMakerEditorToolkitName", "{DungeonMakerName}{DirtyState}"), Args);
}

FText FDungeonMakerAssetEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingGraph);
}

FLinearColor FDungeonMakerAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FDungeonMakerAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("DungeonMakerEditor");
}

FString FDungeonMakerAssetEditor::GetDocumentationLink() const
{
	return TEXT("");
}

void FDungeonMakerAssetEditor::SaveAsset_Execute()
{
	if (EditingGraph != nullptr)
	{
		RebuildDungeonMaker();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

void FDungeonMakerAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditingGraph);
	Collector.AddReferencedObject(EditingGraph->EdGraph);
}

TSharedRef<SDockTab> FDungeonMakerAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDungeonMakerAssetEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (ViewportWidget.IsValid())
	{
		SpawnedTab->SetContent(ViewportWidget.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FDungeonMakerAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDungeonMakerAssetEditorTabs::DetailsID);

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("Details_Title", "Details"))
		[
			PropertyWidget.ToSharedRef()
		];
}

void FDungeonMakerAssetEditor::CreateInternalWidgets()
{
	ViewportWidget = CreateViewportWidget();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyWidget = PropertyModule.CreateDetailView(Args);
	PropertyWidget->SetObject(EditingGraph);

	//PropertyWidget->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FDungeonMakerAssetEditor::IsPropertyEditable));
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FDungeonMakerAssetEditor::OnFinishedChangingProperties);
}

TSharedRef<SGraphEditor> FDungeonMakerAssetEditor::CreateViewportWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_DungeonMaker", "Dungeon Mission");

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDungeonMakerAssetEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FDungeonMakerAssetEditor::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditingGraph->EdGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}


void FDungeonMakerAssetEditor::BindCommands()
{
	ToolkitCommands->MapAction(FDungeonMakerEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateSP(this, &FDungeonMakerAssetEditor::GraphSettings),
		FCanExecuteAction::CreateSP(this, &FDungeonMakerAssetEditor::CanGraphSettings)
	);
}

void FDungeonMakerAssetEditor::CreateEdGraph()
{
	if (EditingGraph->EdGraph == nullptr)
	{
		EditingGraph->EdGraph = CastChecked<UDungeonMakerEdGraph>(FBlueprintEditorUtils::CreateNewGraph(EditingGraph, NAME_None, UDungeonMakerEdGraph::StaticClass(), UDungeonMakerAssetGraphSchema::StaticClass()));
		EditingGraph->EdGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the results node)
		const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);
	}
}

void FDungeonMakerAssetEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);

	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FDungeonMakerEditorCommands::Get().GraphSettings,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::GraphSettings),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanGraphSettings));

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FDungeonMakerAssetEditor::CanDuplicateNodes)
	);
}

TSharedPtr<SGraphEditor> FDungeonMakerAssetEditor::GetCurrGraphEditor()
{
	return ViewportWidget;
}

FGraphPanelSelectionSet FDungeonMakerAssetEditor::GetSelectedNodes()
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FDungeonMakerAssetEditor::RebuildDungeonMaker()
{
	if (EditingGraph == nullptr)
	{
		LOG_WARNING(TEXT("FDungeonMakerAssetEditor::RebuildDungeonMaker EditingGraph is nullptr"));
		return;
	}

	UDungeonMakerEdGraph* EdGraph = Cast<UDungeonMakerEdGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildDungeonMaker();
}

void FDungeonMakerAssetEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FDungeonMakerAssetEditor::CanSelectAllNodes()
{
	return true;
}

void FDungeonMakerAssetEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				Node->Modify();
				Node->DestroyNode();
			}
		}
	}

	LOG_WARNING(TEXT("FDungeonMakerAssetEditor::DeleteSelectedNodes Exec"));
}

bool FDungeonMakerAssetEditor::CanDeleteNodes()
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	LOG_WARNING(TEXT("FDungeonMakerAssetEditor::CanDeleteNodes Can't delete"));

	return false;
}

void FDungeonMakerAssetEditor::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FDungeonMakerAssetEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FDungeonMakerAssetEditor::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FDungeonMakerAssetEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FGenericPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FDungeonMakerAssetEditor::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FDungeonMakerAssetEditor::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FDungeonMakerAssetEditor::PasteNodesHere(const FVector2D& Location)
{
}

bool FDungeonMakerAssetEditor::CanPasteNodes()
{
	return false;
}

void FDungeonMakerAssetEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FDungeonMakerAssetEditor::CanDuplicateNodes()
{
	//return CanCopyNodes();
	return false;
}

void FDungeonMakerAssetEditor::GraphSettings()
{
	PropertyWidget->SetObject(EditingGraph);

	LOG_WARNING(TEXT("FDungeonMakerAssetEditor::GraphSettings"));
}

bool FDungeonMakerAssetEditor::CanGraphSettings() const
{
	return true;
}

void FDungeonMakerAssetEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : NewSelection)
	{
		Selection.Add(SelectionEntry);
	}

	if (Selection.Num() == 0) 
	{
		PropertyWidget->SetObject(EditingGraph);

	}
	else if (Selection.Num() == 1)
	{
		PropertyWidget->SetObject(Selection[0]);
	}
	else
	{
		PropertyWidget->SetObject(nullptr);
	}
}

void FDungeonMakerAssetEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	
}

void FDungeonMakerAssetEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingGraph == nullptr)
		return;

	RebuildDungeonMaker();

	EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();
}

void FDungeonMakerAssetEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildDungeonMaker();
}

void FDungeonMakerAssetEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager) 
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


#undef LOCTEXT_NAMESPACE

