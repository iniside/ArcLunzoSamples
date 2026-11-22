// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesUmgWidget.generated.h"

class ULocalPlayer;
class UUserWidget;

UCLASS()
class UArcRmlSamplesUmgWidgetViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Count = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Increment();

	UFUNCTION()
	void HandleCloseRequested();

	TWeakObjectPtr<UUserWidget> Host;
};

namespace ArcRmlSamplesUmgWidget
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
	void Close(ULocalPlayer* LocalPlayer);
	bool IsOpen(ULocalPlayer* LocalPlayer);
}
