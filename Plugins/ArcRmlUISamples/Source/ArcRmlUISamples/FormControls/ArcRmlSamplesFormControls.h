// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesFormControls.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesFormControlsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	bool bSubtitles = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString ClassName = TEXT("warrior");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Volume = 50;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Quality = TEXT("medium");

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetSubtitles(bool bValue);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetClassName(const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetVolume(int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetQuality(const FString& Value);
};

namespace ArcRmlSamplesFormControls
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
