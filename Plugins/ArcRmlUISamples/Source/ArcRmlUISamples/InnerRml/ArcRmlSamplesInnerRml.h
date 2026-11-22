// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesInnerRml.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesInnerRmlViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Banner = TEXT("Welcome back, <span class=\"ir-gold\">traveller</span>");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Typed = TEXT("<b>x</b>");

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetTyped(const FString& Value);
};

namespace ArcRmlSamplesInnerRml
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
