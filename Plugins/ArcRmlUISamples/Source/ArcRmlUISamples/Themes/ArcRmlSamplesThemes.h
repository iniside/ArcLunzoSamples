// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesThemes.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesThemesViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Theme = TEXT("default");

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetTheme(const FString& Name);
};

namespace ArcRmlSamplesThemes
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
