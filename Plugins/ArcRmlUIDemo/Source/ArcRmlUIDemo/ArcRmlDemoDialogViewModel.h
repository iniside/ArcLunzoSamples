// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoViewModelBase.h"

#include "ArcRmlDemoDialogViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoDialogSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	FString Variant;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> Buttons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	int32 CancelIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ArcRmlDemo")
	int32 FocusIndex = 0;
};

UCLASS()
class UArcRmlDemoDialogViewModel : public UArcRmlDemoViewModelBase
{
	GENERATED_BODY()

public:
	void Configure(const FArcRmlDemoDialogSpec& Spec, TFunction<void(int32)> InOnChoose);
	void SetBody(const FString& InBody);
	void SetCountdown(float Pct, const FString& Label, const FString& Value);
	TFunction<void(int32)> TakeCallback();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Choose(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Closed();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Body;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Variant;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> Buttons;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Focused = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCountdown = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	float CountdownPct = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString CountdownLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString CountdownValue;

private:
	TFunction<void(int32)> OnChoose;
	int32 CancelIndex = 0;
};
