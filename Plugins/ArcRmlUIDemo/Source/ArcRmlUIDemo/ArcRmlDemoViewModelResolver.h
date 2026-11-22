// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"

#include "ArcRmlDemoViewModelResolver.generated.h"

UCLASS()
class UArcRmlDemoViewModelResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
