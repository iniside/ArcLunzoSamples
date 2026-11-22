// Copyright 2026 Lukasz Baran. All Rights Reserved.

#if WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "Components/Widget.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "GraphEditor.h"
#include "HAL/IConsoleManager.h"
#include "IDetailsView.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/Paths.h"
#include "PropertyPath.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "WidgetReference.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogArcRmlWidgetEditorCommand, Log, All);

namespace ArcRmlWidgetEditorCommandPrivate
{
	const TCHAR* Usage = TEXT("ArcRmlTest.WidgetEditor: usage ArcRmlTest.WidgetEditor <object path> Designer [Select=<widget>] [Property=<name>] [Palette=<filter>] | Graph | Shot <file name>");

	void FindWidgetsOfType(const TSharedRef<SWidget>& Root, const FName Type, TArray<TSharedRef<SWidget>>& OutWidgets)
	{
		if (Root->GetType() == Type)
		{
			OutWidgets.Add(Root);
		}
		FChildren* Children = Root->GetChildren();
		for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
		{
			FindWidgetsOfType(Children->GetChildAt(Index), Type, OutWidgets);
		}
	}

	TSharedPtr<SWindow> FindEditorWindow(FWidgetBlueprintEditor& Editor)
	{
		const TSharedPtr<FTabManager> TabManager = Editor.GetTabManager();
		const TSharedPtr<SDockTab> OwnerTab = TabManager.IsValid() ? TabManager->GetOwnerTab() : nullptr;
		return OwnerTab.IsValid() ? OwnerTab->GetParentWindow() : nullptr;
	}

	void ShowPropertyInDetails(const TSharedRef<SWindow>& Window, const UClass* WidgetClass, const FName PropertyName)
	{
		const FProperty* Property = FindFProperty<FProperty>(WidgetClass, PropertyName);
		if (!Property)
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: %ls has no property %ls", *WidgetClass->GetName(), *PropertyName.ToString());
			return;
		}
		TArray<TSharedRef<SWidget>> DetailsViews;
		FindWidgetsOfType(Window, FName(TEXT("SDetailsView")), DetailsViews);
		for (const TSharedRef<SWidget>& Widget : DetailsViews)
		{
			const TSharedRef<IDetailsView> DetailsView = StaticCastSharedRef<IDetailsView>(Widget);
			for (const TWeakObjectPtr<UObject>& Selected : DetailsView->GetSelectedObjects())
			{
				if (Selected.IsValid() && Selected->IsA(WidgetClass))
				{
					const TSharedRef<FPropertyPath> Path = FPropertyPath::CreateEmpty()->ExtendPath(FPropertyInfo(const_cast<FProperty*>(Property)));
					DetailsView->ScrollPropertyIntoView(*Path, false);
					DetailsView->HighlightProperty(*Path);
					return;
				}
			}
		}
		UE_LOGF(LogArcRmlWidgetEditorCommand, Warning, "ArcRmlTest.WidgetEditor: no details view shows a %ls yet", *WidgetClass->GetName());
	}

	void TypePaletteFilter(const TSharedRef<SWindow>& Window, const FString& Filter)
	{
		TArray<TSharedRef<SWidget>> Palettes;
		FindWidgetsOfType(Window, FName(TEXT("SPaletteView")), Palettes);
		TArray<TSharedRef<SWidget>> SearchBoxes;
		if (!Palettes.IsEmpty())
		{
			FindWidgetsOfType(Palettes[0], FName(TEXT("SSearchBox")), SearchBoxes);
		}
		if (SearchBoxes.IsEmpty())
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Warning, "ArcRmlTest.WidgetEditor: no Palette search box in the editor window");
			return;
		}
		const TSharedRef<SSearchBox> SearchBox = StaticCastSharedRef<SSearchBox>(SearchBoxes[0]);
		if (SearchBox->GetText().ToString() == Filter)
		{
			return;
		}
		SearchBox->SetText(FText::GetEmpty());
		FSlateApplication::Get().SetKeyboardFocus(SearchBox, EFocusCause::SetDirectly);
		const FModifierKeysState NoModifiers;
		for (const TCHAR Character : Filter)
		{
			FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(Character, NoModifiers, 0, false));
		}
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
	}

	void SaveWindowShot(const TSharedRef<SWindow>& Window, const FString& FileName)
	{
		TArray<FColor> Pixels;
		FIntVector Size;
		if (!FSlateApplication::Get().TakeScreenshot(Window, Pixels, Size))
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: TakeScreenshot failed");
			return;
		}
		for (FColor& Pixel : Pixels)
		{
			Pixel.A = 255;
		}
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir() / FileName + TEXT(".png"));
		if (FImageUtils::SaveImageAutoFormat(*Path, FImageView(Pixels.GetData(), Size.X, Size.Y)))
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Log, "ArcRmlTest.WidgetEditor: saved %ls (%dx%d)", *Path, Size.X, Size.Y);
		}
		else
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: could not write %ls", *Path);
		}
	}

	void HandleWidgetEditorCommand(const TArray<FString>& Args)
	{
		const bool bDesigner = Args.Num() >= 2 && Args[1].Equals(TEXT("Designer"), ESearchCase::IgnoreCase);
		const bool bGraph = Args.Num() >= 2 && Args[1].Equals(TEXT("Graph"), ESearchCase::IgnoreCase);
		const bool bShot = Args.Num() >= 3 && Args[1].Equals(TEXT("Shot"), ESearchCase::IgnoreCase);
		if (!GEditor || !FSlateApplication::IsInitialized() || (!bDesigner && !bGraph && !bShot))
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Warning, "%ls", Usage);
			return;
		}
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *Args[0]);
		UAssetEditorSubsystem* AssetEditors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (!Blueprint || !AssetEditors)
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: no Widget Blueprint at %ls", *Args[0]);
			return;
		}
		IAssetEditorInstance* Instance = AssetEditors->FindEditorForAsset(Blueprint, true);
		if (!Instance && AssetEditors->OpenEditorForAsset(Blueprint))
		{
			Instance = AssetEditors->FindEditorForAsset(Blueprint, true);
		}
		if (!Instance)
		{
			UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: could not open the editor for %ls", *Args[0]);
			return;
		}
		FWidgetBlueprintEditor* Editor = static_cast<FWidgetBlueprintEditor*>(Instance);
		const TSharedPtr<SWindow> Window = FindEditorWindow(*Editor);
		if (bShot)
		{
			if (!Window.IsValid())
			{
				UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: no window hosts the editor for %ls", *Args[0]);
				return;
			}
			SaveWindowShot(Window.ToSharedRef(), Args[2]);
			return;
		}
		if (bGraph)
		{
			Editor->SetCurrentMode(FWidgetBlueprintApplicationModes::GraphMode);
			UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
			if (!EventGraph)
			{
				UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: %ls has no Event Graph", *Args[0]);
				return;
			}
			Editor->OpenDocument(EventGraph, FDocumentTracker::OpenNewDocument);
			const TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(EventGraph);
			if (GraphEditor.IsValid())
			{
				GraphEditor->ZoomToFit(false);
			}
			return;
		}
		Editor->SetCurrentMode(FWidgetBlueprintApplicationModes::DesignerMode);
		FString SelectName;
		FString PropertyName;
		FString PaletteFilter;
		for (int32 Index = 2; Index < Args.Num(); ++Index)
		{
			if (!Args[Index].Split(TEXT("="), nullptr, nullptr))
			{
				UE_LOGF(LogArcRmlWidgetEditorCommand, Warning, "%ls", Usage);
				return;
			}
			FString Key;
			FString Value;
			Args[Index].Split(TEXT("="), &Key, &Value);
			if (Key.Equals(TEXT("Select"), ESearchCase::IgnoreCase))
			{
				SelectName = Value;
			}
			else if (Key.Equals(TEXT("Property"), ESearchCase::IgnoreCase))
			{
				PropertyName = Value;
			}
			else if (Key.Equals(TEXT("Palette"), ESearchCase::IgnoreCase))
			{
				PaletteFilter = Value;
			}
		}
		if (!SelectName.IsEmpty())
		{
			UWidget* Template = Blueprint->WidgetTree ? Blueprint->WidgetTree->FindWidget(FName(*SelectName)) : nullptr;
			if (!Template)
			{
				UE_LOGF(LogArcRmlWidgetEditorCommand, Error, "ArcRmlTest.WidgetEditor: no widget %ls in %ls", *SelectName, *Args[0]);
				return;
			}
			TSet<FWidgetReference> Selection;
			Selection.Add(Editor->GetReferenceFromTemplate(Template));
			Editor->SelectWidgets(Selection, false);
			if (!PropertyName.IsEmpty() && Window.IsValid())
			{
				ShowPropertyInDetails(Window.ToSharedRef(), Template->GetClass(), FName(*PropertyName));
			}
		}
		if (!PaletteFilter.IsEmpty() && Window.IsValid())
		{
			TypePaletteFilter(Window.ToSharedRef(), PaletteFilter);
		}
	}

	FAutoConsoleCommand WidgetEditorCommand(TEXT("ArcRmlTest.WidgetEditor"),
		TEXT("Drives a Widget Blueprint editor for screenshots: ArcRmlTest.WidgetEditor <object path> Designer [Select=<widget>] [Property=<name>] [Palette=<filter>] | Graph (Event Graph zoomed to fit) | Shot <file name> (the editor window as Saved/Screenshots/<platform>/<file name>.png)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleWidgetEditorCommand));
}

#endif
