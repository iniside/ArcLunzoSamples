// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoViewModelResolver.h"

#include "ArcRmlDemoSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "MVVMViewModelBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoViewModelResolver)

UObject* UArcRmlDemoViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!ExpectedType || !UserWidget || ExpectedType->HasAnyClassFlags(CLASS_Abstract) || !ExpectedType->IsChildOf(UMVVMViewModelBase::StaticClass()))
	{
		return nullptr;
	}
	const ULocalPlayer* LocalPlayer = UserWidget->GetOwningLocalPlayer();
	UArcRmlDemoSubsystem* Demo = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlDemoSubsystem>() : nullptr;
	return Demo && Demo->IsRunning() ? Demo->FindOrCreateScreenModel(ExpectedType) : nullptr;
}

void UArcRmlDemoViewModelResolver::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	// Instances are per player for the demo session; the subsystem releases them at Stop.
}
