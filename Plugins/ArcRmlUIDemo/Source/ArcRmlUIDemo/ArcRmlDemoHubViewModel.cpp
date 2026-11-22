// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoHubViewModel.h"

#include "ArcRmlDemoCharacterViewModel.h"
#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSocialViewModel.h"
#include "ArcRmlDemoSubsystem.h"
#include "ArcRmlDemoTalentsViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoHubViewModel)

void UArcRmlDemoHubViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Gold));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Shards));
	Refresh();
}

void UArcRmlDemoHubViewModel::InitScreen(const FString& Param)
{
	const int32 Index = TabDefs.IndexOfByPredicate([&Param](const FArcRmlDemoHubTab& Def) -> bool { return Def.Document == Param; });
	if (Index != INDEX_NONE)
	{
		ARC_RML_DEMO_SET(*this, Tab, Index);
	}
	Refresh();
}

void UArcRmlDemoHubViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (Bar == TEXT("main") && TabDefs.IsValidIndex(Index))
	{
		SelectTab(TabDefs[Index].Document);
	}
}

void UArcRmlDemoHubViewModel::SelectTab(const FString& Document)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ShowHubTab(Document);
	}
}

void UArcRmlDemoHubViewModel::Refresh()
{
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Demo || !Game)
	{
		return;
	}
	const UArcRmlDemoCharacterViewModel* Character = Demo->FindScreenModel<UArcRmlDemoCharacterViewModel>();
	const UArcRmlDemoTalentsViewModel* Talents = Demo->FindScreenModel<UArcRmlDemoTalentsViewModel>();
	const UArcRmlDemoSocialViewModel* Social = Demo->FindScreenModel<UArcRmlDemoSocialViewModel>();

	TArray<FArcRmlDemoTabRow> Rows;
	for (const FArcRmlDemoHubTab& Def : TabDefs)
	{
		const int32 Badge = Def.Document == TEXT("Character") ? (Character ? Character->BadgeCount() : 0)
			: Def.Document == TEXT("TalentTree") ? (Talents ? Talents->BadgeCount() : 0)
			: Def.Document == TEXT("Friends") ? (Social ? Social->BadgeCount() : 0)
			: 0;
		FArcRmlDemoTabRow& Row = Rows.AddDefaulted_GetRef();
		Row.Label = Def.Label;
		Row.Badge = Badge > 0 ? FString::FromInt(Badge) : FString();
	}

	TArray<FArcRmlDemoCurrencyRow> Purse;
	FArcRmlDemoCurrencyRow& GoldRow = Purse.Add_GetRef(Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Game->Gold)));
	GoldRow.Label = TEXT("Gold");
	FArcRmlDemoCurrencyRow& ShardRow = Purse.Add_GetRef(Game->ShardsRow(UArcRmlDemoGameViewModel::FormatNumber(Game->Shards)));
	ShardRow.Label = TEXT("Tide shards");
	ShardRow.Rarity = TEXT("epic");

	ARC_RML_DEMO_SET(*this, Tabs, Rows);
	ARC_RML_DEMO_SET(*this, Currency, Purse);
}

void UArcRmlDemoHubViewModel::RefreshFor(UArcRmlDemoSubsystem* Demo)
{
	if (UArcRmlDemoHubViewModel* Hub = Demo ? Demo->FindScreenModel<UArcRmlDemoHubViewModel>() : nullptr)
	{
		Hub->Refresh();
	}
}

void UArcRmlDemoHubViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

