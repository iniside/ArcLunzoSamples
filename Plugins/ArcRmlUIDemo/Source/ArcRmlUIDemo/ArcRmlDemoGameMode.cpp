// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoGameMode.h"

#include "ArcRmlDemoLog.h"
#include "ArcRmlDemoSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoGameMode)

AArcRmlDemoPlayerController::AArcRmlDemoPlayerController()
{
	bShowMouseCursor = true;
}

AArcRmlDemoGameMode::AArcRmlDemoGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AArcRmlDemoPlayerController::StaticClass();
}

UArcRmlDemoSubsystem* AArcRmlDemoGameMode::FindDemo() const
{
	UGameInstance* GameInstance = GetGameInstance();
	ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlDemoSubsystem>() : nullptr;
}

void AArcRmlDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	UArcRmlDemoSubsystem* Demo = FindDemo();
	if (!Demo)
	{
		UE_LOGF(LogArcRmlDemo, Warning, "ArcRmlDemo: the demo game mode found no local player to start the demo for");
		return;
	}
	const bool bResume = Demo->IsRunning() && Demo->IsTravelPending();
	Demo->Start();
	if (!bResume && !StartScreen.IsEmpty())
	{
		Demo->Jump(StartScreen);
	}
}

void AArcRmlDemoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UArcRmlDemoSubsystem* Demo = FindDemo();
	if (Demo && !(EndPlayReason == EEndPlayReason::LevelTransition && Demo->IsTravelPending()))
	{
		Demo->Stop();
	}
	Super::EndPlay(EndPlayReason);
}
