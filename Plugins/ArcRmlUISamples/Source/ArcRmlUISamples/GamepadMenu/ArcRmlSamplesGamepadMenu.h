// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesGamepadMenu.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesGamepadMenuViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FText LastChoice = INVTEXT("Nothing yet");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	bool bInfoShown = false;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Choose(const FString& Item);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void ToggleInfo();
};

namespace ArcRmlSamplesGamepadMenu
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
