// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoMatchResultsViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSettings.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoMatchResultsViewModel)

namespace ArcRmlDemoMatchResultsViewModelPrivate
{
	int32 ScoreOf(const FArcRmlDemoScoreRow& Row)
	{
		return Row.Cols.IsEmpty() ? 0 : FCString::Atoi(*Row.Cols[0]);
	}
}

void UArcRmlDemoMatchResultsViewModel::Initialize()
{
	TArray<FArcRmlDemoTeamRow> Ranked = Teams;
	for (FArcRmlDemoTeamRow& Team : Ranked)
	{
		Team.Rows.StableSort([](const FArcRmlDemoScoreRow& A, const FArcRmlDemoScoreRow& B) -> bool
		{
			return ArcRmlDemoMatchResultsViewModelPrivate::ScoreOf(A) > ArcRmlDemoMatchResultsViewModelPrivate::ScoreOf(B);
		});
		for (int32 Index = 0; Index < Team.Rows.Num(); ++Index)
		{
			Team.Rows[Index].Rank = Index + 1;
		}
	}
	Teams = Ranked;
	NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Teams));
}

void UArcRmlDemoMatchResultsViewModel::Back()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo)
	{
		return;
	}
	Demo->Travel(GetDefault<UArcRmlDemoSettings>()->MenuMap, TArray<FString>{TEXT("main")});
	if (UArcRmlDemoGameViewModel* Game = GetGame())
	{
		Game->SetMultiplayer(false);
	}
}

void UArcRmlDemoMatchResultsViewModel::PlayAgain()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ResetTo(TArray<FString>{TEXT("loading")}, TEXT("hud"));
	}
}

void UArcRmlDemoMatchResultsViewModel::Continue()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->Travel(GetDefault<UArcRmlDemoSettings>()->MenuMap, TArray<FString>{TEXT("main"), TEXT("lobby")});
	}
}
