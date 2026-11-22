// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoPauseMenuViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSettings.h"
#include "ArcRmlDemoSubsystem.h"
#include "ArcRmlDemoUIViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoPauseMenuViewModel)

namespace ArcRmlDemoPauseMenuViewModelPrivate
{
	const FName SaveTimer(TEXT("save"));
}

void UArcRmlDemoPauseMenuViewModel::Initialize()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		if (UArcRmlDemoUIViewModel* UI = Demo->GetUI())
		{
			UI->WatchField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoUIViewModel, Toast), FFieldValueChangedDelegate::CreateUObject(this, &ThisClass::HandleUIFieldChanged));
		}
	}
	Refresh();
}

void UArcRmlDemoPauseMenuViewModel::Shutdown()
{
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		if (UArcRmlDemoUIViewModel* UI = Demo->GetUI())
		{
			UI->RemoveAllFieldValueChangedDelegates(this);
		}
	}
	Super::Shutdown();
}

void UArcRmlDemoPauseMenuViewModel::ScreenActivated()
{
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game || Game->SaveNotice.IsEmpty())
	{
		return;
	}
	SavedSlot = Game->SaveNotice;
	Game->SetSaveNotice(FString());
	SetState(TEXT("saving"));
	Later(ArcRmlDemoPauseMenuViewModelPrivate::SaveTimer, SavingSeconds, [this]() -> void
	{
		SetState(TEXT("saved"));
		Later(ArcRmlDemoPauseMenuViewModelPrivate::SaveTimer, SavedSeconds, [this]() -> void
		{
			SetState(TEXT("default"));
		});
	});
}

void UArcRmlDemoPauseMenuViewModel::Activate(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo || !Items.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Selected, Index);
	if (Items[Index].bDisabled)
	{
		return;
	}
	const FName Id = Items[Index].Id;
	if (Id == TEXT("resume"))
	{
		Demo->Pop();
	}
	else if (Id == TEXT("hub"))
	{
		Demo->ReplaceTop(TEXT("hub"));
	}
	else if (Id == TEXT("settings"))
	{
		Demo->Push(TEXT("settings"));
	}
	else if (Id == TEXT("save") || Id == TEXT("load"))
	{
		Demo->Push(TEXT("saveload"), Id.ToString());
	}
	else if (Id == TEXT("quitMenu"))
	{
		TWeakObjectPtr<UArcRmlDemoPauseMenuViewModel> WeakThis(this);
		Demo->OpenDialog(QuitMenuDialog, [WeakThis](int32 Choice) -> void
		{
			UArcRmlDemoPauseMenuViewModel* Self = WeakThis.Get();
			UArcRmlDemoSubsystem* Owner = Self ? Self->GetDemo() : nullptr;
			if (!Owner || Choice == 0)
			{
				return;
			}
			if (Choice == 1)
			{
				Owner->ShowToast(Self->QuitSavedToast.Tone, Self->QuitSavedToast.Title, Self->QuitSavedToast.Body);
			}
			if (UArcRmlDemoGameViewModel* Game = Self->GetGame())
			{
				Game->SetMultiplayer(false);
			}
			Owner->Travel(GetDefault<UArcRmlDemoSettings>()->MenuMap, TArray<FString>{TEXT("main")});
		});
	}
	else if (Id == TEXT("quit"))
	{
		TWeakObjectPtr<UArcRmlDemoPauseMenuViewModel> WeakThis(this);
		Demo->OpenDialog(QuitDialog, [WeakThis](int32 Choice) -> void
		{
			UArcRmlDemoPauseMenuViewModel* Self = WeakThis.Get();
			UArcRmlDemoSubsystem* Owner = Self ? Self->GetDemo() : nullptr;
			if (!Owner || Choice != 1)
			{
				return;
			}
			if (UArcRmlDemoGameViewModel* Game = Self->GetGame())
			{
				Game->SetMultiplayer(false);
			}
			Owner->Travel(GetDefault<UArcRmlDemoSettings>()->MenuMap, TArray<FString>{TEXT("splash")});
		});
	}
}

void UArcRmlDemoPauseMenuViewModel::HandleUIFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	Refresh();
}

void UArcRmlDemoPauseMenuViewModel::SetState(const FString& InState)
{
	State = InState;
	Refresh();
}

void UArcRmlDemoPauseMenuViewModel::Refresh()
{
	const bool bSaving = State == TEXT("saving");
	TArray<FArcRmlDemoMenuItemRow> Rows = MenuItems;
	for (FArcRmlDemoMenuItemRow& Row : Rows)
	{
		if (bSaving && Row.Id == TEXT("save"))
		{
			Row.bDisabled = true;
			Row.Description = TEXT("Saving…");
		}
	}
	ARC_RML_DEMO_SET(*this, Items, Rows);

	FArcRmlDemoToast Shown;
	if (bSaving)
	{
		Shown = SavingToast;
	}
	else if (State == TEXT("saved"))
	{
		Shown = SavedToast;
		Shown.Body = (SavedSlot.IsEmpty() ? FString(TEXT("Slot 01")) : SavedSlot) + TEXT(" · ") + SavedToast.Body;
	}
	else if (const UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		if (const UArcRmlDemoUIViewModel* UI = Demo->GetUI())
		{
			Shown = UI->Toast;
			Shown.Progress.Reset();
		}
	}
	ARC_RML_DEMO_SET(*this, Toast, Shown);
}
