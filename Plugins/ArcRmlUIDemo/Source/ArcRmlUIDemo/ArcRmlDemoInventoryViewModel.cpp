// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoInventoryViewModel.h"

#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoInventoryViewModel)

namespace ArcRmlDemoInventoryViewModelPrivate
{
	const TCHAR* const SearchField = TEXT("invSearch");
}

UArcRmlDemoInventoryViewModel::UArcRmlDemoInventoryViewModel()
{
	Slots.SetRowType(FArcRmlDemoItemSlotRow::StaticStruct());
}

void UArcRmlDemoInventoryViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Bag));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Equipment));
	Refresh();
}

void UArcRmlDemoInventoryViewModel::Back()
{
	if (MoveFrom != INDEX_NONE)
	{
		MoveFrom = INDEX_NONE;
		Refresh();
		return;
	}
	Super::Back();
}

void UArcRmlDemoInventoryViewModel::Activate(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	const int32 Pos = PosAt(Index);
	if (!Demo || !Game || !Game->Bag.IsValidIndex(Pos))
	{
		return;
	}
	if (MoveFrom != INDEX_NONE)
	{
		if (Game->Bag[Pos].IsEmpty() && Game->Bag.IsValidIndex(MoveFrom))
		{
			const FArcRmlDemoItem Moved = Game->Bag[MoveFrom];
			const int32 From = MoveFrom;
			MoveFrom = INDEX_NONE;
			SelectedPos = Pos;
			Game->SetBagSlot(Pos, Moved);
			Game->SetBagSlot(From, FArcRmlDemoItem());
			Refresh();
		}
		return;
	}
	SelectedPos = Pos;
	const FArcRmlDemoItem Equipping = Game->Bag[Pos];
	if (Equipping.IsEmpty() || Equipping.Slot.IsEmpty())
	{
		Refresh();
		return;
	}
	const FString EquipSlot = EquipSlotFor(Equipping);
	const FArcRmlDemoItem* Worn = Game->FindEquipped(EquipSlot);
	const FArcRmlDemoItem Old = Worn ? *Worn : FArcRmlDemoItem();
	FArcRmlDemoItem Wearing = Equipping;
	Wearing.bNew = false;
	Game->SetEquipped(EquipSlot, Wearing);
	Game->SetBagSlot(Pos, Old);
	Demo->ShowToast(TEXT("success"), TEXT("Equipped ") + Equipping.Name,
		!Old.IsEmpty() ? Old.Name + TEXT(" moved to your backpack.") : UArcRmlDemoGameViewModel::SlotLabel(EquipSlot) + TEXT(" slot filled."));
}

void UArcRmlDemoInventoryViewModel::Use(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	const int32 Pos = PosAt(Index);
	const FArcRmlDemoItem* Held = BagItem(Pos);
	if (!Demo || !Game || MoveFrom != INDEX_NONE || !Held || Held->Category != TEXT("consumable"))
	{
		return;
	}
	SelectedPos = Pos;
	FArcRmlDemoItem Left = *Held;
	Left.Quantity -= 1;
	Demo->ShowToast(TEXT("success"), TEXT("Used ") + Left.Name, Left.Quantity > 0 ? FString::Printf(TEXT("%d left"), Left.Quantity) : FString(TEXT("That was the last one.")));
	Game->SetBagSlot(Pos, Left.Quantity > 0 ? Left : FArcRmlDemoItem());
	Refresh();
}

void UArcRmlDemoInventoryViewModel::Move(int32 Index)
{
	const int32 Pos = PosAt(Index);
	const FArcRmlDemoItem* Held = BagItem(Pos);
	if (MoveFrom != INDEX_NONE || !Held || Held->Category == TEXT("consumable") || !BuildView().bAll)
	{
		return;
	}
	SelectedPos = Pos;
	MoveFrom = Pos;
	Refresh();
}

void UArcRmlDemoInventoryViewModel::Swap(int32 Index)
{
	UArcRmlDemoGameViewModel* Game = GetGame();
	const int32 Pos = PosAt(Index);
	if (!Game || MoveFrom == INDEX_NONE || Pos == MoveFrom || !BagItem(Pos) || !Game->Bag.IsValidIndex(MoveFrom))
	{
		return;
	}
	TArray<FArcRmlDemoItem> Swapped = Game->Bag;
	Swapped.Swap(Pos, MoveFrom);
	SelectedPos = Pos;
	MoveFrom = INDEX_NONE;
	Game->SetBag(Swapped);
	Refresh();
}

void UArcRmlDemoInventoryViewModel::Drop(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	const int32 Pos = PosAt(Index);
	const FArcRmlDemoItem* Held = BagItem(Pos);
	if (!Demo || !Game || MoveFrom != INDEX_NONE || !Held)
	{
		return;
	}
	SelectedPos = Pos;
	Demo->ShowToast(TEXT("warning"), TEXT("Dropped ") + Held->Name);
	Game->SetBagSlot(Pos, FArcRmlDemoItem());
	Refresh();
}

void UArcRmlDemoInventoryViewModel::Junk(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const int32 Pos = PosAt(Index);
	const FArcRmlDemoItem* Held = BagItem(Pos);
	if (!Demo || MoveFrom != INDEX_NONE || !Held)
	{
		return;
	}
	SelectedPos = Pos;
	const bool bJunk = !JunkIds.Contains(Held->Id);
	if (bJunk)
	{
		JunkIds.Add(Held->Id);
	}
	else
	{
		JunkIds.Remove(Held->Id);
	}
	Demo->ShowToast(TEXT("info"), (bJunk ? TEXT("Marked ") : TEXT("Unmarked ")) + Held->Name, bJunk ? FString(TEXT("Junk sells in one go at any vendor.")) : FString());
	Refresh();
}

void UArcRmlDemoInventoryViewModel::FilterBy(int32 Index)
{
	if (MoveFrom != INDEX_NONE || Categories.IsEmpty())
	{
		return;
	}
	CategoryIndex = FMath::Clamp(Index, 0, Categories.Num() - 1);
	ARC_RML_DEMO_SET(*this, Query, FString());
	Refresh();
}

void UArcRmlDemoInventoryViewModel::CycleSort()
{
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game || MoveFrom != INDEX_NONE || SortOptions.IsEmpty())
	{
		return;
	}
	SortIndex = (SortIndex + 1) % SortOptions.Num();
	TArray<FArcRmlDemoItem> Items = Game->Bag.FilterByPredicate([](const FArcRmlDemoItem& Held) -> bool { return !Held.IsEmpty(); });
	const FString& By = SortOptions[SortIndex];
	Items.StableSort([&By](const FArcRmlDemoItem& A, const FArcRmlDemoItem& B) -> bool
	{
		if (By == TEXT("Rarity"))
		{
			const int32 RankA = UArcRmlDemoGameViewModel::RarityRank(A.Rarity);
			const int32 RankB = UArcRmlDemoGameViewModel::RarityRank(B.Rarity);
			return RankA != RankB ? RankA > RankB : A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
		}
		if (By == TEXT("Name"))
		{
			return A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
		}
		const int32 ByCategory = A.Category.Compare(B.Category, ESearchCase::IgnoreCase);
		return ByCategory != 0 ? ByCategory < 0 : A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
	});
	Items.SetNum(FMath::Max(Game->Bag.Num(), Items.Num()));
	Game->SetBag(Items);
	Refresh();
}

void UArcRmlDemoInventoryViewModel::ShowAll()
{
	FilterBy(0);
}

void UArcRmlDemoInventoryViewModel::ShowCategory(const FString& Key)
{
	MoveFrom = INDEX_NONE;
	CategoryIndex = FMath::Max(0, Categories.IndexOfByPredicate([&Key](const FArcRmlDemoInventoryCategory& Def) -> bool { return Def.Key == Key; }));
	ARC_RML_DEMO_SET(*this, Query, FString());
	const FView View = BuildView();
	SelectedPos = View.Cells.IsEmpty() ? INDEX_NONE : View.Cells[0].Pos;
	Refresh();
}

void UArcRmlDemoInventoryViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

void UArcRmlDemoInventoryViewModel::WriteText(const FString& Field, const FString& Value)
{
	if (Field == ArcRmlDemoInventoryViewModelPrivate::SearchField)
	{
		ARC_RML_DEMO_SET(*this, Query, Value);
		Refresh();
	}
}

UArcRmlDemoInventoryViewModel::FView UArcRmlDemoInventoryViewModel::BuildView() const
{
	FView View;
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return View;
	}
	const FString Key = Categories.IsValidIndex(CategoryIndex) ? Categories[CategoryIndex].Key : FString();
	const FString Search = Query.TrimStartAndEnd();
	View.bAll = Key.IsEmpty() && Search.IsEmpty();
	for (int32 Pos = 0; Pos < Game->Bag.Num(); ++Pos)
	{
		const FArcRmlDemoItem& Held = Game->Bag[Pos];
		if (View.bAll || (!Held.IsEmpty() && (Key.IsEmpty() || Held.Category == Key) && (Search.IsEmpty() || Held.Name.Contains(Search, ESearchCase::IgnoreCase))))
		{
			View.Cells.Add(FCell{Pos, false});
		}
	}
	View.bEmpty = View.Cells.IsEmpty();
	if (View.bAll)
	{
		for (int32 Index = 0; Index < LockedSlots; ++Index)
		{
			View.Cells.Add(FCell{INDEX_NONE, true});
		}
	}
	return View;
}

int32 UArcRmlDemoInventoryViewModel::PosAt(int32 Index) const
{
	const FView View = BuildView();
	return View.Cells.IsValidIndex(Index) ? View.Cells[Index].Pos : INDEX_NONE;
}

const FArcRmlDemoItem* UArcRmlDemoInventoryViewModel::BagItem(int32 Pos) const
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	return Game && Game->Bag.IsValidIndex(Pos) && !Game->Bag[Pos].IsEmpty() ? &Game->Bag[Pos] : nullptr;
}

FString UArcRmlDemoInventoryViewModel::EquipSlotFor(const FArcRmlDemoItem& Equipping) const
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return Equipping.Slot;
	}
	if (Equipping.Slot == TEXT("ring"))
	{
		return Game->FindEquipped(TEXT("ring1")) ? (Game->FindEquipped(TEXT("ring2")) ? TEXT("ring1") : TEXT("ring2")) : TEXT("ring1");
	}
	if (Equipping.Slot == TEXT("trinket"))
	{
		return Game->FindEquipped(TEXT("trinket2")) ? TEXT("trinket1") : TEXT("trinket2");
	}
	return Equipping.Slot;
}

void UArcRmlDemoInventoryViewModel::Refresh()
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return;
	}
	const FView View = BuildView();
	const bool bMove = MoveFrom != INDEX_NONE;

	TArray<FString> Keys;
	TArray<FArcRmlDemoItemSlotRow> Rows;
	for (int32 Index = 0; Index < View.Cells.Num(); ++Index)
	{
		const FCell& Cell = View.Cells[Index];
		FArcRmlDemoItemSlotRow& Row = Rows.AddDefaulted_GetRef();
		Row.Pos = Cell.Pos;
		Row.bLocked = Cell.bLocked;
		if (const FArcRmlDemoItem* Held = BagItem(Cell.Pos))
		{
			Row.Icon = Held->Icon;
			Row.Rarity = Held->Rarity;
			Row.Quantity = Held->Quantity;
			Row.Durability = Held->Durability >= 0 ? FString::FromInt(Held->Durability) : FString();
			Row.bNew = Held->bNew;
			Row.bConsumable = Held->Category == TEXT("consumable");
			if (bMove && MoveFrom == Cell.Pos)
			{
				Row.EmptyIcon = Held->Icon;
				Row.Icon = FArcRmlImageRef();
				Row.bGhost = true;
			}
		}
		Keys.Add(FString::FromInt(Index));
	}
	SetList(Slots, GET_MEMBER_NAME_CHECKED(ThisClass, Slots), Keys, Rows);

	int32 Used = 0;
	float Weight = 0.0f;
	for (const FArcRmlDemoItem& Held : Game->Bag)
	{
		Used += Held.IsEmpty() ? 0 : 1;
		Weight += Held.IsEmpty() ? 0.0f : Held.Weight * Held.Quantity;
	}
	TArray<FArcRmlDemoChipRow> ChipRows;
	for (int32 Index = 0; Index < Categories.Num(); ++Index)
	{
		const FArcRmlDemoInventoryCategory& Def = Categories[Index];
		FArcRmlDemoChipRow& Chip = ChipRows.AddDefaulted_GetRef();
		Chip.Label = Def.Label;
		Chip.Icon = Def.Icon;
		Chip.Count = Def.Key.IsEmpty() ? Used : Game->Bag.FilterByPredicate([&Def](const FArcRmlDemoItem& Held) -> bool { return !Held.IsEmpty() && Held.Category == Def.Key; }).Num();
		Chip.bActive = Index == CategoryIndex;
	}

	const FArcRmlDemoItem* Shown = bMove ? BagItem(MoveFrom) : BagItem(SelectedPos);
	FArcRmlDemoItemTooltip Tip;
	FArcRmlDemoItemTooltip Worn;
	if (Shown)
	{
		Tip = Game->TipOf(*Shown);
		const FArcRmlDemoItem* WornItem = !bMove && !Shown->Slot.IsEmpty() ? Game->FindEquipped(EquipSlotFor(*Shown)) : nullptr;
		if (WornItem)
		{
			Worn = Game->TipOf(*WornItem);
		}
	}

	FArcRmlDemoToast Moving;
	if (bMove && Shown)
	{
		Moving.bVisible = true;
		Moving.Tone = TEXT("info");
		Moving.Title = TEXT("Moving ") + Shown->Name;
		Moving.Body = TEXT("Place on an empty slot, or swap with an item.");
	}

	const float Rounded = FMath::RoundToFloat(Weight * 10.0f) / 10.0f;
	const float Pct = WeightMax > 0 ? Rounded / WeightMax * 100.0f : 0.0f;
	const FString SortName = SortOptions.IsValidIndex(SortIndex) ? SortOptions[SortIndex] : FString();
	const FString NextSort = SortOptions.IsEmpty() ? FString() : SortOptions[(SortIndex + 1) % SortOptions.Num()];

	ARC_RML_DEMO_SET(*this, Chips, ChipRows);
	ARC_RML_DEMO_SET(*this, Item, Tip);
	ARC_RML_DEMO_SET(*this, Equipped, Worn);
	ARC_RML_DEMO_SET(*this, bShowTip, !View.bEmpty && Shown != nullptr);
	ARC_RML_DEMO_SET(*this, WeightPct, Pct);
	ARC_RML_DEMO_SET(*this, WeightLabel, FString::Printf(TEXT("%.1f / %d kg"), Rounded, WeightMax));
	ARC_RML_DEMO_SET(*this, WeightTone, Pct > 75.0f ? FString(TEXT("warning")) : FString(TEXT("accent")));
	ARC_RML_DEMO_SET(*this, BagSub, FString::Printf(TEXT("%d / %d slots · %d locked"), Used, Game->BagSize, LockedSlots));
	ARC_RML_DEMO_SET(*this, bHasItems, !View.bEmpty);
	ARC_RML_DEMO_SET(*this, bEmpty, View.bEmpty);
	ARC_RML_DEMO_SET(*this, bShowsAll, View.bAll);
	ARC_RML_DEMO_SET(*this, bMoving, bMove);
	ARC_RML_DEMO_SET(*this, MoveToast, Moving);
	ARC_RML_DEMO_SET(*this, SortLabel, TEXT("Sort: ") + SortName);
	ARC_RML_DEMO_SET(*this, NextSortLabel, TEXT("Sort: ") + NextSort);
	ARC_RML_DEMO_SET(*this, Selected, View.Cells.IndexOfByPredicate([this](const FCell& Cell) -> bool { return Cell.Pos == SelectedPos; }));
}
