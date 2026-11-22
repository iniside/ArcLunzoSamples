// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoLobbyViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoLobbyViewModel)

namespace ArcRmlDemoLobbyViewModelPrivate
{
	const FString SearchField(TEXT("search"));
	const FName ReadyTimer(TEXT("ready"));
	const FName RefreshTimer(TEXT("refresh"));

	FArcRmlDemoFactRow Fact(const TCHAR* Label, const FString& Value)
	{
		FArcRmlDemoFactRow Row;
		Row.Label = Label;
		Row.Value = Value;
		return Row;
	}
}

void UArcRmlDemoLobbyViewModel::InitScreen(const FString& Param)
{
	CancelTimer(ArcRmlDemoLobbyViewModelPrivate::ReadyTimer);
	CancelTimer(ArcRmlDemoLobbyViewModelPrivate::RefreshTimer);
	SetState(TEXT("browse"));
}

void UArcRmlDemoLobbyViewModel::Back()
{
	if (State == TEXT("searching") || State == TEXT("ready"))
	{
		CancelTimer(ArcRmlDemoLobbyViewModelPrivate::ReadyTimer);
		CancelTimer(ArcRmlDemoLobbyViewModelPrivate::RefreshTimer);
		SetState(TEXT("browse"));
		return;
	}
	Leave();
}

void UArcRmlDemoLobbyViewModel::SelectServer(int32 Index)
{
	if (State == TEXT("searching"))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, SelectedServer, Index);
	Refresh();
}

void UArcRmlDemoLobbyViewModel::ToggleFavourite()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (State == TEXT("searching") || !Demo)
	{
		return;
	}
	const TArray<int32> Visible = VisibleServers();
	const FString Label = Visible.IsValidIndex(SelectedServer) ? ServerList[Visible[SelectedServer]].Label : FString();
	Demo->ShowToast(TEXT("info"), TEXT("Favourite toggled"), Label);
}

void UArcRmlDemoLobbyViewModel::RefreshServers()
{
	if (State == TEXT("searching"))
	{
		return;
	}
	SetState(TEXT("searching"));
	Later(ArcRmlDemoLobbyViewModelPrivate::RefreshTimer, RefreshSeconds, [this]() -> void
	{
		SetState(TEXT("browse"));
	});
}

void UArcRmlDemoLobbyViewModel::ToggleHideFull()
{
	if (State == TEXT("searching"))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bHideFull, !bHideFull);
	ARC_RML_DEMO_SET(*this, SelectedServer, 0);
	Refresh();
}

void UArcRmlDemoLobbyViewModel::ClearFilters()
{
	if (State == TEXT("searching"))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Query, FString());
	ARC_RML_DEMO_SET(*this, bHideFull, false);
	Refresh();
}

void UArcRmlDemoLobbyViewModel::ToggleReady()
{
	using namespace ArcRmlDemoLobbyViewModelPrivate;

	if (State == TEXT("searching"))
	{
		return;
	}
	if (State == TEXT("ready"))
	{
		CancelTimer(ReadyTimer);
		SetState(TEXT("browse"));
		return;
	}
	SetState(TEXT("ready"));
	Later(ReadyTimer, ReadySeconds, [this]() -> void
	{
		if (State != TEXT("ready"))
		{
			return;
		}
		SetState(TEXT("searching"));
		Later(ArcRmlDemoLobbyViewModelPrivate::ReadyTimer, MatchSeconds, [this]() -> void
		{
			UArcRmlDemoSubsystem* Demo = GetDemo();
			if (!Demo || Demo->GetTop() != GetScreenKey() || State != TEXT("searching"))
			{
				return;
			}
			if (UArcRmlDemoGameViewModel* Game = GetGame())
			{
				Game->SetMultiplayer(true);
			}
			SetState(TEXT("browse"));
			Demo->Push(TEXT("loading"), TEXT("hud"));
		});
	});
}

void UArcRmlDemoLobbyViewModel::Leave()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ResetTo(TArray<FString>{TEXT("main")});
	}
}

void UArcRmlDemoLobbyViewModel::Selected(const FString& Field, int32 Index)
{
	if (Field == TEXT("crumbs"))
	{
		Back();
	}
	else if (Field == TEXT("pager"))
	{
		ARC_RML_DEMO_SET(*this, Page, FMath::Clamp(Index, 0, FMath::Max(0, PageCount - 1)));
	}
}

void UArcRmlDemoLobbyViewModel::WriteText(const FString& Field, const FString& Value)
{
	if (Field != ArcRmlDemoLobbyViewModelPrivate::SearchField)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Query, Value);
	ARC_RML_DEMO_SET(*this, SelectedServer, 0);
	Refresh();
}

TArray<int32> UArcRmlDemoLobbyViewModel::VisibleServers() const
{
	TArray<int32> Result;
	for (int32 Index = 0; Index < ServerList.Num(); ++Index)
	{
		const FArcRmlDemoServerRow& Server = ServerList[Index];
		if (bHideFull && Server.Players >= Server.MaxPlayers)
		{
			continue;
		}
		if (!Query.IsEmpty() && !Server.Label.Contains(Query, ESearchCase::IgnoreCase))
		{
			continue;
		}
		Result.Add(Index);
	}
	return Result;
}

void UArcRmlDemoLobbyViewModel::SetState(const FString& InState)
{
	ARC_RML_DEMO_SET(*this, State, InState);
	Refresh();
}

void UArcRmlDemoLobbyViewModel::Refresh()
{
	using namespace ArcRmlDemoLobbyViewModelPrivate;

	const TArray<int32> Visible = VisibleServers();
	const bool bNowSearching = State == TEXT("searching");
	const bool bNowEmpty = Visible.IsEmpty() && !bNowSearching;
	const bool bNowReady = !Visible.IsEmpty() && State == TEXT("ready");
	const int32 Shown = FMath::Min(SelectedServer, FMath::Max(0, Visible.Num() - 1));

	TArray<FArcRmlDemoServerRow> Rows;
	if (!bNowEmpty && !bNowSearching)
	{
		for (const int32 Index : Visible)
		{
			Rows.Add(ServerList[Index]);
		}
	}

	TArray<FArcRmlDemoPlayerRow> Members = PartyMembers;
	if (!Members.IsEmpty())
	{
		Members[0].Status = State == TEXT("browse") ? TEXT("not-ready") : TEXT("ready");
	}

	TArray<FArcRmlDemoFactRow> Stats;
	if (bNowEmpty || bNowSearching)
	{
		Stats.Add(Fact(TEXT("Server"), TEXT("—")));
	}
	else if (Visible.IsValidIndex(Shown))
	{
		const FArcRmlDemoServerRow& Server = ServerList[Visible[Shown]];
		Stats = {Fact(TEXT("Server"), Server.Label), Fact(TEXT("Mode"), Server.Mode), Fact(TEXT("Map"), Server.Map),
			Fact(TEXT("Ping"), FString::Printf(TEXT("%d ms"), Server.Ping))};
	}

	ARC_RML_DEMO_SET(*this, Servers, Rows);
	ARC_RML_DEMO_SET(*this, Party, Members);
	ARC_RML_DEMO_SET(*this, SelectedServer, Shown);
	ARC_RML_DEMO_SET(*this, bEmpty, bNowEmpty);
	ARC_RML_DEMO_SET(*this, bSearching, bNowSearching);
	ARC_RML_DEMO_SET(*this, bReady, bNowReady);
	ARC_RML_DEMO_SET(*this, Showing, bNowEmpty || bNowSearching ? FString(TEXT("0 results")) : FString(TEXT("Showing 1–8 of 128")));
	ARC_RML_DEMO_SET(*this, PartySub, bNowReady ? FString(TEXT("3 / 4 · all set")) : FString(TEXT("3 / 4 · 1 not ready")));
	ARC_RML_DEMO_SET(*this, ServerStats, Stats);
	ARC_RML_DEMO_SET(*this, ReadyLabel, bNowReady ? FString(TEXT("Cancel ready")) : FString(TEXT("Ready")));
	ARC_RML_DEMO_SET(*this, ReadyVariant, bNowReady ? FString(TEXT("secondary")) : FString(TEXT("primary")));
}
