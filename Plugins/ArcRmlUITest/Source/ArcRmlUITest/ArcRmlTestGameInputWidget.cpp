// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlTestGameInputWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogArcRmlTestWidget, Log, All);

namespace ArcRmlTestGameInputWidgetPrivate
{
	void HandlePushCommand(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		if (!PlayerController)
		{
			UE_LOGF(LogArcRmlTestWidget, Warning, "ArcRmlTest.PushGameInputWidget: no player controller in this world");
			return;
		}
		UArcRmlTestGameInputWidget* Widget = CreateWidget<UArcRmlTestGameInputWidget>(PlayerController, UArcRmlTestGameInputWidget::StaticClass());
		Widget->AddToViewport(50);
		Widget->ActivateWidget();
		UE_LOGF(LogArcRmlTestWidget, Display, "ArcRmlTest.PushGameInputWidget: '%ls' pushed, activated %ls", *Widget->GetName(), Widget->IsActivated() ? TEXT("true") : TEXT("false"));
	}

	void HandlePopCommand(const TArray<FString>& Args, UWorld* World)
	{
		for (TObjectIterator<UArcRmlTestGameInputWidget> It; It; ++It)
		{
			if (It->HasAnyFlags(RF_ClassDefaultObject) || It->GetWorld() != World)
			{
				continue;
			}
			It->DeactivateWidget();
			It->RemoveFromParent();
			UE_LOGF(LogArcRmlTestWidget, Display, "ArcRmlTest.PopGameInputWidget: '%ls' removed", *It->GetName());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs PushCommand(TEXT("ArcRmlTest.PushGameInputWidget"),
		TEXT("Pushes a native CommonUI activatable widget that asks for the Game input config onto the game viewport and activates it"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePushCommand));

	FAutoConsoleCommandWithWorldAndArgs PopCommand(TEXT("ArcRmlTest.PopGameInputWidget"),
		TEXT("Deactivates and removes the widgets ArcRmlTest.PushGameInputWidget pushed"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HandlePopCommand));
}

TOptional<FUIInputConfig> UArcRmlTestGameInputWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}
