// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesMenu.generated.h"

class ULocalPlayer;

USTRUCT(BlueprintType)
struct FArcRmlSamplesMenuEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Name;
};

UCLASS()
class UArcRmlSamplesMenuViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesMenuViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TArray<FArcRmlSamplesMenuEntry> Samples;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TArray<FArcRmlSamplesMenuEntry> BlueprintSamples;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void OpenSample(UObject* WorldContextObject, const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void OpenBlueprintSample(UObject* WorldContextObject, const FString& Name);
};

#if !UE_BUILD_SHIPPING
namespace ArcRmlSamplesMenu
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
#endif
