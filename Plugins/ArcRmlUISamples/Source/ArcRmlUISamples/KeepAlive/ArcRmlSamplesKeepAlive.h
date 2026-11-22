// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesKeepAlive.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesKeepAliveMenuViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void Preload(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void OpenKept(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void OpenPreloaded(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void DiscardBoth(UObject* WorldContextObject);
};

UCLASS()
class UArcRmlSamplesKeepAliveKeptViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Count = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Add();
};

namespace ArcRmlSamplesKeepAlive
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
