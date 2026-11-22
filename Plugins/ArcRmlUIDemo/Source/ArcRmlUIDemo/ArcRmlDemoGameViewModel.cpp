// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoGameViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoGameViewModel)

namespace ArcRmlDemoGameViewModelPrivate
{
	FArcRmlDemoStatLine Stat(const FString& Value, const TCHAR* Label)
	{
		FArcRmlDemoStatLine Line;
		Line.Value = Value;
		Line.Label = Label;
		return Line;
	}

	FString TypeOf(const FArcRmlDemoItem& Item)
	{
		if (Item.Category == TEXT("weapon"))
		{
			return TEXT("Weapon");
		}
		if (Item.Category == TEXT("armor"))
		{
			return TEXT("Armor · ") + UArcRmlDemoGameViewModel::SlotLabel(Item.Slot);
		}
		if (Item.Category == TEXT("consumable"))
		{
			return TEXT("Consumable · stacks");
		}
		if (Item.Category == TEXT("material"))
		{
			return TEXT("Crafting material");
		}
		return Item.Category == TEXT("quest") ? FString(TEXT("Quest item")) : FString();
	}
}

void UArcRmlDemoGameViewModel::ResetToStart()
{
	TArray<FArcRmlDemoItem> StartBag = StartingBag;
	StartBag.SetNum(FMath::Max(BagSize, 0));
	ARC_RML_DEMO_SET(*this, Bag, StartBag);
	ARC_RML_DEMO_SET(*this, Equipment, StartingEquipment);
	ARC_RML_DEMO_SET(*this, Gold, StartingGold);
	ARC_RML_DEMO_SET(*this, Shards, StartingShards);
	ARC_RML_DEMO_SET(*this, bMultiplayer, false);
	ARC_RML_DEMO_SET(*this, AppliedSettings, StartingAppliedSettings);
	ARC_RML_DEMO_SET(*this, SaveSlots, StartingSaveSlots);
}

bool UArcRmlDemoGameViewModel::AddToBag(const FArcRmlDemoItem& Item)
{
	if (Item.Slot.IsEmpty())
	{
		for (FArcRmlDemoItem& Held : Bag)
		{
			if (!Held.IsEmpty() && Held.Name == Item.Name)
			{
				Held.Quantity += Item.Quantity;
				NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Bag));
				return true;
			}
		}
	}
	const int32 Free = FirstFree();
	if (Free == INDEX_NONE)
	{
		return false;
	}
	Bag[Free] = Item;
	Bag[Free].bNew = true;
	NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Bag));
	return true;
}

void UArcRmlDemoGameViewModel::SetBagSlot(int32 Index, const FArcRmlDemoItem& Item)
{
	if (!Bag.IsValidIndex(Index) || Bag[Index] == Item)
	{
		return;
	}
	Bag[Index] = Item;
	NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Bag));
}

void UArcRmlDemoGameViewModel::SetBag(const TArray<FArcRmlDemoItem>& InBag)
{
	ARC_RML_DEMO_SET(*this, Bag, InBag);
}

void UArcRmlDemoGameViewModel::UseMaterial(const FString& ItemName, int32 Count)
{
	bool bChanged = false;
	for (FArcRmlDemoItem& Held : Bag)
	{
		if (Count <= 0)
		{
			break;
		}
		if (Held.IsEmpty() || Held.Name != ItemName)
		{
			continue;
		}
		const int32 Taken = FMath::Min(Count, Held.Quantity);
		Held.Quantity -= Taken;
		Count -= Taken;
		bChanged = true;
		if (Held.Quantity <= 0)
		{
			Held = FArcRmlDemoItem();
		}
	}
	if (bChanged)
	{
		NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Bag));
	}
}

void UArcRmlDemoGameViewModel::SetEquipped(const FString& EquipSlot, const FArcRmlDemoItem& Item)
{
	FArcRmlDemoEquipSlot* Found = Equipment.FindByPredicate([&EquipSlot](const FArcRmlDemoEquipSlot& Candidate) -> bool { return Candidate.Slot == EquipSlot; });
	if (!Found || Found->Item == Item)
	{
		return;
	}
	Found->Item = Item;
	NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, Equipment));
}

bool UArcRmlDemoGameViewModel::SpendGold(int32 Amount)
{
	if (Amount < 0 || Amount > Gold)
	{
		return false;
	}
	ARC_RML_DEMO_SET(*this, Gold, Gold - Amount);
	return true;
}

void UArcRmlDemoGameViewModel::AddGold(int32 Amount)
{
	ARC_RML_DEMO_SET(*this, Gold, Gold + Amount);
}

void UArcRmlDemoGameViewModel::SetMultiplayer(bool bInMultiplayer)
{
	ARC_RML_DEMO_SET(*this, bMultiplayer, bInMultiplayer);
}

void UArcRmlDemoGameViewModel::SetAppliedSettings(const TArray<FArcRmlDemoAppliedSetting>& InSettings)
{
	ARC_RML_DEMO_SET(*this, AppliedSettings, InSettings);
}

void UArcRmlDemoGameViewModel::SetSaveSlot(int32 Index, const FArcRmlDemoSaveSlot& Slot)
{
	if (!SaveSlots.IsValidIndex(Index) || SaveSlots[Index] == Slot)
	{
		return;
	}
	SaveSlots[Index] = Slot;
	NotifyField(GET_MEMBER_NAME_CHECKED(ThisClass, SaveSlots));
}

void UArcRmlDemoGameViewModel::SetSaveNotice(const FString& SlotLabel)
{
	ARC_RML_DEMO_SET(*this, SaveNotice, SlotLabel);
}

int32 UArcRmlDemoGameViewModel::FirstFree() const
{
	return Bag.IndexOfByPredicate([](const FArcRmlDemoItem& Held) -> bool { return Held.IsEmpty(); });
}

int32 UArcRmlDemoGameViewModel::HaveOf(const FString& ItemName) const
{
	int32 Count = 0;
	for (const FArcRmlDemoItem& Held : Bag)
	{
		if (!Held.IsEmpty() && Held.Name == ItemName)
		{
			Count += Held.Quantity;
		}
	}
	return Count;
}

const FArcRmlDemoItem* UArcRmlDemoGameViewModel::FindEquipped(const FString& EquipSlot) const
{
	const FArcRmlDemoEquipSlot* Found = Equipment.FindByPredicate([&EquipSlot](const FArcRmlDemoEquipSlot& Candidate) -> bool { return Candidate.Slot == EquipSlot; });
	return Found && !Found->Item.IsEmpty() ? &Found->Item : nullptr;
}

const FArcRmlDemoAppliedSetting* UArcRmlDemoGameViewModel::FindAppliedSetting(const FString& SettingLabel) const
{
	return AppliedSettings.FindByPredicate([&SettingLabel](const FArcRmlDemoAppliedSetting& Candidate) -> bool { return Candidate.Label == SettingLabel; });
}

FString UArcRmlDemoGameViewModel::FormatNumber(int64 Value)
{
	const bool bNegative = Value < 0;
	FString Digits = FString::Printf(TEXT("%lld"), bNegative ? -Value : Value);
	for (int32 Index = Digits.Len() - 3; Index > 0; Index -= 3)
	{
		Digits.InsertAt(Index, TEXT(','));
	}
	return bNegative ? TEXT("-") + Digits : Digits;
}

FArcRmlDemoCurrencyRow UArcRmlDemoGameViewModel::Coins(const FString& Value, bool bInsufficient) const
{
	FArcRmlDemoCurrencyRow Row;
	Row.Icon = GoldIcon;
	Row.Value = Value;
	Row.bInsufficient = bInsufficient;
	return Row;
}

FArcRmlDemoCurrencyRow UArcRmlDemoGameViewModel::ShardsRow(const FString& Value) const
{
	FArcRmlDemoCurrencyRow Row;
	Row.Icon = ShardIcon;
	Row.Value = Value;
	return Row;
}

FString UArcRmlDemoGameViewModel::RarityLabel(const FString& InRarity)
{
	static const TMap<FString, FString> Labels = {
		{TEXT("common"), TEXT("Common")}, {TEXT("uncommon"), TEXT("Uncommon")}, {TEXT("rare"), TEXT("Rare")}, {TEXT("epic"), TEXT("Epic")},
		{TEXT("legendary"), TEXT("Legendary")}};
	const FString* Label = Labels.Find(InRarity);
	return Label ? *Label : FString();
}

int32 UArcRmlDemoGameViewModel::RarityRank(const FString& InRarity)
{
	static const TMap<FString, int32> Ranks = {
		{TEXT("common"), 0}, {TEXT("uncommon"), 1}, {TEXT("rare"), 2}, {TEXT("epic"), 3}, {TEXT("legendary"), 4}};
	const int32* Rank = Ranks.Find(InRarity);
	return Rank ? *Rank : 0;
}

FString UArcRmlDemoGameViewModel::SlotLabel(const FString& InSlot)
{
	static const TMap<FString, FString> Labels = {{TEXT("helmet"), TEXT("Helmet")}, {TEXT("amulet"), TEXT("Amulet")}, {TEXT("chest"), TEXT("Chest")},
		{TEXT("gloves"), TEXT("Gloves")}, {TEXT("legs"), TEXT("Legs")}, {TEXT("boots"), TEXT("Boots")}, {TEXT("ring1"), TEXT("Ring")}, {TEXT("ring2"), TEXT("Ring")},
		{TEXT("trinket1"), TEXT("Trinket")}, {TEXT("trinket2"), TEXT("Trinket")}, {TEXT("main"), TEXT("Main hand")}, {TEXT("off"), TEXT("Off hand")}};
	const FString* Label = Labels.Find(InSlot);
	return Label ? *Label : FString();
}

FArcRmlDemoItemTooltip UArcRmlDemoGameViewModel::TipOf(const FArcRmlDemoItem& Item) const
{
	using namespace ArcRmlDemoGameViewModelPrivate;

	const int32 Rank = RarityRank(Item.Rarity);
	FArcRmlDemoItemTooltip Tip;
	Tip.Name = Item.Name;
	Tip.Rarity = Item.Rarity;
	Tip.RarityLabel = RarityLabel(Item.Rarity);
	Tip.Type = TypeOf(Item);
	if (Item.Category == TEXT("weapon"))
	{
		Tip.Primary.Add(Stat(FString::Printf(TEXT("%d–%d"), 120 + Rank * 30, 170 + Rank * 30), TEXT("Damage")));
		Tip.Stats.Add(Stat(FString::Printf(TEXT("+%d"), 20 + Rank * 7), TEXT("Strength")));
		Tip.Stats.Add(Stat(FString::Printf(TEXT("+%d.5%%"), 2 + Rank), TEXT("Critical strike chance")));
	}
	else if (Item.Category == TEXT("armor"))
	{
		Tip.Stats.Add(Stat(FString::Printf(TEXT("+%d"), 120 + Rank * 60), TEXT("Armor")));
		Tip.Stats.Add(Stat(FString::Printf(TEXT("+%d"), 10 + Rank * 6), TEXT("Vitality")));
	}
	else if (Item.Category == TEXT("consumable"))
	{
		Tip.Stats.Add(Stat(TEXT("Use"), TEXT("to apply its effect")));
	}
	else
	{
		Tip.Stats.Add(Stat(TEXT("Material"), TEXT("used in crafting")));
	}
	const int32 Step = Rank + 1;
	Tip.SellItems.Add(Coins(FormatNumber(int64(Step) * Step * 120 * FMath::Max(1, Item.Quantity))));
	return Tip;
}
