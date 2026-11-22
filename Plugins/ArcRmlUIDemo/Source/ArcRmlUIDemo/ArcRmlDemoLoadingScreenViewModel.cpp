// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoLoadingScreenViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSettings.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoLoadingScreenViewModel)

namespace ArcRmlDemoLoadingScreenViewModelPrivate
{
	const FName ProgressTimer(TEXT("progress"));
}

void UArcRmlDemoLoadingScreenViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, bMultiplayer));
}

void UArcRmlDemoLoadingScreenViewModel::InitScreen(const FString& Param)
{
	Next = Param.IsEmpty() ? FString(TEXT("hud")) : Param;
	ARC_RML_DEMO_SET(*this, Progress, 0);
	SetTip(TipIndex + 1);
	Every(ArcRmlDemoLoadingScreenViewModelPrivate::ProgressTimer, StepSeconds, [this]() -> bool
	{
		ARC_RML_DEMO_SET(*this, Progress, FMath::Min(100, Progress + StepPercent));
		Refresh();
		return Progress < 100;
	});
	Refresh();
}

void UArcRmlDemoLoadingScreenViewModel::Back()
{
	Continue();
}

void UArcRmlDemoLoadingScreenViewModel::Continue()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (Progress >= 100 && Demo)
	{
		Demo->Travel(GetDefault<UArcRmlDemoSettings>()->GameMap, TArray<FString>{Next});
	}
}

void UArcRmlDemoLoadingScreenViewModel::TipStep(int32 Delta)
{
	if (Progress >= 100)
	{
		return;
	}
	SetTip(TipIndex + (Delta > 0 ? 1 : -1));
}

void UArcRmlDemoLoadingScreenViewModel::Selected(const FString& Field, int32 Index)
{
	if (Field == TEXT("tip") && !Tips.IsEmpty())
	{
		SetTip(FMath::Clamp(Index, 0, Tips.Num() - 1));
	}
}

void UArcRmlDemoLoadingScreenViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

void UArcRmlDemoLoadingScreenViewModel::SetTip(int32 Index)
{
	const int32 Count = Tips.Num();
	ARC_RML_DEMO_SET(*this, TipCount, Count);
	ARC_RML_DEMO_SET(*this, TipIndex, Count > 0 ? (Index % Count + Count) % Count : 0);
	ARC_RML_DEMO_SET(*this, Tip, Tips.IsValidIndex(TipIndex) ? Tips[TipIndex] : FString());
}

void UArcRmlDemoLoadingScreenViewModel::Refresh()
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	ARC_RML_DEMO_SET(*this, Chapter, Game && Game->bMultiplayer ? MultiplayerChapter : StoryChapter);
	ARC_RML_DEMO_SET(*this, State, Progress >= 100 ? FString(TEXT("ready")) : FString(TEXT("loading")));
	FString NewStep;
	if (Progress < 40)
	{
		NewStep = FString::Printf(TEXT("Loading world · %d%%"), Progress);
	}
	else if (Progress < 80)
	{
		NewStep = FString::Printf(TEXT("Streaming textures · %.1f / 1.9 GB"), Progress * 0.019);
	}
	else if (Progress < 100)
	{
		NewStep = TEXT("Compiling shaders");
	}
	ARC_RML_DEMO_SET(*this, Step, NewStep);
}
