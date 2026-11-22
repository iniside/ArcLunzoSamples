// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesLocalization.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesLocalizationViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Culture;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Coins = 1;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SwitchCulture(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void AddCoin();

private:
	void HandleCultureChanged();

	FDelegateHandle CultureChangedHandle;
};

namespace ArcRmlSamplesLocalization
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
