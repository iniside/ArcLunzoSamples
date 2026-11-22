// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoMainMenuViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoMainMenuViewModel)

void UArcRmlDemoMainMenuViewModel::Activate(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo || !Items.IsValidIndex(Index) || Items[Index].bDisabled)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Selected, Index);
	const FName Id = Items[Index].Id;
	if (Id == TEXT("continue") || Id == TEXT("new"))
	{
		if (UArcRmlDemoGameViewModel* Game = GetGame())
		{
			Game->SetMultiplayer(false);
		}
		Demo->Push(TEXT("loading"), TEXT("hud"));
	}
	else if (Id == TEXT("load"))
	{
		Demo->Push(TEXT("saveload"), TEXT("load"));
	}
	else if (Id == TEXT("multiplayer"))
	{
		Demo->Push(TEXT("lobby"));
	}
	else if (Id == TEXT("settings"))
	{
		Demo->Push(TEXT("settings"));
	}
	else if (Id == TEXT("credits"))
	{
		Demo->Push(TEXT("credits"));
	}
	else if (Id == TEXT("quit"))
	{
		TWeakObjectPtr<UArcRmlDemoMainMenuViewModel> WeakThis(this);
		Demo->OpenDialog(QuitDialog, [WeakThis](int32 Choice) -> void
		{
			UArcRmlDemoMainMenuViewModel* Self = WeakThis.Get();
			UArcRmlDemoSubsystem* Owner = Self ? Self->GetDemo() : nullptr;
			if (!Owner || Choice != 1)
			{
				return;
			}
			if (UArcRmlDemoGameViewModel* Game = Self->GetGame())
			{
				Game->SetMultiplayer(false);
			}
			Owner->ResetTo(TArray<FString>{TEXT("splash")});
		});
	}
}

void UArcRmlDemoMainMenuViewModel::OpenNews()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ShowToast(NewsTone, NewsTitle, NewsBody);
	}
}
