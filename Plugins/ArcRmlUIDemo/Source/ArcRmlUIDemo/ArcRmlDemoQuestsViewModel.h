// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoQuestsViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoQuest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Region;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Progress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bTracked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString State = TEXT("active");
};

USTRUCT(BlueprintType)
struct FArcRmlDemoQuestCategory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString KindLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoObjectiveRow> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoQuest> Quests;
};

UCLASS()
class UArcRmlDemoQuestsViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Details(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Track(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ShowOnMap(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Abandon(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Choose(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Tabs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 SubTab = 1;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bConfirm = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoQuestRow> Quests;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Selected = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString DetailSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlImageRef DetailIcon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FString> DetailBody;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoObjectiveRow> Objectives;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRewardRow> RewardChips;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRewardItemRow> RewardItems;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bDone = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanAbandon = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bHasQuest = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanTrack = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString TrackLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString TrackVariant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> ModalButtons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoQuestCategory> Categories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 TrackMax = 0;

private:
	bool IsDone() const;
	bool CanAbandon() const;
	int32 TrackedCount() const;
	FArcRmlDemoQuest* FindQuest(int32 Index);
	void SelectQuest(int32 Index);
	void Refresh();

	TArray<TArray<FArcRmlDemoQuest>> Lists;
};
