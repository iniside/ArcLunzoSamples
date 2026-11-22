// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlLocTestViewModel.generated.h"

/** View model of the Localization test page: shows the current culture and switches it. */
UCLASS()
class UArcRmlLocTestViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize();
	void Shutdown();

	/** Sets the engine culture. The debug cultures LEET and keys exist only in non-Shipping builds. */
	UFUNCTION(BlueprintCallable, Category = "ArcRmlLocTest")
	void SwitchCulture(const FString& Name);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlLocTest")
	FString Culture;

private:
	void HandleCultureChanged();

	FDelegateHandle CultureChangedHandle;
};
