// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoMatchResultsViewModel.generated.h"

UCLASS()
class UArcRmlDemoMatchResultsViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("results");
	}

	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void PlayAgain();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Continue();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bWin = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ModeLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Banner;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 ScoreA = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 ScoreB = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FString> Columns;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTeamRow> Teams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString MvpInitials;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString MvpName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString MvpSub;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoFactRow> MvpStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Xp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RankLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Rp = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RpLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RpTone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRewardRow> Rewards;
};
