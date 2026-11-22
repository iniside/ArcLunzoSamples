// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoHudViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoHudViewModel)

void UArcRmlDemoHudViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, bMultiplayer));
	Refresh();
}

void UArcRmlDemoHudViewModel::OpenVendor()
{
	PushFromHud(TEXT("vendor"));
}

void UArcRmlDemoHudViewModel::OpenCrafting()
{
	PushFromHud(TEXT("crafting"));
}

void UArcRmlDemoHudViewModel::EndMatch()
{
	if (bMultiplayer)
	{
		PushFromHud(TEXT("results"));
	}
}

void UArcRmlDemoHudViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

void UArcRmlDemoHudViewModel::Refresh()
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	ARC_RML_DEMO_SET(*this, bMultiplayer, Game && Game->bMultiplayer);
}

void UArcRmlDemoHudViewModel::PushFromHud(const TCHAR* Screen)
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->Push(Screen);
	}
}
