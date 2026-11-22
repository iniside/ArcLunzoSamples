// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoVendorViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoStockItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlDemoItem Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bUnusable = false;
};

UCLASS()
class UArcRmlDemoVendorViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("vendor");
	}

	virtual void Initialize() override;
	virtual void InitScreen(const FString& Param) override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Activate(const FString& List, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Deal();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void BuyMax(const FString& List, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Compare(const FString& List, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Stepped(const FString& Field, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Choose(int32 Index);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Tabs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 ActiveTab = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bConfirm = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> Wallet;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString StockTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString StockSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoStockRow> StockRows;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoItemSlotRow> Bag;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 BagStops = 1;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString BagSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoItemTooltip Item;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString TxTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString TxAction;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString TotalLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString QuantityLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> UnitPrice;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> After;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> Total;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ShortBody;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bBlocked = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanDeal = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanBuyMax = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bComparable = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bQuantityOn = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ModalTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ConfirmBody;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> ModalButtons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoStockItem> Stock;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString StockSubtitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString BuybackSubtitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 BagCells = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 MaxBuyQuantity = 0;

protected:
	virtual void GameFieldChanged(FName FieldName) override;

private:
	struct FDeal
	{
		bool bSell = false;
		FArcRmlDemoItem Item;
		bool bOwned = false;
		int32 BagPos = INDEX_NONE;
		int32 Unit = 0;
		int32 Quantity = 1;
		int32 MaxQuantity = 1;
		int32 Total = 0;
		bool bShort = false;
	};

	bool IsSell() const
	{
		return ActiveTab == 1;
	}

	bool IsBuyback() const
	{
		return ActiveTab == 2;
	}

	FString ItemList() const
	{
		return IsSell() ? TEXT("bag") : TEXT("stock");
	}

	FString ActionLabel() const;
	TArray<int32> BagPositions() const;
	bool FindDeal(FDeal& Out) const;
	bool SelectFrom(const FString& List, int32 Index);
	void OpenConfirm();
	void Refresh();

	TArray<FArcRmlDemoItem> Buyback;
	int32 Selected = 0;
	int32 Quantity = 1;
};
