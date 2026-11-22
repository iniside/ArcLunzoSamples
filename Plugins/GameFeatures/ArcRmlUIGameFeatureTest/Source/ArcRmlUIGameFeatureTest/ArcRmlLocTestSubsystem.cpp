// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlLocTestSubsystem.h"

#include "ArcRmlLocTestViewModel.h"
#include "Engine/GameInstance.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlLocTestSubsystem)

namespace ArcRmlLocTestSubsystemPrivate
{
	FMVVMViewModelContext MakeContext()
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = UArcRmlLocTestViewModel::StaticClass();
		Context.ContextName = TEXT("LocTest");
		return Context;
	}

	UMVVMViewModelCollectionObject* FindCollection(const UGameInstance* GameInstance)
	{
		UMVVMGameSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
		return Subsystem ? Subsystem->GetViewModelCollection() : nullptr;
	}
}

void UArcRmlLocTestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UMVVMGameSubsystem>();

	ViewModel = NewObject<UArcRmlLocTestViewModel>(this);
	ViewModel->Initialize();
	if (UMVVMViewModelCollectionObject* ViewModels = ArcRmlLocTestSubsystemPrivate::FindCollection(GetGameInstance()))
	{
		ViewModels->AddViewModelInstance(ArcRmlLocTestSubsystemPrivate::MakeContext(), ViewModel);
	}
}

void UArcRmlLocTestSubsystem::Deinitialize()
{
	if (UMVVMViewModelCollectionObject* ViewModels = ArcRmlLocTestSubsystemPrivate::FindCollection(GetGameInstance()))
	{
		ViewModels->RemoveViewModel(ArcRmlLocTestSubsystemPrivate::MakeContext());
	}
	ViewModel->Shutdown();
	ViewModel = nullptr;
	Super::Deinitialize();
}
