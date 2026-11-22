// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "SourceManual/ArcRmlSamplesSourceManual.h"

#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesSourceManual)

namespace ArcRmlSamplesSourceManualPrivate
{
	UArcRmlViewportSubsystem* FindViewport(UObject* WorldContextObject)
	{
		const APlayerController* Player = Cast<APlayerController>(WorldContextObject);
		ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
	}
}

void UArcRmlSamplesSourceManualViewModel::Swap(UObject* WorldContextObject)
{
	UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesSourceManualPrivate::FindViewport(WorldContextObject);
	if (!Viewport)
	{
		return;
	}

	UArcRmlSamplesSourceManualViewModel* Next = NewObject<UArcRmlSamplesSourceManualViewModel>(GetTransientPackage());
	Next->Generation = Generation + 1;
	Next->Label = FString::Printf(TEXT("Instance %d"), Next->Generation);
	Viewport->SetViewModel(FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/SourceManual/SourceManual.rml")}, TEXT("item"), Next);
}

namespace ArcRmlSamplesSourceManual
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/SourceManual/SourceManual.rml")};
		UArcRmlSamplesSourceManualViewModel* First = NewObject<UArcRmlSamplesSourceManualViewModel>(GetTransientPackage());
		First->Label = TEXT("Instance 1");
		if (!Viewport->SetViewModel(Document, TEXT("item"), First))
		{
			return FArcRmlDocumentRef();
		}

		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
