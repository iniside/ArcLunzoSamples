// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesSounds.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesSoundsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Clicks = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Click();
};

namespace ArcRmlSamplesSounds
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
