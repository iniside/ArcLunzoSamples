// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoAchievementsViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoAchievementsViewModel)

namespace ArcRmlDemoAchievementsViewModelPrivate
{
	FString TierOf(float Pct)
	{
		return Pct < 1.0f ? TEXT("legendary") : Pct < 5.0f ? TEXT("epic") : Pct < 15.0f ? TEXT("rare") : Pct < 40.0f ? TEXT("uncommon") : TEXT("common");
	}
}

UArcRmlDemoAchievementsViewModel::UArcRmlDemoAchievementsViewModel()
{
	Achievements.SetRowType(FArcRmlDemoAchievementRow::StaticStruct());
}

void UArcRmlDemoAchievementsViewModel::Initialize()
{
	Refresh();
}

void UArcRmlDemoAchievementsViewModel::Details(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const TArray<int32> List = Shown();
	if (!Demo || !List.IsValidIndex(Index))
	{
		return;
	}
	const FArcRmlDemoAchievement& Achievement = AllAchievements[List[Index]];
	const bool bHidden = Achievement.State == TEXT("hidden");
	Demo->ShowToast(Achievement.State == TEXT("unlocked") ? TEXT("success") : TEXT("info"), bHidden ? FString(TEXT("Hidden achievement")) : Achievement.Title,
		bHidden ? FString(TEXT("Keep playing to reveal it.")) : Achievement.Description + FString::Printf(TEXT(" · %.1f%% of players"), Achievement.Pct));
}

void UArcRmlDemoAchievementsViewModel::Pin(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const TArray<int32> List = Shown();
	if (!Demo || !List.IsValidIndex(Index))
	{
		return;
	}
	const FArcRmlDemoAchievement& Achievement = AllAchievements[List[Index]];
	if (Achievement.State == TEXT("unlocked") || Achievement.State == TEXT("hidden"))
	{
		return;
	}
	const bool bPin = !Pinned.Contains(Achievement.Title);
	if (bPin)
	{
		Pinned.Add(Achievement.Title);
	}
	else
	{
		Pinned.Remove(Achievement.Title);
	}
	Demo->ShowToast(TEXT("info"), (bPin ? TEXT("Pinned ") : TEXT("Unpinned ")) + Achievement.Title, bPin ? FString(TEXT("Progress shows on the HUD.")) : FString());
}

void UArcRmlDemoAchievementsViewModel::CycleSort()
{
	if (SortOptions.IsEmpty())
	{
		return;
	}
	SortIndex = (SortIndex + 1) % SortOptions.Num();
	Refresh();
}

void UArcRmlDemoAchievementsViewModel::SelectCategory(int32 Index)
{
	if (Categories.IsValidIndex(Index))
	{
		ARC_RML_DEMO_SET(*this, Category, Index);
	}
}

void UArcRmlDemoAchievementsViewModel::FilterBy(int32 Index)
{
	if (Filters.IsValidIndex(Index))
	{
		FilterIndex = Index;
		Refresh();
	}
}

bool UArcRmlDemoAchievementsViewModel::Matches(const FArcRmlDemoAchievementFilter& Filter, const FArcRmlDemoAchievement& Achievement) const
{
	return Filter.States.IsEmpty() || Filter.States.Contains(Achievement.State);
}

TArray<int32> UArcRmlDemoAchievementsViewModel::Shown() const
{
	TArray<int32> Result;
	if (!Filters.IsValidIndex(FilterIndex))
	{
		return Result;
	}
	for (int32 Index = 0; Index < AllAchievements.Num(); ++Index)
	{
		if (Matches(Filters[FilterIndex], AllAchievements[Index]))
		{
			Result.Add(Index);
		}
	}
	const FString SortKey = SortOptions.IsValidIndex(SortIndex) ? SortOptions[SortIndex].Key : FString();
	const TArray<FArcRmlDemoAchievement>& All = AllAchievements;
	Result.StableSort([&SortKey, &All](int32 IndexA, int32 IndexB) -> bool
	{
		const FArcRmlDemoAchievement& A = All[IndexA];
		const FArcRmlDemoAchievement& B = All[IndexB];
		if (SortKey == TEXT("name"))
		{
			return A.Title.Compare(B.Title, ESearchCase::IgnoreCase) < 0;
		}
		if (SortKey == TEXT("points"))
		{
			return A.Points != B.Points ? A.Points > B.Points : A.Pct < B.Pct;
		}
		return A.Pct < B.Pct;
	});
	return Result;
}

void UArcRmlDemoAchievementsViewModel::Refresh()
{
	using namespace ArcRmlDemoAchievementsViewModelPrivate;

	const TArray<int32> List = Shown();
	TArray<FString> Keys;
	TArray<FArcRmlDemoAchievementRow> Rows;
	for (const int32 Index : List)
	{
		const FArcRmlDemoAchievement& Achievement = AllAchievements[Index];
		FArcRmlDemoAchievementRow& Row = Rows.AddDefaulted_GetRef();
		Row.Icon = Achievement.Icon;
		Row.Title = Achievement.Title;
		Row.Description = Achievement.Description;
		Row.State = Achievement.State;
		Row.Value = Achievement.Value;
		Row.Max = Achievement.Max;
		Row.Date = Achievement.Date;
		Row.RarityLabel = FString::Printf(TEXT("%.1f%% of players"), Achievement.Pct);
		Row.Pct = Achievement.Pct;
		Row.Tier = TierOf(Achievement.Pct);
		Row.Points = FString::FromInt(Achievement.Points);
		Keys.Add(Achievement.Title);
	}
	SetList(Achievements, GET_MEMBER_NAME_CHECKED(ThisClass, Achievements), Keys, Rows);

	TArray<FArcRmlDemoChipRow> ChipRows;
	for (int32 Index = 0; Index < Filters.Num(); ++Index)
	{
		const FArcRmlDemoAchievementFilter& Def = Filters[Index];
		FArcRmlDemoChipRow& Chip = ChipRows.AddDefaulted_GetRef();
		Chip.Label = Def.Label;
		Chip.Icon = Def.Icon;
		Chip.Count = AllAchievements.FilterByPredicate([this, &Def](const FArcRmlDemoAchievement& Achievement) -> bool { return Matches(Def, Achievement); }).Num();
		Chip.bActive = Index == FilterIndex;
	}

	const FArcRmlDemoAchievementSort* Current = SortOptions.IsValidIndex(SortIndex) ? &SortOptions[SortIndex] : nullptr;
	const FArcRmlDemoAchievementSort* Next = SortOptions.IsEmpty() ? nullptr : &SortOptions[(SortIndex + 1) % SortOptions.Num()];
	ARC_RML_DEMO_SET(*this, Chips, ChipRows);
	ARC_RML_DEMO_SET(*this, SortLabel, Current ? Current->Label : FString());
	ARC_RML_DEMO_SET(*this, NextSortLabel, Next ? Next->Label : FString());
	ARC_RML_DEMO_SET(*this, ShownSub, FString::Printf(TEXT("%d shown · sorted by %s"), List.Num(), Current ? *Current->SortedBy : TEXT("")));
}
