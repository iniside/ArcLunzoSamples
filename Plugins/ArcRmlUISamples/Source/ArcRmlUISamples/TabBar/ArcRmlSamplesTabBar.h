// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesTabBar.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesTabBarViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Selected = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Select(int32 Index);
};

namespace ArcRmlSamplesTabBar
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
