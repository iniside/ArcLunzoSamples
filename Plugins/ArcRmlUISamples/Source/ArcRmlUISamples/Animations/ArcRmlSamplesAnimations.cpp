// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Animations/ArcRmlSamplesAnimations.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesAnimations)

void UArcRmlSamplesAnimationsViewModel::Hit()
{
	UE_MVVM_SET_PROPERTY_VALUE(Hits, Hits + 1);
}

namespace ArcRmlSamplesAnimations
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Animations/Animations.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
