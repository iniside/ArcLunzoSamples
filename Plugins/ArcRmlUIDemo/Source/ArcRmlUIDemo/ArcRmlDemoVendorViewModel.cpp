// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoVendorViewModel.h"

#include "ArcRmlDemoSubsystem.h"
#include "HAL/PlatformTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoVendorViewModel)

namespace ArcRmlDemoVendorViewModelPrivate
{
	FArcRmlDemoModalButtonRow Button(const FString& Label, const FString& Key = FString())
	{
		FArcRmlDemoModalButtonRow Row;
		Row.Label = Label;
		Row.Key = Key;
		return Row;
	}

	int32 SellPrice(const FString& Rarity)
	{
		const int32 Step = UArcRmlDemoGameViewModel::RarityRank(Rarity) + 1;
		return Step * Step * 30;
	}
}

void UArcRmlDemoVendorViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Bag));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Gold));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Shards));
	Refresh();
}

void UArcRmlDemoVendorViewModel::InitScreen(const FString& Param)
{
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	Refresh();
}

void UArcRmlDemoVendorViewModel::Back()
{
	if (bConfirm)
	{
		Choose(0);
		return;
	}
	Super::Back();
}

void UArcRmlDemoVendorViewModel::Activate(const FString& List, int32 Index)
{
	if (bConfirm || List != ItemList() || Index < 0)
	{
		return;
	}
	Selected = Index;
	Refresh();
	OpenConfirm();
}

void UArcRmlDemoVendorViewModel::Deal()
{
	if (!bConfirm)
	{
		OpenConfirm();
	}
}

void UArcRmlDemoVendorViewModel::BuyMax(const FString& List, int32 Index)
{
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (bConfirm || !Game || ActiveTab != 0)
	{
		return;
	}
	const bool bOnItem = SelectFrom(List, Index);
	FDeal Current;
	if (!FindDeal(Current) || (!bOnItem && Game->Gold < Current.Unit))
	{
		Refresh();
		return;
	}
	Quantity = FMath::Clamp(Game->Gold / FMath::Max(1, Current.Unit), 1, FMath::Max(1, Current.MaxQuantity));
	Refresh();
}

void UArcRmlDemoVendorViewModel::Compare(const FString& List, int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (bConfirm || !Demo || !Game)
	{
		return;
	}
	SelectFrom(List, Index);
	Refresh();
	FDeal Current;
	if (!FindDeal(Current) || Current.Item.Slot.IsEmpty())
	{
		return;
	}
	const FString& Slot = Current.Item.Slot;
	const FString EquipSlot = Slot == TEXT("ring") ? TEXT("ring1") : Slot == TEXT("trinket") ? TEXT("trinket1") : Slot;
	const FArcRmlDemoItem* Equipped = Game->FindEquipped(EquipSlot);
	Demo->ShowToast(TEXT("info"), TEXT("Compared with ") + (Equipped ? Equipped->Name : FString(TEXT("an empty slot"))),
		Equipped ? UArcRmlDemoGameViewModel::RarityLabel(Equipped->Rarity) + TEXT(" → ") + UArcRmlDemoGameViewModel::RarityLabel(Current.Item.Rarity) : FString());
}

void UArcRmlDemoVendorViewModel::Stepped(const FString& Field, int32 Delta)
{
	FDeal Current;
	if (Field != TEXT("qty") || !FindDeal(Current))
	{
		return;
	}
	Quantity = FMath::Clamp(Current.Quantity + (Delta < 0 ? -1 : 1), 1, FMath::Max(1, Current.MaxQuantity));
	Refresh();
}

void UArcRmlDemoVendorViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (bConfirm || !Tabs.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, ActiveTab, Index);
	Quantity = 1;
	Selected = 0;
	Refresh();
}

void UArcRmlDemoVendorViewModel::Choose(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!bConfirm || !Demo || !Game)
	{
		return;
	}
	FDeal Current;
	const bool bValid = FindDeal(Current);
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	if (Index != 1 || !bValid)
	{
		Refresh();
		return;
	}
	const FString Gold = UArcRmlDemoGameViewModel::FormatNumber(Current.Total);
	if (Current.bSell)
	{
		FArcRmlDemoItem Left = Game->Bag[Current.BagPos];
		FArcRmlDemoItem Sold = Left;
		Sold.Quantity = Current.Quantity;
		Left.Quantity -= Current.Quantity;
		Game->SetBagSlot(Current.BagPos, Left.Quantity > 0 ? Left : FArcRmlDemoItem());
		Game->AddGold(Current.Total);
		Buyback.Insert(Sold, 0);
		Demo->ShowToast(TEXT("success"), FString::Printf(TEXT("Sold %d × %s"), Current.Quantity, *Current.Item.Name), TEXT("+") + Gold + TEXT(" gold"));
		Selected = FMath::Min(Selected, FMath::Max(0, BagPositions().Num() - 1));
		Quantity = 1;
		Refresh();
		return;
	}
	FArcRmlDemoItem Bought = Current.Item;
	Bought.Quantity = Current.Quantity;
	if (!Current.bOwned)
	{
		Bought.Id = FString::Printf(TEXT("v%llu"), FPlatformTime::Cycles64());
	}
	if (!Game->AddToBag(Bought))
	{
		Demo->ShowToast(TEXT("error"), TEXT("Backpack full"), TEXT("Sell or drop something first."));
		Refresh();
		return;
	}
	Game->SpendGold(Current.Total);
	if (IsBuyback())
	{
		const int32 Found = Buyback.IndexOfByPredicate([&Current](const FArcRmlDemoItem& Held) -> bool { return Held.Name == Current.Item.Name; });
		if (Found != INDEX_NONE)
		{
			Buyback[Found].Quantity -= Current.Quantity;
			if (Buyback[Found].Quantity <= 0)
			{
				Buyback.RemoveAt(Found);
			}
		}
	}
	Demo->ShowToast(TEXT("success"), FString::Printf(TEXT("%s%d × %s"), IsBuyback() ? TEXT("Bought back ") : TEXT("Bought "), Current.Quantity, *Current.Item.Name),
		TEXT("−") + Gold + TEXT(" gold"));
	Quantity = 1;
	Refresh();
}

void UArcRmlDemoVendorViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

FString UArcRmlDemoVendorViewModel::ActionLabel() const
{
	return IsSell() ? TEXT("Sell") : IsBuyback() ? TEXT("Buy back") : TEXT("Buy");
}

TArray<int32> UArcRmlDemoVendorViewModel::BagPositions() const
{
	TArray<int32> Result;
	if (const UArcRmlDemoGameViewModel* Game = GetGame())
	{
		for (int32 Pos = 0; Pos < Game->Bag.Num(); ++Pos)
		{
			if (!Game->Bag[Pos].IsEmpty())
			{
				Result.Add(Pos);
			}
		}
	}
	return Result;
}

bool UArcRmlDemoVendorViewModel::FindDeal(FDeal& Out) const
{
	using namespace ArcRmlDemoVendorViewModelPrivate;

	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return false;
	}
	Out = FDeal();
	Out.bSell = IsSell();
	if (Out.bSell)
	{
		const TArray<int32> Positions = BagPositions();
		if (!Positions.IsValidIndex(Selected))
		{
			return false;
		}
		Out.BagPos = Positions[Selected];
		Out.Item = Game->Bag[Out.BagPos];
		Out.bOwned = true;
		Out.Unit = SellPrice(Out.Item.Rarity);
		Out.MaxQuantity = Out.Item.Quantity;
	}
	else if (IsBuyback())
	{
		if (!Buyback.IsValidIndex(Selected))
		{
			return false;
		}
		Out.Item = Buyback[Selected];
		Out.bOwned = true;
		Out.Unit = SellPrice(Out.Item.Rarity);
		Out.MaxQuantity = Out.Item.Quantity;
	}
	else
	{
		if (!Stock.IsValidIndex(Selected))
		{
			return false;
		}
		Out.Item = Stock[Selected].Item;
		Out.Unit = Stock[Selected].Price;
		Out.MaxQuantity = MaxBuyQuantity;
	}
	Out.Quantity = FMath::Clamp(Quantity, 1, FMath::Max(1, Out.MaxQuantity));
	Out.Total = Out.Unit * Out.Quantity;
	Out.bShort = !Out.bSell && Out.Total > Game->Gold;
	return true;
}

bool UArcRmlDemoVendorViewModel::SelectFrom(const FString& List, int32 Index)
{
	if (List != ItemList() || Index < 0)
	{
		return false;
	}
	Selected = Index;
	return true;
}

void UArcRmlDemoVendorViewModel::OpenConfirm()
{
	FDeal Current;
	if (!FindDeal(Current) || Current.bShort)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, true);
	Refresh();
}

void UArcRmlDemoVendorViewModel::Refresh()
{
	using namespace ArcRmlDemoVendorViewModelPrivate;

	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game)
	{
		return;
	}
	const bool bSellTab = IsSell();
	const bool bBuyback = IsBuyback();
	const int32 Gold = Game->Gold;
	const TArray<int32> Positions = BagPositions();
	FDeal Current;
	const bool bDeal = FindDeal(Current);

	TArray<FArcRmlDemoStockRow> Rows;
	const int32 StockCount = bBuyback ? Buyback.Num() : Stock.Num();
	for (int32 Index = 0; Index < StockCount; ++Index)
	{
		const FArcRmlDemoItem& Entry = bBuyback ? Buyback[Index] : Stock[Index].Item;
		const int32 Price = bBuyback ? SellPrice(Entry.Rarity) : Stock[Index].Price;
		FArcRmlDemoStockRow& Row = Rows.AddDefaulted_GetRef();
		Row.Icon = Entry.Icon;
		Row.Rarity = Entry.Rarity;
		Row.Price.Add(Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Price), !bSellTab && Price > Gold));
		Row.Quantity = FMath::Max(1, Entry.Quantity);
		Row.bUnusable = !bBuyback && Stock[Index].bUnusable;
		Row.bSelected = !bSellTab && Index == Selected;
	}

	TArray<FArcRmlDemoItemSlotRow> Cells;
	const int32 CellCount = FMath::Max(BagCells, Positions.Num());
	for (int32 Index = 0; Index < CellCount; ++Index)
	{
		FArcRmlDemoItemSlotRow& Cell = Cells.AddDefaulted_GetRef();
		Cell.Pos = Index;
		if (Positions.IsValidIndex(Index))
		{
			const FArcRmlDemoItem& Held = Game->Bag[Positions[Index]];
			Cell.Icon = Held.Icon;
			Cell.Rarity = Held.Rarity;
			Cell.Quantity = Held.Quantity;
			Cell.bNew = Held.bNew;
			Cell.bSelected = bSellTab && Index == Selected;
		}
	}

	FArcRmlDemoItemTooltip Tip;
	if (bDeal)
	{
		FArcRmlDemoItem Shown = Current.Item;
		if (!Current.bOwned)
		{
			Shown.Quantity = 1;
		}
		Tip = Game->TipOf(Shown);
	}
	else
	{
		Tip.Name = bBuyback ? TEXT("Nothing to buy back") : TEXT("Nothing selected");
		Tip.Rarity = TEXT("common");
		Tip.Type = bBuyback ? TEXT("Items you sell this visit appear here.") : TEXT("");
	}

	const int32 Unit = bDeal ? Current.Unit : 0;
	const int32 Count = bDeal ? Current.Quantity : 1;
	const int32 Sum = Unit * Count;
	const int32 Short = !bSellTab && Sum > Gold ? Sum - Gold : 0;
	const FString Action = ActionLabel();
	const FString TxKey = bSellTab ? TEXT("sell") : bBuyback ? TEXT("buyback") : TEXT("buy");

	TArray<FArcRmlDemoCurrencyRow> Purse;
	FArcRmlDemoCurrencyRow& GoldRow = Purse.Add_GetRef(Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Gold)));
	GoldRow.Label = TEXT("Gold");
	FArcRmlDemoCurrencyRow& ShardRow = Purse.Add_GetRef(Game->ShardsRow(UArcRmlDemoGameViewModel::FormatNumber(Game->Shards)));
	ShardRow.Label = TEXT("Tide shards");
	ShardRow.Rarity = TEXT("epic");

	ARC_RML_DEMO_SET(*this, Wallet, Purse);
	ARC_RML_DEMO_SET(*this, StockTitle, bBuyback ? FString(TEXT("Buyback")) : FString(TEXT("Vendor stock")));
	ARC_RML_DEMO_SET(*this, StockSub, bBuyback ? BuybackSubtitle : StockSubtitle);
	ARC_RML_DEMO_SET(*this, StockRows, Rows);
	ARC_RML_DEMO_SET(*this, Bag, Cells);
	ARC_RML_DEMO_SET(*this, BagStops, FMath::Max(1, Positions.Num()));
	ARC_RML_DEMO_SET(*this, BagSub, FString::Printf(TEXT("%d / %d slots"), Positions.Num(), Game->BagSize));
	ARC_RML_DEMO_SET(*this, Item, Tip);
	ARC_RML_DEMO_SET(*this, TxTitle, bSellTab ? FString(TEXT("Sell")) : bBuyback ? FString(TEXT("Buy back")) : FString(TEXT("Purchase")));
	ARC_RML_DEMO_SET(*this, TxAction, Action);
	ARC_RML_DEMO_SET(*this, TotalLabel, bSellTab ? FString(TEXT("You receive")) : FString(TEXT("Total")));
	ARC_RML_DEMO_SET(*this, QuantityLabel, FString::FromInt(Count));
	ARC_RML_DEMO_SET(*this, UnitPrice, TArray<FArcRmlDemoCurrencyRow>({Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Unit))}));
	ARC_RML_DEMO_SET(*this, After, TArray<FArcRmlDemoCurrencyRow>({Game->Coins(
		Short ? FString(TEXT("—")) : UArcRmlDemoGameViewModel::FormatNumber(bSellTab ? Gold + Sum : Gold - Sum))}));
	ARC_RML_DEMO_SET(*this, Total, TArray<FArcRmlDemoCurrencyRow>({Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Sum), Short > 0)}));
	ARC_RML_DEMO_SET(*this, ShortBody, Short ? TEXT("You need ") + UArcRmlDemoGameViewModel::FormatNumber(Short) + TEXT(" more gold. Sell items or lower the quantity.") : FString());
	ARC_RML_DEMO_SET(*this, bBlocked, Short > 0);
	ARC_RML_DEMO_SET(*this, bCanDeal, bDeal && !Current.bShort);
	ARC_RML_DEMO_SET(*this, bCanBuyMax, ActiveTab == 0 && bDeal && Gold >= Current.Unit);
	ARC_RML_DEMO_SET(*this, bComparable, bDeal && !Current.Item.Slot.IsEmpty());
	ARC_RML_DEMO_SET(*this, bQuantityOn, bDeal && Current.MaxQuantity > 1);
	ARC_RML_DEMO_SET(*this, ModalTitle, bSellTab ? FString(TEXT("Confirm sale")) : FString(TEXT("Confirm purchase")));
	ARC_RML_DEMO_SET(*this, ConfirmBody, bSellTab
		? FString::Printf(TEXT("Sell %s for %s gold? You can buy it back until you leave Old Port."), *Tip.Name, *UArcRmlDemoGameViewModel::FormatNumber(Sum))
		: FString::Printf(TEXT("Buy %d × %s for %s gold?"), Count, *Tip.Name, *UArcRmlDemoGameViewModel::FormatNumber(Sum)));
	ARC_RML_DEMO_SET(*this, ModalButtons, TArray<FArcRmlDemoModalButtonRow>({Button(TEXT("Cancel")), Button(Action, TxKey)}));
}
