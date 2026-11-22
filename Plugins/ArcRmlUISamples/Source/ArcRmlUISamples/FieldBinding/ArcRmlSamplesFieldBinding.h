// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesFieldBinding.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesFieldBindingViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Count = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	float Progress = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	bool bEven = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FText Message = INVTEXT("Not clicked yet");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FLinearColor Accent = FLinearColor(0.2f, 0.6f, 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Increment();
};

namespace ArcRmlSamplesFieldBinding
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
