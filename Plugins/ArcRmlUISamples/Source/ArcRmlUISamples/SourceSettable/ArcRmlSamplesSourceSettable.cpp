// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "SourceSettable/ArcRmlSamplesSourceSettable.h"

#include "Engine/GameInstance.h"
#include "MVVMGameSubsystem.h"
#include "Types/MVVMViewModelCollection.h"
#include "Types/MVVMViewModelContext.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesSourceSettable)

namespace ArcRmlSamplesSourceSettablePrivate
{
	UArcRmlViewportSubsystem* FindViewport(UObject* WorldContextObject)
	{
		const APlayerController* Player = Cast<APlayerController>(WorldContextObject);
		ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
	}
}

void UArcRmlSamplesSourceSettableViewModel::Replace(UObject* WorldContextObject)
{
	UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesSourceSettablePrivate::FindViewport(WorldContextObject);
	if (!Viewport)
	{
		return;
	}

	UArcRmlSamplesSourceSettableViewModel* Next = NewObject<UArcRmlSamplesSourceSettableViewModel>(GetTransientPackage());
	Next->Generation = Generation + 1;
	Next->Label = FString::Printf(TEXT("Replacement %d"), Next->Generation);
	Viewport->SetViewModel(FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/SourceSettable/SourceSettable.rml")}, TEXT("item"), Next);
}

namespace ArcRmlSamplesSourceSettable
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/SourceSettable/SourceSettable.rml")};
		UMVVMGameSubsystem* Mvvm = LocalPlayer->GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
		if (!Mvvm)
		{
			return FArcRmlDocumentRef();
		}

		FMVVMViewModelContext Context;
		Context.ContextClass = UArcRmlSamplesSourceSettableViewModel::StaticClass();
		Context.ContextName = TEXT("VM_SampleSettable");
		Mvvm->GetViewModelCollection()->RemoveViewModel(Context);
		UArcRmlSamplesSourceSettableViewModel* Instance = NewObject<UArcRmlSamplesSourceSettableViewModel>(Mvvm);
		Instance->Label = TEXT("From the GameInstance collection");
		Mvvm->GetViewModelCollection()->AddViewModelInstance(Context, Instance);

		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
