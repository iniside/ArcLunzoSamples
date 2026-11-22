// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesAnimations.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesAnimationsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Hits = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Hit();
};

namespace ArcRmlSamplesAnimations
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
