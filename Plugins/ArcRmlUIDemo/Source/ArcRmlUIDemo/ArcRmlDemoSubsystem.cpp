// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoSubsystem.h"

#include "ArcRmlDemoDialogViewModel.h"
#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoHubViewModel.h"
#include "ArcRmlDemoLog.h"
#include "ArcRmlDemoScreenViewModel.h"
#include "ArcRmlDemoSettings.h"
#include "ArcRmlDemoUIViewModel.h"
#include "ArcRmlUISettings.h"
#include "CommonInputSettings.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Host/ArcRmlViewportSubsystem.h"
#include "ICommonInputModule.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MVVMGameSubsystem.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Types/MVVMViewModelCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoSubsystem)

namespace ArcRmlDemoSubsystemPrivate
{
	const FName UIContextName(TEXT("VM_ArcRmlDemoUI"));
	const FName GameContextName(TEXT("VM_ArcRmlDemoGame"));
	const FName ToastTimer(TEXT("toast"));
	const FName DialogModelName(TEXT("dialog"));
	const TCHAR* const DocumentRoot = TEXT("/ArcRmlUIDemo/UI/Rml/Demo/");
	const TCHAR* const DialogDocument = TEXT("Dialog");
	const TCHAR* const HubScreen = TEXT("hub");
	const TCHAR* const FirstHubTab = TEXT("Inventory");
	constexpr double ToastSeconds = 2.6;

	bool bWarnedMissingInputData = false;
	uint64 NextTimerSerial = 1;

	FMVVMViewModelContext MakeContext(UClass* Class, FName Name)
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = Class;
		Context.ContextName = Name;
		return Context;
	}

	UMVVMViewModelCollectionObject* FindCollection(const ULocalPlayer* LocalPlayer)
	{
		UGameInstance* GameInstance = LocalPlayer ? LocalPlayer->GetGameInstance() : nullptr;
		UMVVMGameSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
		return Subsystem ? Subsystem->GetViewModelCollection() : nullptr;
	}

	struct FScreenDocument
	{
		const TCHAR* Screen;
		const TCHAR* Document;
	};

	const FScreenDocument ScreenDocuments[] = {{TEXT("splash"), TEXT("Splash")}, {TEXT("main"), TEXT("MainMenu")}, {TEXT("settings"), TEXT("Settings")},
		{TEXT("saveload"), TEXT("SaveLoad")}, {TEXT("credits"), TEXT("Credits")}, {TEXT("lobby"), TEXT("Lobby")}, {TEXT("loading"), TEXT("LoadingScreen")},
		{TEXT("hud"), TEXT("Hud")}, {TEXT("pause"), TEXT("PauseMenu")}, {TEXT("hub"), TEXT("Hub")}, {TEXT("vendor"), TEXT("Vendor")},
		{TEXT("crafting"), TEXT("Crafting")}, {TEXT("results"), TEXT("MatchResults")}};

	FString DocumentPath(const FString& Document)
	{
		return FString(DocumentRoot) + Document + TEXT(".rml");
	}

	FArcRmlDocumentRef DialogRef()
	{
		return FArcRmlDocumentRef{DocumentPath(DialogDocument)};
	}

	FArcRmlDocumentRef HubFrameRef()
	{
		return FArcRmlDocumentRef{DocumentPath(TEXT("Hub"))};
	}

	const FScreenDocument* FindScreenDocument(const FString& Screen)
	{
		for (const FScreenDocument& Entry : ScreenDocuments)
		{
			if (Screen == Entry.Screen)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	FString ScreenKeyOf(const FString& Path)
	{
		for (const FScreenDocument& Entry : ScreenDocuments)
		{
			if (Path == DocumentPath(Entry.Document))
			{
				return Entry.Screen;
			}
		}
		return FString();
	}

	FString InputName(ECommonInputType InputType)
	{
		return InputType == ECommonInputType::Gamepad ? TEXT("gamepad") : TEXT("keyboard");
	}

	bool IsAnyOf(const FString& Value, std::initializer_list<const TCHAR*> Options)
	{
		for (const TCHAR* Option : Options)
		{
			if (Value == Option)
			{
				return true;
			}
		}
		return false;
	}

	bool IsRootScreen(const FString& Screen)
	{
		return IsAnyOf(Screen, {TEXT("main"), TEXT("splash"), TEXT("loading"), TEXT("hud")});
	}

#if !UE_BUILD_SHIPPING
	UArcRmlDemoSubsystem* FindDemo(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlDemoSubsystem>() : nullptr;
	}

	void HandleStartCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UArcRmlDemoSubsystem* Demo = FindDemo(World))
		{
			Demo->Start();
			return;
		}
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo.Start: this world has no local player");
	}

	void HandleStopCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UArcRmlDemoSubsystem* Demo = FindDemo(World))
		{
			Demo->Stop();
			return;
		}
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo.Stop: this world has no local player");
	}

	void HandleJumpCommand(const TArray<FString>& Args, UWorld* World)
	{
		UArcRmlDemoSubsystem* Demo = FindDemo(World);
		if (!Demo || !Demo->IsRunning() || Args.IsEmpty())
		{
			UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo.Jump: needs a running demo and a screen key");
			return;
		}
		Demo->Jump(Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs StartCommand(TEXT("ArcRmlDemo.Start"), TEXT("Starts the ArcRmlUI demo game UI for the first local player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleStartCommand));

	FAutoConsoleCommandWithWorldAndArgs StopCommand(TEXT("ArcRmlDemo.Stop"), TEXT("Stops the ArcRmlUI demo game UI of the first local player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleStopCommand));

	FAutoConsoleCommandWithWorldAndArgs JumpCommand(TEXT("ArcRmlDemo.Jump"), TEXT("Jumps the running demo to a screen key (splash, main, settings, saveload, credits, lobby, loading, ...)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandleJumpCommand));
#endif
}

void UArcRmlDemoSubsystem::Deinitialize()
{
	Stop();
	Super::Deinitialize();
}

void UArcRmlDemoSubsystem::Start()
{
	using namespace ArcRmlDemoSubsystemPrivate;

	if (bRunning)
	{
		if (bTravelPending)
		{
			ResumeAfterTravel();
			return;
		}
		UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: already running");
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();

	UMVVMViewModelCollectionObject* Collection = FindCollection(LocalPlayer);
	if (!Collection)
	{
		UE_LOGF(LogArcRmlDemo, Error, "ArcRmlDemo: cannot start, the game instance has no view model collection");
		return;
	}
	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!Viewport)
	{
		UE_LOGF(LogArcRmlDemo, Error, "ArcRmlDemo: cannot start, the local player has no ArcRmlUI viewport subsystem");
		return;
	}
	const UArcRmlDemoSettings* Settings = GetDefault<UArcRmlDemoSettings>();
	UClass* UIClass = Settings->UIViewModelClass.LoadSynchronous();
	UClass* GameClass = Settings->GameViewModelClass.LoadSynchronous();
	UClass* HubClass = Settings->HubViewModelClass.LoadSynchronous();
	if (!UIClass || !GameClass || !HubClass)
	{
		UE_LOGF(LogArcRmlDemo, Error, "ArcRmlDemo: cannot start, a view model class in the ArcRmlUI Demo settings does not load (UI '%ls', Game '%ls', Hub '%ls')",
			*Settings->UIViewModelClass.ToString(), *Settings->GameViewModelClass.ToString(), *Settings->HubViewModelClass.ToString());
		return;
	}
	const FMVVMViewModelContext UIContext = MakeContext(UIClass, UIContextName);
	const FMVVMViewModelContext GameContext = MakeContext(GameClass, GameContextName);
	if (Collection->FindViewModelInstance(UIContext) || Collection->FindViewModelInstance(GameContext))
	{
		UE_LOGF(LogArcRmlDemo, Error, "ArcRmlDemo: cannot start, context '%ls' or '%ls' is already registered", *UIContextName.ToString(),
			*GameContextName.ToString());
		return;
	}

	UIModel = NewObject<UArcRmlDemoUIViewModel>(this, UIClass);
	GameModel = NewObject<UArcRmlDemoGameViewModel>(this, GameClass);
	GameModel->ResetToStart();
	GameModel->WatchField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, bMultiplayer),
		UArcRmlDemoGameViewModel::FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleGameFieldChanged));
	Collection->AddViewModelInstance(UIContext, UIModel);
	Collection->AddViewModelInstance(GameContext, GameModel);
	bRunning = true;

	Viewport->OnDocumentActivated.AddDynamic(this, &ThisClass::HandleScreenActivated);
	Viewport->OnDocumentClosed.AddDynamic(this, &ThisClass::HandleScreenClosed);
	if (UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(LocalPlayer))
	{
		InputMethodHandle = CommonInput->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
		UIModel->SetInput(InputName(CommonInput->GetCurrentInputType()));
	}
	AddCommonMapping();
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::HandleTick), 0.0f);

	ResetTo(TArray<FString>{TEXT("splash")});
	FindOrCreateScreenModel(HubClass);
	PreloadDocuments();
	RefreshDevBar();
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: started, view model contexts '%ls' and '%ls'", *UIContextName.ToString(), *GameContextName.ToString());
}

void UArcRmlDemoSubsystem::Stop()
{
	using namespace ArcRmlDemoSubsystemPrivate;

	if (!bRunning)
	{
		return;
	}
	bRunning = false;
	bTravelPending = false;
	PendingScreens.Reset();

	if (UArcRmlViewportSubsystem* Viewport = GetViewport())
	{
		Viewport->OnDocumentActivated.RemoveAll(this);
		Viewport->OnDocumentClosed.RemoveAll(this);
	}
	CloseDocuments();
	DiscardDocuments();
	if (TickerHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Timers.Reset();

	ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	if (UCommonInputSubsystem* CommonInput = LocalPlayer ? UCommonInputSubsystem::Get(LocalPlayer) : nullptr)
	{
		CommonInput->OnInputMethodChangedNative.Remove(InputMethodHandle);
	}
	InputMethodHandle.Reset();
	RemoveCommonMapping();

	for (const TObjectPtr<UMVVMViewModelBase>& Model : ScreenModels)
	{
		if (UArcRmlDemoScreenViewModel* Screen = Cast<UArcRmlDemoScreenViewModel>(Model.Get()))
		{
			Screen->Shutdown();
		}
	}
	ScreenModels.Reset();

	if (UMVVMViewModelCollectionObject* Collection = FindCollection(LocalPlayer))
	{
		if (UIModel)
		{
			Collection->RemoveViewModel(MakeContext(UIModel->GetClass(), UIContextName));
		}
		if (GameModel)
		{
			Collection->RemoveViewModel(MakeContext(GameModel->GetClass(), GameContextName));
		}
	}
	if (GameModel)
	{
		GameModel->RemoveAllFieldValueChangedDelegates(this);
	}
	UIModel = nullptr;
	GameModel = nullptr;
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: stopped");
}

UArcRmlDemoUIViewModel* UArcRmlDemoSubsystem::GetUI() const
{
	return UIModel;
}

UArcRmlDemoGameViewModel* UArcRmlDemoSubsystem::GetGame() const
{
	return GameModel;
}

UArcRmlDemoDialogViewModel* UArcRmlDemoSubsystem::GetDialog() const
{
	using namespace ArcRmlDemoSubsystemPrivate;

	const UArcRmlViewportSubsystem* Viewport = GetViewport();
	return Viewport && Viewport->IsDocumentOpen(DialogRef()) ? Cast<UArcRmlDemoDialogViewModel>(Viewport->GetViewModel(DialogRef(), DialogModelName)) : nullptr;
}

UMVVMViewModelBase* UArcRmlDemoSubsystem::FindOrCreateScreenModel(const UClass* Class)
{
	if (!bRunning || !Class)
	{
		return nullptr;
	}
	for (const TObjectPtr<UMVVMViewModelBase>& Model : ScreenModels)
	{
		if (Model && Model->GetClass() == Class)
		{
			return Model;
		}
	}
	UMVVMViewModelBase* Created = NewObject<UMVVMViewModelBase>(this, const_cast<UClass*>(Class));
	ScreenModels.Add(Created);
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: created view model '%ls'", *Class->GetPathName());
	if (UArcRmlDemoScreenViewModel* Screen = Cast<UArcRmlDemoScreenViewModel>(Created))
	{
		Screen->Initialize();
	}
	return Created;
}

UArcRmlDemoScreenViewModel* UArcRmlDemoSubsystem::FindScreenModelByKey(const FString& Screen) const
{
	if (Screen.IsEmpty())
	{
		return nullptr;
	}
	for (const TObjectPtr<UMVVMViewModelBase>& Model : ScreenModels)
	{
		UArcRmlDemoScreenViewModel* ScreenModel = Cast<UArcRmlDemoScreenViewModel>(Model.Get());
		if (ScreenModel && ScreenModel->GetScreenKey() == Screen)
		{
			return ScreenModel;
		}
	}
	return nullptr;
}

UArcRmlViewportSubsystem* UArcRmlDemoSubsystem::GetViewport() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
}

TArray<FArcRmlDocumentRef> UArcRmlDemoSubsystem::GetOpenDocuments() const
{
	TArray<FArcRmlDocumentRef> Documents;
	const UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!Viewport)
	{
		return Documents;
	}
	for (const FGameplayTag& Layer : GetDefault<UArcRmlUISettings>()->LayerOrder)
	{
		for (const FArcRmlDocumentRef& Document : Viewport->GetLayerDocuments(Layer))
		{
			if (Document.Path.StartsWith(ArcRmlDemoSubsystemPrivate::DocumentRoot))
			{
				Documents.Add(Document);
			}
		}
	}
	return Documents;
}

TArray<FString> UArcRmlDemoSubsystem::GetStack() const
{
	TArray<FString> Screens;
	for (const FArcRmlDocumentRef& Document : GetOpenDocuments())
	{
		const FString Screen = ArcRmlDemoSubsystemPrivate::ScreenKeyOf(Document.Path);
		if (!Screen.IsEmpty())
		{
			Screens.Add(Screen);
		}
	}
	return Screens;
}

FString UArcRmlDemoSubsystem::GetTop() const
{
	const TArray<FString> Screens = GetStack();
	return Screens.IsEmpty() ? FString() : Screens.Last();
}

FString UArcRmlDemoSubsystem::GetScreenBelow(const FString& Screen) const
{
	const TArray<FString> Screens = GetStack();
	const int32 Index = Screens.FindLast(Screen);
	if (Index == INDEX_NONE)
	{
		return Screens.IsEmpty() ? FString() : Screens.Last();
	}
	return Index > 0 ? Screens[Index - 1] : FString();
}

FArcRmlDocumentRef UArcRmlDemoSubsystem::GetTopDocument() const
{
	const TArray<FArcRmlDocumentRef> Documents = GetOpenDocuments();
	for (int32 Index = Documents.Num() - 1; Index >= 0; --Index)
	{
		if (!ArcRmlDemoSubsystemPrivate::ScreenKeyOf(Documents[Index].Path).IsEmpty())
		{
			return Documents[Index];
		}
	}
	return FArcRmlDocumentRef();
}

void UArcRmlDemoSubsystem::Push(const FString& Screen, const FString& Param)
{
	if (bRunning)
	{
		OpenScreen(Screen, Param);
	}
}

void UArcRmlDemoSubsystem::ReplaceTop(const FString& Screen, const FString& Param)
{
	if (!bRunning)
	{
		return;
	}
	const FArcRmlDocumentRef Top = GetTopDocument();
	if (UArcRmlViewportSubsystem* Viewport = Top.IsSet() ? GetViewport() : nullptr)
	{
		Viewport->CloseDocument(Top);
	}
	OpenScreen(Screen, Param);
}

void UArcRmlDemoSubsystem::ResetTo(TConstArrayView<FString> Screens, const FString& Param)
{
	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!bRunning || !Viewport)
	{
		return;
	}
	DropDialogChoice();
	if (GameModel)
	{
		GameModel->SetSaveNotice(FString());
	}
	const TArray<FString> Stack = GetStack();
	int32 Kept = 0;
	while (Kept < Stack.Num() && Kept < Screens.Num() && Stack[Kept] == Screens[Kept])
	{
		++Kept;
	}
	int32 StackIndex = 0;
	for (const FArcRmlDocumentRef& Document : GetOpenDocuments())
	{
		const bool bScreen = !ArcRmlDemoSubsystemPrivate::ScreenKeyOf(Document.Path).IsEmpty();
		if (!bScreen || StackIndex++ >= Kept)
		{
			Viewport->CloseDocument(Document);
		}
	}
	for (int32 Index = Kept; Index < Screens.Num(); ++Index)
	{
		OpenScreen(Screens[Index], Index == Screens.Num() - 1 ? Param : FString());
	}
}

void UArcRmlDemoSubsystem::Pop()
{
	if (!bRunning || GetStack().Num() <= 1)
	{
		return;
	}
	if (UArcRmlViewportSubsystem* Viewport = GetViewport())
	{
		Viewport->CloseDocument(GetTopDocument());
	}
}

void UArcRmlDemoSubsystem::Back()
{
	if (!bRunning)
	{
		return;
	}
	if (UArcRmlDemoDialogViewModel* Dialog = GetDialog())
	{
		Dialog->Cancel();
		return;
	}
	if (UArcRmlDemoScreenViewModel* Screen = FindScreenModelByKey(GetTop()))
	{
		Screen->Back();
		return;
	}
	LeaveScreen();
}

void UArcRmlDemoSubsystem::LeaveScreen()
{
	if (bRunning && !ArcRmlDemoSubsystemPrivate::IsRootScreen(GetTop()))
	{
		Pop();
	}
}

void UArcRmlDemoSubsystem::Jump(const FString& Screen)
{
	if (!bRunning)
	{
		return;
	}
	static const TMap<FString, TArray<FString>> InGame = {{TEXT("hud"), {TEXT("hud")}}, {TEXT("pause"), {TEXT("hud"), TEXT("pause")}},
		{TEXT("hub"), {TEXT("hud"), TEXT("hub")}}, {TEXT("vendor"), {TEXT("hud"), TEXT("vendor")}}, {TEXT("crafting"), {TEXT("hud"), TEXT("crafting")}},
		{TEXT("results"), {TEXT("hud"), TEXT("results")}}};

	if (Screen == TEXT("splash"))
	{
		ResetTo(TArray<FString>{TEXT("splash")});
		return;
	}
	if (Screen == TEXT("loading"))
	{
		ResetTo(TArray<FString>{TEXT("main"), TEXT("loading")}, TEXT("hud"));
		return;
	}
	if (Screen == TEXT("results"))
	{
		GameModel->SetMultiplayer(true);
	}
	if (const TArray<FString>* InGameStack = InGame.Find(Screen))
	{
		ResetTo(*InGameStack);
		return;
	}
	ResetTo(Screen == TEXT("main") ? TArray<FString>{TEXT("main")} : TArray<FString>{TEXT("main"), Screen});
}

void UArcRmlDemoSubsystem::Travel(const TSoftObjectPtr<UWorld>& Map, TConstArrayView<FString> Screens)
{
	if (!bRunning)
	{
		return;
	}
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	UWorld* World = LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
	const FString MapPackage = Map.ToSoftObjectPath().GetLongPackageName();
	if (World && !MapPackage.IsEmpty() && UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) == MapPackage)
	{
		ResetTo(Screens);
		return;
	}
	if (!World || MapPackage.IsEmpty() || !FPackageName::DoesPackageExist(MapPackage))
	{
		UE_LOGF(LogArcRmlDemo, Error, "ArcRmlDemo: cannot travel to map '%ls' (not set in the ArcRmlUI Demo settings or not found); staying on the current map",
			*Map.ToString());
		ResetTo(Screens);
		return;
	}
	PendingScreens = TArray<FString>(Screens);
	bTravelPending = true;
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: travelling to '%ls'", *MapPackage);
	UGameplayStatics::OpenLevelBySoftObjectPtr(World, Map);
}

void UArcRmlDemoSubsystem::ResumeAfterTravel()
{
	const TArray<FString> Screens = MoveTemp(PendingScreens);
	PendingScreens.Reset();
	bTravelPending = false;

	RemoveCommonMapping();
	AddCommonMapping();
	ResetTo(Screens);
	PreloadDocuments();
	RefreshDevBar();
	const ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	const UWorld* World = LocalPlayer ? LocalPlayer->GetWorld() : nullptr;
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: resumed after travel on '%ls'", World ? *UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) : TEXT("?"));
}

void UArcRmlDemoSubsystem::ShowHubTab(const FString& Document)
{
	const UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (bRunning && Viewport && Viewport->IsDocumentOpen(ArcRmlDemoSubsystemPrivate::HubFrameRef()))
	{
		OpenScreen(ArcRmlDemoSubsystemPrivate::HubScreen, Document);
	}
}

FString UArcRmlDemoSubsystem::CurrentHubTab() const
{
	const UArcRmlViewportSubsystem* Viewport = GetViewport();
	const FArcRmlDocumentRef Shown = Viewport ? Viewport->GetShownTab(ArcRmlDemoSubsystemPrivate::HubFrameRef()) : FArcRmlDocumentRef();
	if (Shown.IsSet())
	{
		return FPaths::GetBaseFilename(Shown.Path);
	}
	const UArcRmlDemoHubViewModel* Hub = FindScreenModel<UArcRmlDemoHubViewModel>();
	if (Hub && Hub->TabDefs.IsValidIndex(Hub->Tab))
	{
		return Hub->TabDefs[Hub->Tab].Document;
	}
	return ArcRmlDemoSubsystemPrivate::FirstHubTab;
}

bool UArcRmlDemoSubsystem::OpenScreen(const FString& Screen, const FString& Param)
{
	using namespace ArcRmlDemoSubsystemPrivate;

	UArcRmlViewportSubsystem* Viewport = GetViewport();
	const FScreenDocument* Entry = FindScreenDocument(Screen);
	if (!Viewport || !Entry)
	{
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo: screen '%ls' has no document", *Screen);
		return false;
	}
	FString Document = Entry->Document;
	FString InitParam = Param;
	if (Screen == HubScreen)
	{
		InitParam = Param.IsEmpty() ? CurrentHubTab() : Param;
		Document = Document / InitParam;
	}
	const FArcRmlDocumentRef Ref{DocumentPath(Document)};
	UE_LOGF(LogArcRmlDemo, Log, "ArcRmlDemo: open '%ls' for screen '%ls'", *Ref.Path, *Screen);
	TWeakObjectPtr<UArcRmlDemoSubsystem> WeakThis(this);
	const bool bOpened = Viewport->OpenDocument(Ref, [WeakThis, Screen, InitParam](const FArcRmlDocumentRef& Opened) -> void
	{
		const UArcRmlDemoSubsystem* Self = WeakThis.Get();
		UArcRmlDemoScreenViewModel* Model = Self && Self->bRunning ? Self->FindScreenModelByKey(Screen) : nullptr;
		if (Model)
		{
			Model->InitScreen(InitParam);
		}
	});
	if (!bOpened)
	{
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo: cannot open '%ls' for screen '%ls'", *Ref.Path, *Screen);
	}
	return bOpened;
}

UArcRmlDemoDialogViewModel* UArcRmlDemoSubsystem::OpenDialog(const FArcRmlDemoDialogSpec& Spec, TFunction<void(int32)> OnChoose)
{
	using namespace ArcRmlDemoSubsystemPrivate;

	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!bRunning || !Viewport)
	{
		return nullptr;
	}
	if (GetDialog())
	{
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo: a dialog is already open; the new one is refused");
		return nullptr;
	}
	if (UArcRmlDemoDialogViewModel* Leaving = Cast<UArcRmlDemoDialogViewModel>(Viewport->GetViewModel(DialogRef(), DialogModelName)))
	{
		Leaving->Closed();
	}
	UArcRmlDemoDialogViewModel* Dialog = NewObject<UArcRmlDemoDialogViewModel>(this);
	if (!Viewport->SetViewModel(DialogRef(), DialogModelName, Dialog))
	{
		return nullptr;
	}
	TWeakObjectPtr<UArcRmlDemoDialogViewModel> WeakDialog(Dialog);
	const bool bOpened = Viewport->OpenDocument(DialogRef(), [WeakDialog, Spec, OnChoose = MoveTemp(OnChoose)](const FArcRmlDocumentRef& Opened) -> void
	{
		if (UArcRmlDemoDialogViewModel* Shown = WeakDialog.Get())
		{
			Shown->Configure(Spec, OnChoose);
		}
	});
	if (!bOpened)
	{
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo: cannot open the dialog document '%ls'", *DialogRef().Path);
		return nullptr;
	}
	return Dialog;
}

void UArcRmlDemoSubsystem::ChooseDialog(UArcRmlDemoDialogViewModel* Dialog, int32 Index)
{
	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!bRunning || !Viewport || !Dialog || Dialog != GetDialog())
	{
		return;
	}
	const TFunction<void(int32)> Callback = Dialog->TakeCallback();
	Viewport->CloseDocument(ArcRmlDemoSubsystemPrivate::DialogRef());
	if (Callback)
	{
		Callback(Index);
	}
}

void UArcRmlDemoSubsystem::DropDialogChoice()
{
	if (UArcRmlDemoDialogViewModel* Dialog = GetDialog())
	{
		Dialog->TakeCallback();
	}
}

void UArcRmlDemoSubsystem::ShowToast(const FString& Tone, const FString& Title, const FString& Body)
{
	if (!bRunning)
	{
		return;
	}
	FArcRmlDemoToast Toast;
	Toast.bVisible = true;
	Toast.Tone = Tone;
	Toast.Title = Title;
	Toast.Body = Body;
	UIModel->SetToast(Toast);
	Later(this, ArcRmlDemoSubsystemPrivate::ToastTimer, ArcRmlDemoSubsystemPrivate::ToastSeconds, [this]() -> void
	{
		UIModel->SetToast(FArcRmlDemoToast());
	});
}

void UArcRmlDemoSubsystem::SetInputScheme(const FString& Scheme)
{
	if (!bRunning)
	{
		return;
	}
	UIModel->SetInput(Scheme);
	if (UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(GetLocalPlayer<ULocalPlayer>()))
	{
		CommonInput->SetCurrentInputType(Scheme == TEXT("gamepad") ? ECommonInputType::Gamepad : ECommonInputType::MouseAndKeyboard);
	}
}

void UArcRmlDemoSubsystem::ToggleMultiplayer()
{
	if (GameModel)
	{
		GameModel->SetMultiplayer(!GameModel->bMultiplayer);
	}
}

void UArcRmlDemoSubsystem::Later(const UObject* Owner, FName Id, double Seconds, TFunction<void()> Fn)
{
	AddTimer(Owner, Id, Seconds, 0.0, [Fn = MoveTemp(Fn)]() -> bool
	{
		Fn();
		return false;
	});
}

void UArcRmlDemoSubsystem::Every(const UObject* Owner, FName Id, double Seconds, TFunction<bool()> Fn)
{
	AddTimer(Owner, Id, Seconds, Seconds, MoveTemp(Fn));
}

void UArcRmlDemoSubsystem::CancelTimer(const UObject* Owner, FName Id)
{
	for (FTimer& Timer : Timers)
	{
		if (Timer.Id == Id && Timer.Owner.Get() == Owner)
		{
			Timer.bDead = true;
		}
	}
}

void UArcRmlDemoSubsystem::CancelTimers(const UObject* Owner)
{
	for (FTimer& Timer : Timers)
	{
		if (Timer.Owner.Get() == Owner)
		{
			Timer.bDead = true;
		}
	}
}

void UArcRmlDemoSubsystem::AddTimer(const UObject* Owner, FName Id, double Seconds, double Interval, TFunction<bool()> Fn)
{
	if (!bRunning || !Owner)
	{
		return;
	}
	CancelTimer(Owner, Id);
	FTimer Timer;
	Timer.Owner = Owner;
	Timer.Id = Id;
	Timer.Serial = ArcRmlDemoSubsystemPrivate::NextTimerSerial++;
	Timer.Due = FPlatformTime::Seconds() + Seconds;
	Timer.Interval = Interval;
	Timer.Fn = MoveTemp(Fn);
	Timers.Add(MoveTemp(Timer));
}

bool UArcRmlDemoSubsystem::HandleTick(float DeltaTime)
{
	if (!bRunning)
	{
		return true;
	}

	const double Now = FPlatformTime::Seconds();
	TArray<uint64> DueSerials;
	for (const FTimer& Timer : Timers)
	{
		if (!Timer.bDead && Timer.Due <= Now)
		{
			DueSerials.Add(Timer.Serial);
		}
	}

	for (const uint64 Serial : DueSerials)
	{
		FTimer* Timer = Timers.FindByPredicate([Serial](const FTimer& Candidate) -> bool { return Candidate.Serial == Serial; });
		if (!Timer || Timer->bDead)
		{
			continue;
		}
		if (!Timer->Owner.IsValid())
		{
			Timer->bDead = true;
			continue;
		}
		const bool bRepeats = Timer->Interval > 0.0;
		const TFunction<bool()> Fn = Timer->Fn;
		if (bRepeats)
		{
			Timer->Due = Now + Timer->Interval;
		}
		else
		{
			Timer->bDead = true;
		}

		const bool bKeep = Fn();
		if (!bRunning)
		{
			return true;
		}
		if (bRepeats && !bKeep)
		{
			if (FTimer* Repeating = Timers.FindByPredicate([Serial](const FTimer& Candidate) -> bool { return Candidate.Serial == Serial; }))
			{
				Repeating->bDead = true;
			}
		}
	}

	Timers.RemoveAll([](const FTimer& Timer) -> bool { return Timer.bDead; });
	return true;
}

void UArcRmlDemoSubsystem::HandleScreenActivated(const FArcRmlDocumentRef& Document)
{
	if (!bRunning)
	{
		return;
	}
	if (UArcRmlDemoScreenViewModel* Screen = FindScreenModelByKey(ArcRmlDemoSubsystemPrivate::ScreenKeyOf(Document.Path)))
	{
		Screen->ScreenActivated();
	}
	RefreshDevBar();
}

void UArcRmlDemoSubsystem::HandleScreenClosed(const FArcRmlDocumentRef& Document, EArcRmlCloseReason Reason)
{
	if (bRunning)
	{
		RefreshDevBar();
	}
}

void UArcRmlDemoSubsystem::RefreshDevBar()
{
	if (UIModel && GameModel)
	{
		UIModel->RefreshDevBar(GetStack(), GameModel->bMultiplayer);
	}
}

void UArcRmlDemoSubsystem::HandleInputMethodChanged(ECommonInputType InputType)
{
	if (UIModel)
	{
		UIModel->SetInput(ArcRmlDemoSubsystemPrivate::InputName(InputType));
	}
}

void UArcRmlDemoSubsystem::HandleGameFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	RefreshDevBar();
}

void UArcRmlDemoSubsystem::CloseDocuments()
{
	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!Viewport)
	{
		return;
	}
	DropDialogChoice();
	for (const FArcRmlDocumentRef& Document : GetOpenDocuments())
	{
		Viewport->CloseDocument(Document);
	}
}

void UArcRmlDemoSubsystem::PreloadDocuments()
{
	using namespace ArcRmlDemoSubsystemPrivate;

	UArcRmlViewportSubsystem* Viewport = GetViewport();
	if (!Viewport)
	{
		return;
	}
	PreloadedDocuments.Reset();
	for (const FScreenDocument& Screen : ScreenDocuments)
	{
		if (FString(Screen.Screen) != HubScreen)
		{
			PreloadedDocuments.AddUnique(DocumentPath(Screen.Document));
			continue;
		}
		PreloadedDocuments.AddUnique(DocumentPath(FString(Screen.Document) / FirstHubTab));
		if (const UArcRmlDemoHubViewModel* Hub = FindScreenModel<UArcRmlDemoHubViewModel>())
		{
			for (const FArcRmlDemoHubTab& Tab : Hub->TabDefs)
			{
				PreloadedDocuments.AddUnique(DocumentPath(FString(Screen.Document) / Tab.Document));
			}
		}
	}
	PreloadedDocuments.AddUnique(DocumentPath(DialogDocument));
	for (const FString& Path : PreloadedDocuments)
	{
		Viewport->PreloadDocument(FArcRmlDocumentRef{Path});
	}
}

void UArcRmlDemoSubsystem::DiscardDocuments()
{
	if (UArcRmlViewportSubsystem* Viewport = GetViewport())
	{
		for (const FString& Path : PreloadedDocuments)
		{
			Viewport->DiscardDocument(FArcRmlDocumentRef{Path});
		}
	}
	PreloadedDocuments.Reset();
}

void UArcRmlDemoSubsystem::AddCommonMapping()
{
	using namespace ArcRmlDemoSubsystemPrivate;

	UCommonInputSettings& Settings = ICommonInputModule::GetSettings();
	Settings.LoadData();
	const UInputAction* ClickAction = Settings.GetEnhancedInputClickAction();
	const UInputAction* BackAction = Settings.GetEnhancedInputBackAction();
	if (!ClickAction || !BackAction)
	{
		if (!bWarnedMissingInputData)
		{
			bWarnedMissingInputData = true;
			UE_LOGF(LogArcRmlDemo, Warning,
				"ArcRmlDemo: the project's CommonUI input data has no Enhanced Input %ls action; set InputData with click and back actions and enable Enhanced Input support in the Common Input settings",
				!ClickAction && !BackAction ? TEXT("click or back") : (!ClickAction ? TEXT("click") : TEXT("back")));
		}
		if (!ClickAction && !BackAction)
		{
			return;
		}
	}

	CommonMapping = NewObject<UInputMappingContext>(this, MakeUniqueObjectName(this, UInputMappingContext::StaticClass(), TEXT("IMC_ArcRmlDemoCommon")), RF_Transient);
	if (ClickAction)
	{
		for (const FKey& Key : {EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom})
		{
			CommonMapping->MapKey(ClickAction, Key);
		}
	}
	if (BackAction)
	{
		for (const FKey& Key : {EKeys::Escape, EKeys::Gamepad_FaceButton_Right})
		{
			CommonMapping->MapKey(BackAction, Key);
		}
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	if (UEnhancedInputLocalPlayerSubsystem* EnhancedInput = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		EnhancedInput->AddMappingContext(CommonMapping, 0);
	}
}

void UArcRmlDemoSubsystem::RemoveCommonMapping()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer<ULocalPlayer>();
	UEnhancedInputLocalPlayerSubsystem* EnhancedInput = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (EnhancedInput && CommonMapping)
	{
		EnhancedInput->RemoveMappingContext(CommonMapping);
	}
	CommonMapping = nullptr;
}
