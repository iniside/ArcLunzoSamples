// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlViewModelScreens.h"

#include "ArcRmlDocumentRef.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Host/ArcRmlViewportSubsystem.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlViewModelScreens)

DEFINE_LOG_CATEGORY_STATIC(LogArcRmlTestScreens, Log, All);

namespace ArcRmlViewModelScreensPrivate
{
	const FName ContextName(TEXT("VM_ArcRmlScreens"));
	const FName ModelName(TEXT("screen"));
	const TCHAR* const Folder = TEXT("/ArcRmlUITest/UI/Rml/Test/ViewModelScreens/");

	struct FScreen
	{
		const TCHAR* Case;
		const TCHAR* File;
	};

	const FScreen Screens[] = {
		{TEXT("context"), TEXT("Context.rml")},
		{TEXT("resolver"), TEXT("Resolver.rml")},
		{TEXT("create"), TEXT("Create.rml")},
		{TEXT("manual"), TEXT("Manual.rml")},
		{TEXT("path"), TEXT("Path.rml")},
		{TEXT("settable"), TEXT("Settable.rml")},
	};

	FArcRmlDocumentRef DocumentOf(const FScreen& Screen)
	{
		return FArcRmlDocumentRef{FString(Folder) + Screen.File};
	}

	const FScreen* FindScreen(const FString& Case)
	{
		for (const FScreen& Screen : Screens)
		{
			if (Case.Equals(Screen.Case, ESearchCase::IgnoreCase))
			{
				return &Screen;
			}
		}
		return nullptr;
	}

	UArcRmlScreenViewModel* MakeModel(UObject* Outer, const TCHAR* Label, int32 Level, int32 Gold)
	{
		UArcRmlScreenViewModel* Model = NewObject<UArcRmlScreenViewModel>(Outer ? Outer : GetTransientPackage());
		Model->Label = Label;
		Model->Level = Level;
		Model->Gold = Gold;
		return Model;
	}

	void RegisterContextModel(UGameInstance& GameInstance)
	{
		UMVVMGameSubsystem* Subsystem = GameInstance.GetSubsystem<UMVVMGameSubsystem>();
		UMVVMViewModelCollectionObject* Collection = Subsystem ? Subsystem->GetViewModelCollection() : nullptr;
		if (!Collection)
		{
			UE_LOGF(LogArcRmlTestScreens, Warning, "ArcRmlTest.ViewModelScreens: the game instance has no MVVM view model collection");
			return;
		}
		FMVVMViewModelContext Context;
		Context.ContextClass = UArcRmlScreenViewModel::StaticClass();
		Context.ContextName = ContextName;
		Collection->RemoveViewModel(Context);
		Collection->AddViewModelInstance(Context, MakeModel(nullptr, TEXT("Context"), 11, 1100));
	}

	void CloseOtherScreens(UArcRmlViewportSubsystem& Subsystem, const FScreen& Keep)
	{
		for (const FScreen& Screen : Screens)
		{
			const FArcRmlDocumentRef Document = DocumentOf(Screen);
			if (&Screen != &Keep && Subsystem.IsDocumentOpen(Document))
			{
				Subsystem.CloseDocument(Document);
			}
		}
	}

	void HandleCommand(const TArray<FString>& Args, UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		UArcRmlViewportSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Subsystem || Args.IsEmpty())
		{
			UE_LOGF(LogArcRmlTestScreens, Warning,
				"ArcRmlTest.ViewModelScreens: needs a local player and a case: context, resolver, create, manual, path, settable, replace or reload");
			return;
		}

		const FString& Case = Args[0];
		if (Case.Equals(TEXT("replace"), ESearchCase::IgnoreCase))
		{
			const FArcRmlDocumentRef Document = DocumentOf(*FindScreen(TEXT("settable")));
			const bool bSet = Subsystem->SetViewModel(Document, ModelName, MakeModel(nullptr, TEXT("Replaced"), 77, 7700));
			UE_LOGF(LogArcRmlTestScreens, Display, "ArcRmlTest.ViewModelScreens: replace on '%ls' (open %ls) returned %ls", *Document.Path,
				Subsystem->IsDocumentOpen(Document) ? TEXT("true") : TEXT("false"), bSet ? TEXT("true") : TEXT("false"));
			return;
		}
		if (Case.Equals(TEXT("reload"), ESearchCase::IgnoreCase))
		{
#if WITH_EDITOR
			UE_LOGF(LogArcRmlTestScreens, Display, "ArcRmlTest.ViewModelScreens: reloading the open views");
			UArcRmlViewportSubsystem::ReloadOpenViews();
#else
			UE_LOGF(LogArcRmlTestScreens, Warning, "ArcRmlTest.ViewModelScreens: reload needs an editor build");
#endif
			return;
		}

		const FScreen* Screen = FindScreen(Case);
		if (!Screen)
		{
			UE_LOGF(LogArcRmlTestScreens, Warning, "ArcRmlTest.ViewModelScreens: unknown case '%ls'", *Case);
			return;
		}
		CloseOtherScreens(*Subsystem, *Screen);

		const FArcRmlDocumentRef Document = DocumentOf(*Screen);
		if (Case.Equals(TEXT("context"), ESearchCase::IgnoreCase))
		{
			RegisterContextModel(*GameInstance);
		}
		else if (Case.Equals(TEXT("manual"), ESearchCase::IgnoreCase))
		{
			const bool bSet = Subsystem->SetViewModel(Document, ModelName, MakeModel(nullptr, TEXT("Manual"), 44, 4400));
			UE_LOGF(LogArcRmlTestScreens, Display, "ArcRmlTest.ViewModelScreens: manual assignment before open returned %ls", bSet ? TEXT("true") : TEXT("false"));
		}
		else if (Case.Equals(TEXT("path"), ESearchCase::IgnoreCase) && !Cast<AArcRmlScreenPlayerController>(LocalPlayer->GetPlayerController(World)))
		{
			UE_LOGF(LogArcRmlTestScreens, Warning,
				"ArcRmlTest.ViewModelScreens: the path case needs AArcRmlScreenPlayerController; open the map with ?game=/Script/ArcRmlUITest.ArcRmlScreenGameMode");
		}

		const bool bOpened = Subsystem->OpenDocument(Document);
		UE_LOGF(LogArcRmlTestScreens, Display, "ArcRmlTest.ViewModelScreens: opened '%ls': %ls", *Document.Path, bOpened ? TEXT("true") : TEXT("false"));
	}

	FAutoConsoleCommandWithWorldAndArgs ScreensCommand(TEXT("ArcRmlTest.ViewModelScreens"),
		TEXT("Opens one view model source screen, closing the others: context registers in the MVVM collection, manual assigns before open, path needs the ArcRmlScreenGameMode player controller; replace sets a new model on the open settable screen, reload reloads the open views: ArcRmlTest.ViewModelScreens <context|resolver|create|manual|path|settable|replace|reload>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleCommand));
}

UArcRmlScreenCreatedViewModel::UArcRmlScreenCreatedViewModel()
{
	Label = TEXT("Created");
	Level = 33;
	Gold = 3300;
}

UObject* UArcRmlScreenResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (ExpectedType && !UArcRmlScreenViewModel::StaticClass()->IsChildOf(ExpectedType))
	{
		return nullptr;
	}
	return ArcRmlViewModelScreensPrivate::MakeModel(nullptr, *Label, Level, Gold);
}

AArcRmlScreenPlayerController::AArcRmlScreenPlayerController()
{
	bShowMouseCursor = true;
}

void AArcRmlScreenPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ScreenViewModel = ArcRmlViewModelScreensPrivate::MakeModel(this, TEXT("PlayerController"), 55, 5500);
}

AArcRmlScreenGameMode::AArcRmlScreenGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AArcRmlScreenPlayerController::StaticClass();
}
