// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoCharacterViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoHubViewModel.h"
#include "ArcRmlDemoInventoryViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoCharacterViewModel)

void UArcRmlDemoCharacterViewModel::Initialize()
{
	Pool = StartPoints;
	Base.Reset();
	Pending.Reset();
	for (const FArcRmlDemoAttributeDef& Def : AttributeDefs)
	{
		Base.Add(Def.Base);
		Pending.Add(0);
	}
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Bag));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Equipment));
	Refresh();
}

void UArcRmlDemoCharacterViewModel::Change(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Demo || !Game || !Game->Equipment.IsValidIndex(Index))
	{
		return;
	}
	if (UArcRmlDemoInventoryViewModel* Inventory = Demo->FindScreenModel<UArcRmlDemoInventoryViewModel>())
	{
		Inventory->ShowCategory(Game->Equipment[Index].Slot == TEXT("main") ? TEXT("weapon") : TEXT("armor"));
	}
	if (UArcRmlDemoHubViewModel* Hub = Demo->FindScreenModel<UArcRmlDemoHubViewModel>())
	{
		Hub->SelectTab(TEXT("Inventory"));
	}
}

void UArcRmlDemoCharacterViewModel::Unequip(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	const int32 Free = Game ? Game->FirstFree() : INDEX_NONE;
	if (!Demo || !Game || !Game->Equipment.IsValidIndex(Index) || Game->Equipment[Index].Item.IsEmpty() || Free == INDEX_NONE)
	{
		return;
	}
	const FArcRmlDemoEquipSlot Worn = Game->Equipment[Index];
	Game->SetBagSlot(Free, Worn.Item);
	Game->SetEquipped(Worn.Slot, FArcRmlDemoItem());
	Demo->ShowToast(TEXT("info"), TEXT("Unequipped ") + Worn.Item.Name, TEXT("Moved to your backpack."));
}

void UArcRmlDemoCharacterViewModel::AttributeStep(int32 Index, int32 Delta)
{
	if (!Pending.IsValidIndex(Index))
	{
		return;
	}
	if (Delta > 0 && Pool - PendingTotal() > 0)
	{
		++Pending[Index];
	}
	else if (Delta < 0 && Pending[Index] > 0)
	{
		--Pending[Index];
	}
	Refresh();
}

void UArcRmlDemoCharacterViewModel::AttributeInfo(int32 Index)
{
}

void UArcRmlDemoCharacterViewModel::ApplyPoints()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const int32 Spent = PendingTotal();
	if (!Demo || Spent <= 0)
	{
		return;
	}
	for (int32 Index = 0; Index < Base.Num(); ++Index)
	{
		Base[Index] += Pending[Index];
		Pending[Index] = 0;
	}
	Pool -= Spent;
	Demo->ShowToast(TEXT("success"), TEXT("Attributes applied"), FString::Printf(TEXT("%d point%s spent."), Spent, Spent > 1 ? TEXT("s") : TEXT("")));
	Refresh();
}

void UArcRmlDemoCharacterViewModel::ResetPoints()
{
	for (int32& Points : Pending)
	{
		Points = 0;
	}
	Refresh();
}

int32 UArcRmlDemoCharacterViewModel::BadgeCount() const
{
	return Pool > 0 ? Pool - PendingTotal() : 0;
}

void UArcRmlDemoCharacterViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

int32 UArcRmlDemoCharacterViewModel::PendingTotal() const
{
	int32 Total = 0;
	for (const int32 Points : Pending)
	{
		Total += Points;
	}
	return Total;
}

void UArcRmlDemoCharacterViewModel::Refresh()
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return;
	}
	TArray<FArcRmlDemoEquipRow> Left;
	TArray<FArcRmlDemoEquipRow> Right;
	const int32 Half = Game->Equipment.Num() / 2;
	for (int32 Index = 0; Index < Game->Equipment.Num(); ++Index)
	{
		const FArcRmlDemoEquipSlot& Equip = Game->Equipment[Index];
		FArcRmlDemoEquipRow Row;
		Row.Slot = UArcRmlDemoGameViewModel::SlotLabel(Equip.Slot);
		Row.SlotKey = Equip.Slot;
		if (!Equip.Item.IsEmpty())
		{
			Row.Icon = Equip.Item.Icon;
			Row.Rarity = Equip.Item.Rarity;
			Row.Name = Equip.Item.Name;
			Row.Durability = Equip.Item.Durability >= 0 ? FString::FromInt(Equip.Item.Durability) : FString();
		}
		else
		{
			Row.Name = TEXT("Empty");
		}
		(Index < Half ? Left : Right).Add(MoveTemp(Row));
	}

	const int32 Spending = PendingTotal();
	const int32 Unspent = Pool - Spending;
	const bool bAllocate = Pool > 0;
	TArray<FArcRmlDemoAttributeRow> Rows;
	for (int32 Index = 0; Index < AttributeDefs.Num() && Index < Base.Num(); ++Index)
	{
		const FArcRmlDemoAttributeDef& Def = AttributeDefs[Index];
		FArcRmlDemoAttributeRow& Row = Rows.AddDefaulted_GetRef();
		Row.Icon = Def.Icon;
		Row.Label = Def.Label;
		Row.Description = Def.Description;
		Row.Pending = Pending[Index];
		Row.Value = Base[Index] + Row.Pending;
		Row.Delta = Row.Pending > 0 ? FString::Printf(TEXT("+%d"), Row.Pending) : FString();
		Row.bCanInc = bAllocate && Unspent > 0;
		Row.bCanDec = Row.Pending > 0;
	}

	ARC_RML_DEMO_SET(*this, EquipLeft, Left);
	ARC_RML_DEMO_SET(*this, EquipRight, Right);
	ARC_RML_DEMO_SET(*this, Attributes, Rows);
	ARC_RML_DEMO_SET(*this, bUnspent, bAllocate);
	ARC_RML_DEMO_SET(*this, Remaining, FString::Printf(TEXT("%d / %d"), Unspent, Pool));
	ARC_RML_DEMO_SET(*this, PointsSub, bAllocate ? FString::Printf(TEXT("%d points available"), Unspent) : NoPointsSubtitle);
	ARC_RML_DEMO_SET(*this, bNoPending, Spending == 0);
	ARC_RML_DEMO_SET(*this, bCanAdd, bAllocate && Unspent > 0);
	ARC_RML_DEMO_SET(*this, bCanUnequip, Game->FirstFree() != INDEX_NONE);
	UArcRmlDemoHubViewModel::RefreshFor(GetDemo());
}
