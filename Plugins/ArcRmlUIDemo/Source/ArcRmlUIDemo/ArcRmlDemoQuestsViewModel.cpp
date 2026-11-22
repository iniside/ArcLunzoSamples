// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoQuestsViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoQuestsViewModel)

void UArcRmlDemoQuestsViewModel::Initialize()
{
	Lists.Reset();
	for (const FArcRmlDemoQuestCategory& Def : Categories)
	{
		Lists.Add(Def.Quests);
	}
	SubTab = FMath::Clamp(SubTab, 0, FMath::Max(0, Categories.Num() - 1));
	Refresh();
}

void UArcRmlDemoQuestsViewModel::Back()
{
	if (bConfirm)
	{
		Choose(0);
		return;
	}
	Super::Back();
}

void UArcRmlDemoQuestsViewModel::Details(int32 Index)
{
	if (bConfirm)
	{
		return;
	}
	SelectQuest(Index);
	Refresh();
}

void UArcRmlDemoQuestsViewModel::Track(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (bConfirm || !Demo)
	{
		return;
	}
	SelectQuest(Index);
	FArcRmlDemoQuest* Quest = FindQuest(Selected);
	if (!Quest || IsDone())
	{
		Refresh();
		return;
	}
	if (!Quest->bTracked && TrackedCount() >= TrackMax)
	{
		Demo->ShowToast(TEXT("warning"), TEXT("Tracking limit reached"), FString::Printf(TEXT("Untrack a quest first (%d max)."), TrackMax));
		Refresh();
		return;
	}
	Quest->bTracked = !Quest->bTracked;
	Refresh();
}

void UArcRmlDemoQuestsViewModel::ShowOnMap(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (bConfirm || !Demo)
	{
		return;
	}
	SelectQuest(Index);
	if (const FArcRmlDemoQuest* Quest = FindQuest(Selected))
	{
		Demo->ShowToast(TEXT("info"), TEXT("Map marker set"), Quest->Region);
	}
	Refresh();
}

void UArcRmlDemoQuestsViewModel::Abandon(int32 Index)
{
	if (bConfirm)
	{
		return;
	}
	SelectQuest(Index);
	if (!FindQuest(Selected) || !CanAbandon())
	{
		Refresh();
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, true);
	Refresh();
}

void UArcRmlDemoQuestsViewModel::Choose(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!bConfirm || !Demo)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	FArcRmlDemoQuest* Quest = FindQuest(Selected);
	if (Index == 1 && Quest)
	{
		const FString Title = Quest->Title;
		TArray<FArcRmlDemoQuest>& List = Lists[SubTab];
		List.RemoveAt(Selected);
		Demo->ShowToast(TEXT("warning"), TEXT("Quest abandoned"), Title);
		ARC_RML_DEMO_SET(*this, Selected, FMath::Max(0, FMath::Min(Selected, List.Num() - 1)));
	}
	Refresh();
}

void UArcRmlDemoQuestsViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (bConfirm || Bar != TEXT("sub") || !Categories.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, SubTab, Index);
	ARC_RML_DEMO_SET(*this, Selected, 0);
	Refresh();
}

bool UArcRmlDemoQuestsViewModel::IsDone() const
{
	return Categories.IsValidIndex(SubTab) && Categories[SubTab].Key == TEXT("completed");
}

bool UArcRmlDemoQuestsViewModel::CanAbandon() const
{
	return Categories.IsValidIndex(SubTab) && !IsDone() && Categories[SubTab].Key != TEXT("main");
}

int32 UArcRmlDemoQuestsViewModel::TrackedCount() const
{
	int32 Count = 0;
	for (const TArray<FArcRmlDemoQuest>& List : Lists)
	{
		for (const FArcRmlDemoQuest& Quest : List)
		{
			Count += Quest.bTracked ? 1 : 0;
		}
	}
	return Count;
}

FArcRmlDemoQuest* UArcRmlDemoQuestsViewModel::FindQuest(int32 Index)
{
	return Lists.IsValidIndex(SubTab) && Lists[SubTab].IsValidIndex(Index) ? &Lists[SubTab][Index] : nullptr;
}

void UArcRmlDemoQuestsViewModel::SelectQuest(int32 Index)
{
	if (FindQuest(Index))
	{
		ARC_RML_DEMO_SET(*this, Selected, Index);
	}
}

void UArcRmlDemoQuestsViewModel::Refresh()
{
	if (!Categories.IsValidIndex(SubTab) || !Lists.IsValidIndex(SubTab))
	{
		return;
	}
	const FArcRmlDemoQuestCategory& Def = Categories[SubTab];
	const TArray<FArcRmlDemoQuest>& List = Lists[SubTab];
	const bool bCompleted = IsDone();
	const int32 Shown = List.IsEmpty() ? 0 : FMath::Clamp(Selected, 0, List.Num() - 1);

	TArray<FArcRmlDemoTabRow> TabRows;
	for (int32 Index = 0; Index < Categories.Num(); ++Index)
	{
		FArcRmlDemoTabRow& Row = TabRows.AddDefaulted_GetRef();
		Row.Label = Categories[Index].Label;
		const int32 Count = Categories[Index].Key == TEXT("completed") || !Lists.IsValidIndex(Index) ? 0 : Lists[Index].Num();
		Row.Badge = Count > 0 ? FString::FromInt(Count) : FString();
	}

	TArray<FArcRmlDemoQuestRow> Rows;
	for (const FArcRmlDemoQuest& Quest : List)
	{
		FArcRmlDemoQuestRow& Row = Rows.AddDefaulted_GetRef();
		Row.Title = Quest.Title;
		Row.Kind = bCompleted ? FString(TEXT("side")) : Def.Key;
		Row.Level = Quest.Level;
		Row.Region = Quest.Region;
		Row.Progress = Quest.Progress;
		Row.State = Quest.State.IsEmpty() ? FString(TEXT("active")) : Quest.State;
		Row.bTracked = Quest.bTracked;
	}

	const FArcRmlDemoQuest* Quest = List.IsValidIndex(Shown) ? &List[Shown] : nullptr;
	const bool bTracked = Quest && Quest->bTracked;

	ARC_RML_DEMO_SET(*this, Tabs, TabRows);
	ARC_RML_DEMO_SET(*this, Quests, Rows);
	ARC_RML_DEMO_SET(*this, Selected, Shown);
	ARC_RML_DEMO_SET(*this, DetailTitle, Quest ? Quest->Title : FString());
	ARC_RML_DEMO_SET(*this, DetailSub, Quest ? FString::Printf(TEXT("%s · Lv %d · %s"), *Def.KindLabel, Quest->Level, *Quest->Region) : FString());
	ARC_RML_DEMO_SET(*this, DetailIcon, Def.Icon);
	ARC_RML_DEMO_SET(*this, DetailBody, Def.Body);
	ARC_RML_DEMO_SET(*this, Objectives, Def.Objectives);
	ARC_RML_DEMO_SET(*this, bDone, bCompleted);
	ARC_RML_DEMO_SET(*this, bCanAbandon, CanAbandon());
	ARC_RML_DEMO_SET(*this, bHasQuest, Quest != nullptr);
	ARC_RML_DEMO_SET(*this, bCanTrack, Quest && !bCompleted && (bTracked || TrackedCount() < TrackMax));
	ARC_RML_DEMO_SET(*this, TrackLabel, bTracked ? FString(TEXT("Untrack")) : FString(TEXT("Track")));
	ARC_RML_DEMO_SET(*this, TrackVariant, bTracked ? FString(TEXT("secondary")) : FString(TEXT("primary")));
}
