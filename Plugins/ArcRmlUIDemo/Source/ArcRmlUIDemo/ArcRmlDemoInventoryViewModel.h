// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"
#include "ArcRmlStructList.h"

#include "ArcRmlDemoInventoryViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoInventoryCategory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;
};

UCLASS()
class UArcRmlDemoInventoryViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Activate(int32 Index);

	UArcRmlDemoInventoryViewModel();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Use(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Move(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Swap(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Drop(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Junk(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void FilterBy(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void CycleSort();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ShowAll();

	void ShowCategory(const FString& Key);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlStructList Slots;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoChipRow> Chips;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoItemTooltip Item;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoItemTooltip Equipped;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bShowTip = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	float WeightPct = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString WeightLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString WeightTone;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString BagSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bHasItems = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bEmpty = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bShowsAll = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bMoving = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoToast MoveToast;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Query;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString SortLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString NextSortLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Selected = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoInventoryCategory> Categories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FString> SortOptions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 LockedSlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 WeightMax = 0;

protected:
	virtual void GameFieldChanged(FName FieldName) override;
	virtual void WriteText(const FString& Field, const FString& Value) override;

private:
	struct FCell
	{
		int32 Pos = INDEX_NONE;
		bool bLocked = false;
	};

	struct FView
	{
		TArray<FCell> Cells;
		bool bAll = true;
		bool bEmpty = false;
	};

	FView BuildView() const;
	int32 PosAt(int32 Index) const;
	const FArcRmlDemoItem* BagItem(int32 Pos) const;
	FString EquipSlotFor(const FArcRmlDemoItem& Equipping) const;
	void Refresh();

	int32 CategoryIndex = 0;
	int32 SortIndex = 0;
	int32 SelectedPos = 0;
	int32 MoveFrom = INDEX_NONE;
	TSet<FString> JunkIds;
};
