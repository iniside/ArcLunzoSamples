// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoSaveLoadViewModel.generated.h"

UCLASS()
class UArcRmlDemoSaveLoadViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("saveload");
	}

	virtual void Initialize() override;
	virtual void InitScreen(const FString& Param) override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Activate(int32 Slot);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Primary();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Delete();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Choose(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Selected(const FString& Field, int32 Index);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Mode = TEXT("load");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoSaveSlotRow> Slots;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 SelectedSlot = 1;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FString> Crumbs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString CountLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bConfirm = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ConfirmBody;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> ConfirmButtons;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailSlotLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoFactRow> DetailStats;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bNoDelete = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bNoPrimary = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString PrimaryLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FArcRmlDemoSaveSlot NewSave;

protected:
	virtual void GameFieldChanged(FName FieldName) override;

private:
	const TArray<FArcRmlDemoSaveSlot>& GetSlots() const;
	void Refresh();
};
