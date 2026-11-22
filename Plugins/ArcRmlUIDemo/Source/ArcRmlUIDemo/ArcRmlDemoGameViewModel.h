// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoViewModelBase.h"

#include "ArcRmlDemoGameViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	float Weight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	int32 Durability = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	bool bNew = false;

	bool IsEmpty() const
	{
		return Id.IsEmpty();
	}

	bool operator==(const FArcRmlDemoItem& Other) const = default;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoEquipSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FArcRmlDemoItem Item;

	bool operator==(const FArcRmlDemoEquipSlot& Other) const = default;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSaveSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString State;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString SlotLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Location;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Playtime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Date;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Chapter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Difficulty;

	bool operator==(const FArcRmlDemoSaveSlot& Other) const = default;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoAppliedSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	int32 Number = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Text;

	bool operator==(const FArcRmlDemoAppliedSetting& Other) const = default;
};

UCLASS()
class UArcRmlDemoGameViewModel : public UArcRmlDemoViewModelBase
{
	GENERATED_BODY()

public:
	void ResetToStart();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	bool AddToBag(const FArcRmlDemoItem& Item);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SetBagSlot(int32 Index, const FArcRmlDemoItem& Item);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void UseMaterial(const FString& ItemName, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SetEquipped(const FString& EquipSlot, const FArcRmlDemoItem& Item);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	bool SpendGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void AddGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SetMultiplayer(bool bInMultiplayer);

	void SetBag(const TArray<FArcRmlDemoItem>& InBag);
	void SetAppliedSettings(const TArray<FArcRmlDemoAppliedSetting>& InSettings);
	void SetSaveSlot(int32 Index, const FArcRmlDemoSaveSlot& Slot);
	void SetSaveNotice(const FString& SlotLabel);

	int32 FirstFree() const;
	int32 HaveOf(const FString& ItemName) const;
	const FArcRmlDemoItem* FindEquipped(const FString& EquipSlot) const;
	const FArcRmlDemoAppliedSetting* FindAppliedSetting(const FString& SettingLabel) const;

	static FString FormatNumber(int64 Value);
	FArcRmlDemoCurrencyRow Coins(const FString& Value, bool bInsufficient = false) const;
	FArcRmlDemoCurrencyRow ShardsRow(const FString& Value) const;
	static FString RarityLabel(const FString& InRarity);
	static int32 RarityRank(const FString& InRarity);
	static FString SlotLabel(const FString& InSlot);
	FArcRmlDemoItemTooltip TipOf(const FArcRmlDemoItem& Item) const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoItem> Bag;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoEquipSlot> Equipment;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Shards = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bMultiplayer = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoAppliedSetting> AppliedSettings;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoSaveSlot> SaveSlots;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString SaveNotice;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 BagSize = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoItem> StartingBag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoEquipSlot> StartingEquipment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 StartingGold = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 StartingShards = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoSaveSlot> StartingSaveSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoAppliedSetting> StartingAppliedSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FArcRmlImageRef GoldIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FArcRmlImageRef ShardIcon;
};
