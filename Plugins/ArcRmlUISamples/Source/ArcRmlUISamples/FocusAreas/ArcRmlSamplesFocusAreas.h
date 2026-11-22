// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesFocusAreas.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesFocusAreasViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 EventCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Direction;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Area;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString TargetArea;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	bool bCancelled = false;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void NoTarget(const FString& InDirection, const FString& InArea, const FString& InTargetArea);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Cancelled(const FString& InDirection, const FString& InArea, const FString& InTargetArea);

private:
	void Show(const FString& InDirection, const FString& InArea, const FString& InTargetArea, bool bInCancelled);
};

namespace ArcRmlSamplesFocusAreas
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
