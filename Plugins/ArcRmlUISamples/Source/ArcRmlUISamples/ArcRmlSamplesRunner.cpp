// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlSamplesRunner.h"

#if !UE_BUILD_SHIPPING

#include "ArcRmlSamplesLog.h"
#include "Aliases/ArcRmlSamplesAliases.h"
#include "Animations/ArcRmlSamplesAnimations.h"
#include "BackAndParent/ArcRmlSamplesBackAndParent.h"
#include "Component/ArcRmlSamplesComponent.h"
#include "DragDrop/ArcRmlSamplesDragDrop.h"
#include "Events/ArcRmlSamplesEvents.h"
#include "FieldBinding/ArcRmlSamplesFieldBinding.h"
#include "FirstScreen/ArcRmlSamplesFirstScreen.h"
#include "FocusAreas/ArcRmlSamplesFocusAreas.h"
#include "FocusRequest/ArcRmlSamplesFocusRequest.h"
#include "FormControls/ArcRmlSamplesFormControls.h"
#include "FrameTabs/ArcRmlSamplesFrameTabs.h"
#include "GamepadMenu/ArcRmlSamplesGamepadMenu.h"
#include "Images/ArcRmlSamplesImages.h"
#include "InnerRml/ArcRmlSamplesInnerRml.h"
#include "KeepAlive/ArcRmlSamplesKeepAlive.h"
#include "Lists/ArcRmlSamplesLists.h"
#include "Localization/ArcRmlSamplesLocalization.h"
#include "NestedViewModel/ArcRmlSamplesNestedViewModel.h"
#include "RowHotkeys/ArcRmlSamplesRowHotkeys.h"
#include "Sounds/ArcRmlSamplesSounds.h"
#include "SourceContext/ArcRmlSamplesSourceContext.h"
#include "SourceManual/ArcRmlSamplesSourceManual.h"
#include "SourcePath/ArcRmlSamplesSourcePath.h"
#include "SourceResolver/ArcRmlSamplesSourceResolver.h"
#include "SourceSettable/ArcRmlSamplesSourceSettable.h"
#include "StructList/ArcRmlSamplesStructList.h"
#include "TabBar/ArcRmlSamplesTabBar.h"
#include "Tailwind/ArcRmlSamplesTailwind.h"
#include "TextFields/ArcRmlSamplesTextFields.h"
#include "Themes/ArcRmlSamplesThemes.h"
#include "UmgEvents/ArcRmlSamplesUmgEvents.h"
#include "UmgLayout/ArcRmlSamplesUmgLayout.h"
#include "UmgOwnerData/ArcRmlSamplesUmgOwnerData.h"
#include "UmgWidget/ArcRmlSamplesUmgWidget.h"
#include "VirtualList/ArcRmlSamplesVirtualList.h"
#include "WorldMarker/ArcRmlSamplesWorldMarker.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Host/ArcRmlViewportSubsystem.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace ArcRmlSamplesRunner
{
	const TArray<FSampleEntry>& GetSamples()
	{
		static const TArray<FSampleEntry> Samples = {
			{TEXT("FirstScreen"), &ArcRmlSamplesFirstScreen::Open},
			{TEXT("FieldBinding"), &ArcRmlSamplesFieldBinding::Open},
			{TEXT("Events"), &ArcRmlSamplesEvents::Open},
			{TEXT("Lists"), &ArcRmlSamplesLists::Open},
			{TEXT("StructList"), &ArcRmlSamplesStructList::Open},
			{TEXT("NestedViewModel"), &ArcRmlSamplesNestedViewModel::Open},
			{TEXT("SourceContext"), &ArcRmlSamplesSourceContext::Open},
			{TEXT("SourceResolver"), &ArcRmlSamplesSourceResolver::Open},
			{TEXT("SourceManual"), &ArcRmlSamplesSourceManual::Open},
			{TEXT("SourcePath"), &ArcRmlSamplesSourcePath::Open},
			{TEXT("SourceSettable"), &ArcRmlSamplesSourceSettable::Open},
			{TEXT("Component"), &ArcRmlSamplesComponent::Open},
			{TEXT("GamepadMenu"), &ArcRmlSamplesGamepadMenu::Open},
			{TEXT("FocusAreas"), &ArcRmlSamplesFocusAreas::Open},
			{TEXT("BackAndParent"), &ArcRmlSamplesBackAndParent::Open},
			{TEXT("TextFields"), &ArcRmlSamplesTextFields::Open},
			{TEXT("TabBar"), &ArcRmlSamplesTabBar::Open},
			{TEXT("DragDrop"), &ArcRmlSamplesDragDrop::Open},
			{TEXT("FrameTabs"), &ArcRmlSamplesFrameTabs::Open},
			{TEXT("Themes"), &ArcRmlSamplesThemes::Open},
			{TEXT("Animations"), &ArcRmlSamplesAnimations::Open},
			{TEXT("Localization"), &ArcRmlSamplesLocalization::Open},
			{TEXT("WorldMarker"), &ArcRmlSamplesWorldMarker::Open},
			{TEXT("Tailwind"), &ArcRmlSamplesTailwind::Open},
			{TEXT("Images"), &ArcRmlSamplesImages::Open},
			{TEXT("Sounds"), &ArcRmlSamplesSounds::Open},
			{TEXT("FormControls"), &ArcRmlSamplesFormControls::Open},
			{TEXT("FocusRequest"), &ArcRmlSamplesFocusRequest::Open},
			{TEXT("RowHotkeys"), &ArcRmlSamplesRowHotkeys::Open},
			{TEXT("KeepAlive"), &ArcRmlSamplesKeepAlive::Open, {
				FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/KeepAlive/Kept.rml")},
				FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/KeepAlive/Preloaded.rml")}}},
			{TEXT("VirtualList"), &ArcRmlSamplesVirtualList::Open},
			{TEXT("Aliases"), &ArcRmlSamplesAliases::Open},
			{TEXT("InnerRml"), &ArcRmlSamplesInnerRml::Open},
			{TEXT("UmgWidget"), &ArcRmlSamplesUmgWidget::Open, {}, &ArcRmlSamplesUmgWidget::Close, &ArcRmlSamplesUmgWidget::IsOpen},
			{TEXT("UmgLayout"), &ArcRmlSamplesUmgLayout::Open, {}, &ArcRmlSamplesUmgLayout::Close, &ArcRmlSamplesUmgLayout::IsOpen},
			{TEXT("UmgOwnerData"), &ArcRmlSamplesUmgOwnerData::Open, {}, &ArcRmlSamplesUmgOwnerData::Close, &ArcRmlSamplesUmgOwnerData::IsOpen},
			{TEXT("UmgEvents"), &ArcRmlSamplesUmgEvents::Open, {}, &ArcRmlSamplesUmgEvents::Close, &ArcRmlSamplesUmgEvents::IsOpen},
		};
		return Samples;
	}

	const TArray<FBlueprintRoute>& GetBlueprintRoutes()
	{
		static const TArray<FBlueprintRoute> Routes = {
			{TEXT("FirstScreen"), TEXT("/ArcRmlUISamples/UI/Rml/FirstScreen/BP_FirstScreen_Open.BP_FirstScreen_Open_C")},
			{TEXT("SourceContext"), TEXT("/ArcRmlUISamples/UI/Rml/SourceContext/BP_SourceContext_Open.BP_SourceContext_Open_C")},
			{TEXT("SourceManual"), TEXT("/ArcRmlUISamples/UI/Rml/SourceManual/BP_SourceManual_Open.BP_SourceManual_Open_C")},
			{TEXT("BlueprintViewModel"), TEXT("/ArcRmlUISamples/UI/Rml/BlueprintViewModel/BP_BlueprintViewModel_Open.BP_BlueprintViewModel_Open_C")},
		};
		return Routes;
	}

	FArcRmlDocumentRef LastOpened;
	TArray<FArcRmlDocumentRef> LastOpenedExtras;
	TWeakObjectPtr<UClass> LastOpenedLibrary;
	FCloseFunction LastOpenedClose = nullptr;
	FIsOpenFunction LastOpenedIsOpen = nullptr;

	ULocalPlayer* FindLocalPlayer(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	}

	FString ListSampleNames()
	{
		TArray<FString> Names;
		for (const FSampleEntry& Entry : GetSamples())
		{
			Names.Add(Entry.Name);
		}
		return Names.IsEmpty() ? FString(TEXT("(none)")) : FString::Join(Names, TEXT(", "));
	}

	FString ListBlueprintRouteNames()
	{
		TArray<FString> Names;
		for (const FBlueprintRoute& Route : GetBlueprintRoutes())
		{
			Names.Add(Route.Name);
		}
		return FString::Join(Names, TEXT(", "));
	}

	bool IsInputParameter(const FProperty* Property)
	{
		return !Property->HasAnyPropertyFlags(CPF_OutParm) || Property->HasAnyPropertyFlags(CPF_ReferenceParm);
	}

	bool CallLibraryFunction(UClass* Library, FName FunctionName, APlayerController* Player, FArcRmlDocumentRef& Document)
	{
		UFunction* Function = Library ? Library->FindFunctionByName(FunctionName) : nullptr;
		if (!Function || !Player)
		{
			return false;
		}

		FStructOnScope Params(Function);
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (!IsInputParameter(*It))
			{
				continue;
			}
			if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(*It); ObjectProperty && Player->IsA(ObjectProperty->PropertyClass))
			{
				ObjectProperty->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Player);
			}
			else if (const FStructProperty* StructProperty = CastField<FStructProperty>(*It); StructProperty && StructProperty->Struct == FArcRmlDocumentRef::StaticStruct())
			{
				*StructProperty->ContainerPtrToValuePtr<FArcRmlDocumentRef>(Params.GetStructMemory()) = Document;
			}
		}

		Library->GetDefaultObject()->ProcessEvent(Function, Params.GetStructMemory());

		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			const FStructProperty* StructProperty = CastField<FStructProperty>(*It);
			if (StructProperty && StructProperty->Struct == FArcRmlDocumentRef::StaticStruct() && !IsInputParameter(*It))
			{
				Document = *StructProperty->ContainerPtrToValuePtr<FArcRmlDocumentRef>(Params.GetStructMemory());
			}
		}
		return true;
	}

	const FSampleEntry* FindSample(const FString& Name)
	{
		return GetSamples().FindByPredicate([&Name](const FSampleEntry& Entry)
		{
			return Name.Equals(Entry.Name, ESearchCase::IgnoreCase);
		});
	}

	const FBlueprintRoute* FindBlueprintRoute(const FString& Name)
	{
		return GetBlueprintRoutes().FindByPredicate([&Name](const FBlueprintRoute& Route)
		{
			return Name.Equals(Route.Name, ESearchCase::IgnoreCase);
		});
	}

	void OpenSample(const FSampleEntry& Sample, ULocalPlayer* LocalPlayer)
	{
		LastOpened = Sample.Open(LocalPlayer);
		LastOpenedExtras = Sample.ExtraDocuments;
		LastOpenedLibrary = nullptr;
		LastOpenedClose = Sample.Close;
		LastOpenedIsOpen = Sample.IsOpen;
	}

	bool OpenBlueprintSample(const FBlueprintRoute& Route, APlayerController* Player)
	{
		UClass* Library = LoadObject<UClass>(nullptr, Route.Library);
		FArcRmlDocumentRef Document;
		if (!CallLibraryFunction(Library, TEXT("Open"), Player, Document))
		{
			return false;
		}
		LastOpened = Document;
		LastOpenedExtras.Reset();
		LastOpenedLibrary = Library;
		LastOpenedClose = nullptr;
		LastOpenedIsOpen = nullptr;
		return true;
	}

	FArcRmlDocumentRef CloseLastOpened(ULocalPlayer* LocalPlayer, UWorld* World)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport || !LastOpened.IsSet())
		{
			return FArcRmlDocumentRef();
		}
		TArray<FArcRmlDocumentRef> OpenExtras;
		for (const FArcRmlDocumentRef& Extra : LastOpenedExtras)
		{
			if (Viewport->IsDocumentOpen(Extra))
			{
				OpenExtras.Add(Extra);
			}
		}
		const bool bMainOpen = LastOpenedIsOpen ? LastOpenedIsOpen(LocalPlayer) : Viewport->IsDocumentOpen(LastOpened);
		if (!bMainOpen && OpenExtras.IsEmpty())
		{
			return FArcRmlDocumentRef();
		}
		for (const FArcRmlDocumentRef& Extra : OpenExtras)
		{
			Viewport->CloseDocument(Extra);
		}
		if (bMainOpen && LastOpenedClose)
		{
			LastOpenedClose(LocalPlayer);
		}
		else if (bMainOpen)
		{
			FArcRmlDocumentRef Document = LastOpened;
			UClass* Library = LastOpenedLibrary.Get();
			if (!Library || !Library->FindFunctionByName(TEXT("Close")) || !CallLibraryFunction(Library, TEXT("Close"), LocalPlayer->GetPlayerController(World), Document))
			{
				Viewport->CloseDocument(LastOpened);
			}
		}
		const FArcRmlDocumentRef Closed = bMainOpen ? LastOpened : OpenExtras.Last();
		LastOpened = FArcRmlDocumentRef();
		LastOpenedExtras.Reset();
		LastOpenedLibrary = nullptr;
		LastOpenedClose = nullptr;
		LastOpenedIsOpen = nullptr;
		return Closed;
	}

	void HandleOpenCommand(const TArray<FString>& Args, UWorld* World)
	{
		const FSampleEntry* Found = Args.IsEmpty() ? nullptr : FindSample(Args[0]);
		if (!Found)
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples.Open: unknown sample '%ls'; samples: %ls", Args.IsEmpty() ? TEXT("") : *Args[0], *ListSampleNames());
			return;
		}
		ULocalPlayer* LocalPlayer = FindLocalPlayer(World);
		if (!LocalPlayer)
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples.Open: this world has no local player");
			return;
		}
		OpenSample(*Found, LocalPlayer);
	}

	void HandleOpenBlueprintCommand(const TArray<FString>& Args, UWorld* World)
	{
		const FBlueprintRoute* Found = Args.IsEmpty() ? nullptr : FindBlueprintRoute(Args[0]);
		if (!Found)
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples.OpenBP: unknown sample '%ls'; Blueprint samples: %ls", Args.IsEmpty() ? TEXT("") : *Args[0], *ListBlueprintRouteNames());
			return;
		}
		ULocalPlayer* LocalPlayer = FindLocalPlayer(World);
		APlayerController* Player = LocalPlayer ? LocalPlayer->GetPlayerController(World) : nullptr;
		if (!Player)
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples.OpenBP: this world has no local player controller");
			return;
		}
		if (!OpenBlueprintSample(*Found, Player))
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples.OpenBP: %ls has no Open function", Found->Library);
		}
	}

	void HandleCloseCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!CloseLastOpened(FindLocalPlayer(World), World).IsSet())
		{
			UE_LOGF(LogArcRmlSamples, Display, "ArcRmlSamples.Close: no sample is open");
		}
	}

	FAutoConsoleCommandWithWorldAndArgs OpenCommand(TEXT("ArcRmlSamples.Open"), TEXT("Opens an ArcRmlUI sample by its folder name for the first local player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOpenCommand));

	FAutoConsoleCommandWithWorldAndArgs OpenBlueprintCommand(TEXT("ArcRmlSamples.OpenBP"), TEXT("Opens an ArcRmlUI sample through the Open function of its Blueprint function library for the first local player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleOpenBlueprintCommand));

	FAutoConsoleCommandWithWorldAndArgs CloseCommand(TEXT("ArcRmlSamples.Close"), TEXT("Closes the document of the last opened ArcRmlUI sample"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleCloseCommand));
}

#endif
