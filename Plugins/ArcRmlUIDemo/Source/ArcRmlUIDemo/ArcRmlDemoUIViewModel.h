// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoViewModelBase.h"

#include "ArcRmlDemoUIViewModel.generated.h"

UCLASS()
class UArcRmlDemoUIViewModel : public UArcRmlDemoViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Back();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ToggleDevBar();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void OpenPause();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ToggleGameMenu();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SetTheme(const FString& InTheme);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SetInputScheme(const FString& Scheme);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Jump(const FString& Screen);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ToggleMultiplayer();

	void SetInput(const FString& InInput);
	void SetToast(const FArcRmlDemoToast& InToast);
	void RefreshDevBar(const TArray<FString>& Stack, bool bMultiplayer);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Theme = TEXT("midnight");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Input = TEXT("keyboard");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoToast Toast;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool DevOpen = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoDevBar DevBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoDevRow> JumpTargets;
};
