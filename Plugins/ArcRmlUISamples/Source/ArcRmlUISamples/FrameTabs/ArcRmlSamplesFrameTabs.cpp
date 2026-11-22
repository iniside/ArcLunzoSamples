// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "FrameTabs/ArcRmlSamplesFrameTabs.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesFrameTabs)

namespace ArcRmlSamplesFrameTabs
{
	const FArcRmlDocumentRef Frame{TEXT("/ArcRmlUISamples/UI/Rml/FrameTabs/Frame.rml")};
	const FArcRmlDocumentRef Tabs[] = {
		FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/FrameTabs/TabA.rml")},
		FArcRmlDocumentRef{TEXT("/ArcRmlUISamples/UI/Rml/FrameTabs/TabB.rml")},
	};

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}
		// Opening a tab opens its frame; the frame stays open while the tabs switch, so it is the document to close.
		return Viewport->OpenDocument(Tabs[0]) ? Frame : FArcRmlDocumentRef();
	}
}

void UArcRmlSamplesFrameTabsFrameViewModel::ShowTab(UObject* WorldContextObject, int32 Index)
{
	const APlayerController* Player = Cast<APlayerController>(WorldContextObject);
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
	if (!Viewport || Index < 0 || Index >= UE_ARRAY_COUNT(ArcRmlSamplesFrameTabs::Tabs))
	{
		return;
	}
	if (Viewport->OpenDocument(ArcRmlSamplesFrameTabs::Tabs[Index]))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Shown, Index);
	}
}

void UArcRmlSamplesFrameTabsTabViewModel::Add()
{
	UE_MVVM_SET_PROPERTY_VALUE(Count, Count + 1);
}
