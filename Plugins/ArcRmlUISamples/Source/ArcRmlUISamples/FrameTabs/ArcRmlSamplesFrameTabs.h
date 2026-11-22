// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesFrameTabs.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesFrameTabsFrameViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Shown = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void ShowTab(UObject* WorldContextObject, int32 Index);
};

UCLASS()
class UArcRmlSamplesFrameTabsTabViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Count = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Add();
};

namespace ArcRmlSamplesFrameTabs
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
