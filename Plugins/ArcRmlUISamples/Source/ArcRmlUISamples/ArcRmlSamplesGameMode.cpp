// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlSamplesGameMode.h"

#include "Engine/LocalPlayer.h"
#include "Menu/ArcRmlSamplesBackButton.h"
#include "Menu/ArcRmlSamplesMenu.h"
#include "SourcePath/ArcRmlSamplesSourcePath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesGameMode)

AArcRmlSamplesPlayerController::AArcRmlSamplesPlayerController()
{
	bShowMouseCursor = true;
	SampleViewModel = CreateDefaultSubobject<UArcRmlSamplesSourcePathViewModel>(TEXT("SampleViewModel"));
	SampleViewModel->Label = TEXT("From the player controller");
}

void AArcRmlSamplesPlayerController::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	if (IsLocalController())
	{
		ArcRmlSamplesMenu::Open(GetLocalPlayer());
		BackButtonPlayer = GetLocalPlayer();
		BackButton = ArcRmlSamplesBackButton::Add(GetLocalPlayer());
	}
#endif
}

void AArcRmlSamplesPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING
	// Player viewport widgets are cleared only when the viewport client goes (GameViewportClient.cpp:472,480), not on a map change.
	ArcRmlSamplesBackButton::Remove(BackButtonPlayer.Get(), BackButton);
	BackButton.Reset();
	BackButtonPlayer.Reset();
#endif
	Super::EndPlay(EndPlayReason);
}

AArcRmlSamplesGameMode::AArcRmlSamplesGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AArcRmlSamplesPlayerController::StaticClass();
}
