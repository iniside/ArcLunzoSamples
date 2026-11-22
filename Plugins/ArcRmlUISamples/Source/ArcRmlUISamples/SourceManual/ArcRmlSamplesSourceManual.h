// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesSourceManual.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesSourceManualViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Samples")
	FString Label;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Samples")
	int32 Generation = 1;

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void Swap(UObject* WorldContextObject);
};

namespace ArcRmlSamplesSourceManual
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
