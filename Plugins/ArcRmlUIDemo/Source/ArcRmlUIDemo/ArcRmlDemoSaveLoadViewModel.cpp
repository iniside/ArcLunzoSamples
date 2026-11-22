// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoSaveLoadViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoSaveLoadViewModel)

namespace ArcRmlDemoSaveLoadViewModelPrivate
{
	FArcRmlDemoFactRow Fact(const TCHAR* Label, const FString& Value)
	{
		FArcRmlDemoFactRow Row;
		Row.Label = Label;
		Row.Value = Value;
		return Row;
	}

	bool IsFilled(const FArcRmlDemoSaveSlot& Slot)
	{
		return Slot.State == TEXT("filled") || Slot.State == TEXT("autosave");
	}
}

void UArcRmlDemoSaveLoadViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, SaveSlots));
}

void UArcRmlDemoSaveLoadViewModel::InitScreen(const FString& Param)
{
	ARC_RML_DEMO_SET(*this, Mode, Param == TEXT("save") ? FString(TEXT("save")) : FString(TEXT("load")));
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	Refresh();
}

void UArcRmlDemoSaveLoadViewModel::Back()
{
	if (bConfirm)
	{
		Choose(0);
		return;
	}
	Super::Back();
}

void UArcRmlDemoSaveLoadViewModel::Activate(int32 Slot)
{
	if (bConfirm || !GetSlots().IsValidIndex(Slot))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, SelectedSlot, Slot);
	Refresh();
	Primary();
}

void UArcRmlDemoSaveLoadViewModel::Primary()
{
	using namespace ArcRmlDemoSaveLoadViewModelPrivate;

	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (bConfirm || !Demo || !Game || !GetSlots().IsValidIndex(SelectedSlot))
	{
		return;
	}
	const FArcRmlDemoSaveSlot Slot = GetSlots()[SelectedSlot];
	if (Mode != TEXT("save"))
	{
		if (IsFilled(Slot))
		{
			Game->SetMultiplayer(false);
			Demo->ResetTo(TArray<FString>{TEXT("main"), TEXT("loading")}, TEXT("hud"));
		}
		return;
	}
	FArcRmlDemoSaveSlot Saved = NewSave;
	Saved.SlotLabel = Slot.SlotLabel;
	Game->SetSaveSlot(SelectedSlot, Saved);
	Game->SetSaveNotice(Slot.SlotLabel);
	Demo->Pop();
}

void UArcRmlDemoSaveLoadViewModel::Delete()
{
	if (bConfirm || !GetSlots().IsValidIndex(SelectedSlot) || GetSlots()[SelectedSlot].State == TEXT("empty"))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, true);
	Refresh();
}

void UArcRmlDemoSaveLoadViewModel::Choose(int32 Index)
{
	if (!bConfirm)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (Index == 1 && Demo && Game && GetSlots().IsValidIndex(SelectedSlot))
	{
		FArcRmlDemoSaveSlot Empty;
		Empty.State = TEXT("empty");
		Empty.SlotLabel = GetSlots()[SelectedSlot].SlotLabel;
		Game->SetSaveSlot(SelectedSlot, Empty);
		Demo->ShowToast(TEXT("warning"), TEXT("Save deleted"), Empty.SlotLabel);
	}
	Refresh();
}

void UArcRmlDemoSaveLoadViewModel::Selected(const FString& Field, int32 Index)
{
	if (Field == TEXT("crumbs"))
	{
		Back();
	}
}

void UArcRmlDemoSaveLoadViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

const TArray<FArcRmlDemoSaveSlot>& UArcRmlDemoSaveLoadViewModel::GetSlots() const
{
	static const TArray<FArcRmlDemoSaveSlot> None;
	const UArcRmlDemoGameViewModel* Game = GetGame();
	return Game ? Game->SaveSlots : None;
}

void UArcRmlDemoSaveLoadViewModel::Refresh()
{
	using namespace ArcRmlDemoSaveLoadViewModelPrivate;

	const TArray<FArcRmlDemoSaveSlot>& Saves = GetSlots();
	const bool bSave = Mode == TEXT("save");
	TArray<FArcRmlDemoSaveSlotRow> Rows;
	int32 Used = 0;
	for (const FArcRmlDemoSaveSlot& Slot : Saves)
	{
		FArcRmlDemoSaveSlotRow& Row = Rows.AddDefaulted_GetRef();
		Row.State = Slot.State;
		Row.SlotLabel = Slot.SlotLabel;
		Row.Title = Slot.Title;
		Row.Location = Slot.Location;
		Row.Playtime = Slot.Playtime;
		Row.Date = Slot.Date;
		Row.Chapter = Slot.Chapter;
		Row.Difficulty = Slot.Difficulty;
		Row.EmptyHint = bSave ? TEXT("Create a new save here.") : TEXT("");
		Used += Slot.State != TEXT("empty") ? 1 : 0;
	}
	const FArcRmlDemoSaveSlot Shown = Saves.IsValidIndex(SelectedSlot) ? Saves[SelectedSlot] : Saves.IsEmpty() ? FArcRmlDemoSaveSlot() : Saves[0];
	const bool bEmptyDetail = Shown.State == TEXT("empty") || Shown.State == TEXT("corrupted");
	const UArcRmlDemoSubsystem* Demo = GetDemo();
	const FString CrumbRoot = Demo && Demo->GetScreenBelow(GetScreenKey()) == TEXT("pause") ? TEXT("Pause") : TEXT("Main menu");

	ARC_RML_DEMO_SET(*this, Slots, Rows);
	ARC_RML_DEMO_SET(*this, Title, bSave ? FString(TEXT("Save Game")) : FString(TEXT("Load Game")));
	ARC_RML_DEMO_SET(*this, Crumbs, TArray<FString>({CrumbRoot, bSave ? TEXT("Save game") : TEXT("Load game")}));
	ARC_RML_DEMO_SET(*this, CountLabel, FString::Printf(TEXT("%d / %d"), Used, Saves.Num()));
	ARC_RML_DEMO_SET(*this, ConfirmBody,
		FString::Printf(TEXT("%s · %s (%s) will be permanently deleted. This cannot be undone."), *Shown.SlotLabel, *Shown.Title, *Shown.Playtime));

	TArray<FArcRmlDemoFactRow> Stats;
	if (bEmptyDetail)
	{
		ARC_RML_DEMO_SET(*this, DetailTitle, Shown.State == TEXT("corrupted") ? FString(TEXT("Data corrupted")) : FString(TEXT("Empty slot")));
		ARC_RML_DEMO_SET(*this, bNoDelete, Shown.State == TEXT("empty"));
		ARC_RML_DEMO_SET(*this, bNoPrimary, !bSave);
	}
	else
	{
		ARC_RML_DEMO_SET(*this, DetailTitle, Shown.Title);
		ARC_RML_DEMO_SET(*this, bNoDelete, false);
		ARC_RML_DEMO_SET(*this, bNoPrimary, false);
		Stats = {Fact(TEXT("Location"), Shown.Location), Fact(TEXT("Progress"), Shown.Chapter), Fact(TEXT("Difficulty"), Shown.Difficulty),
			Fact(TEXT("Playtime"), Shown.Playtime), Fact(TEXT("Saved"), Shown.Date)};
	}
	ARC_RML_DEMO_SET(*this, DetailSlotLabel, Shown.SlotLabel);
	ARC_RML_DEMO_SET(*this, DetailStats, Stats);
	ARC_RML_DEMO_SET(*this, PrimaryLabel, bSave ? (Shown.State == TEXT("empty") ? FString(TEXT("Save")) : FString(TEXT("Overwrite"))) : FString(TEXT("Load")));
}
