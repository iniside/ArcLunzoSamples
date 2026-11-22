// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesUmgOwnerData.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesUmgOwnerDataViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Health = 40;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Heal();
};

UCLASS()
class UArcRmlSamplesUmgOwnerDataWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	TObjectPtr<UArcRmlSamplesUmgOwnerDataViewModel> Stats;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleCloseRequested();
};

namespace ArcRmlSamplesUmgOwnerData
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
	void Close(ULocalPlayer* LocalPlayer);
	bool IsOpen(ULocalPlayer* LocalPlayer);
}
