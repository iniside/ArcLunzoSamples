// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Sounds/ArcRmlSamplesSounds.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesSounds)

void UArcRmlSamplesSoundsViewModel::Click()
{
	UE_MVVM_SET_PROPERTY_VALUE(Clicks, Clicks + 1);
}

namespace ArcRmlSamplesSounds
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Sounds/Sounds.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
