// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesComponent.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesComponentViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FText Title = INVTEXT("Bound title");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Count = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Increment();
};

namespace ArcRmlSamplesComponent
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
