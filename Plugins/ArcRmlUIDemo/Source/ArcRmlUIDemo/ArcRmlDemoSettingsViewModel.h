// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoDialogViewModel.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoSettingsViewModel.generated.h"

UENUM(BlueprintType)
enum class EArcRmlDemoSettingType : uint8
{
	Text,
	Stepper,
	Toggle,
	Radio,
	Slider,
	Dropdown,
	Checkbox,
	Keybind
};

UENUM(BlueprintType)
enum class EArcRmlDemoSettingFormat : uint8
{
	None,
	Percent,
	Fps,
	Tenth
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSettingValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Number = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Text;

	bool operator==(const FArcRmlDemoSettingValue& Other) const = default;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSettingDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Tab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Group;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	EArcRmlDemoSettingType Type = EArcRmlDemoSettingType::Toggle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlDemoSettingValue Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Min = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Max = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Step = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	EArcRmlDemoSettingFormat Format = EArcRmlDemoSettingFormat::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString CheckLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Long;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString DefaultLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Impact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString TagTone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Placeholder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Preview;
};

UCLASS()
class UArcRmlDemoSettingsViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("settings");
	}

	virtual void InitScreen(const FString& Param) override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Activate(const FString& Field);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Stepped(const FString& Field, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SliderInput(const FString& Field, const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Selected(const FString& Field, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void CloseDropdown(const FString& Field);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void FieldReset(const FString& Field);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void KeyBound(const FString& Field, const FString& Key);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void KeybindCancel(const FString& Field);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Apply();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Tab = TEXT("graphics");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Tabs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 ActiveTab = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoSettingGroup> Groups;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FString> Crumbs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Query;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ChangesLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 SelectedRow = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bDropdownOpen = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bListening = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanReset = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanApply = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailLong;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailPreview;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailCurrent;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailDefault;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailImpact;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailImpactTone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoTabRow> TabDefs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoSettingDef> Settings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString ConfirmTab;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 ConfirmSeconds = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FArcRmlDemoDialogSpec UnsavedDialog;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FArcRmlDemoDialogSpec ConfirmDialog;

protected:
	virtual void WriteText(const FString& Field, const FString& Value) override;

private:
	struct FDropdown
	{
		FString Label;
	};

	TArray<int32> TabRows() const;
	int32 FindSetting(const FString& Label) const;
	int32 RowOf(const FString& Label) const;
	FArcRmlDemoSettingValue DraftOf(const FArcRmlDemoSettingDef& Setting) const;
	FArcRmlDemoSettingValue AppliedOf(const FArcRmlDemoSettingDef& Setting) const;
	int32 CountChanges(const FString& OnlyTab = FString()) const;
	void LoadDraft();
	void Touch(const FString& Label);
	void StepSetting(const FArcRmlDemoSettingDef& Setting, int32 Direction);
	void ResetDropdown();
	void StopListening();
	void SelectTab(int32 Index);
	void ApplyChanges(bool bLeave);
	void CommitDraft();
	void RevertTab(const FString& RevertedTab);
	void OpenConfirmDialog(bool bLeave);
	void Refresh();
	FArcRmlDemoSettingRow MakeRow(const FArcRmlDemoSettingDef& Setting, int32 RowIndex, const TMap<FString, TArray<FString>>& KeyUse) const;
	FString ValueLabel(const FArcRmlDemoSettingDef& Setting, const FArcRmlDemoSettingValue& Value) const;
	FString CrumbRoot() const;

	TMap<FString, FArcRmlDemoSettingValue> Draft;
	TOptional<FDropdown> Dropdown;
	FString Listen;
	int32 ConfirmLeft = 0;
};
