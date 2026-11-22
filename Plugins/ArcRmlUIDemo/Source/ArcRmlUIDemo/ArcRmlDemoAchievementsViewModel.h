// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"
#include "ArcRmlStructList.h"

#include "ArcRmlDemoAchievementsViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoAchievement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Max = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Date;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	float Pct = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Points = 0;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoAchievementFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> States;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoAchievementSort
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString SortedBy;
};

UCLASS()
class UArcRmlDemoAchievementsViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	UArcRmlDemoAchievementsViewModel();

	virtual void Initialize() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Details(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Pin(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void CycleSort();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SelectCategory(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void FilterBy(int32 Index);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlStructList Achievements;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoChipRow> Chips;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString SortLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString NextSortLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ShownSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Category = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCategoryRow> Categories;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoFactRow> Summary;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 RingPct = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RingLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoAchievement> AllAchievements;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoAchievementFilter> Filters;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoAchievementSort> SortOptions;

private:
	bool Matches(const FArcRmlDemoAchievementFilter& Filter, const FArcRmlDemoAchievement& Achievement) const;
	TArray<int32> Shown() const;
	void Refresh();

	int32 FilterIndex = 0;
	int32 SortIndex = 0;
	TSet<FString> Pinned;
};
