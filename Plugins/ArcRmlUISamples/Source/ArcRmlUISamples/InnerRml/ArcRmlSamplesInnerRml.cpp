// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "InnerRml/ArcRmlSamplesInnerRml.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesInnerRml)

namespace ArcRmlSamplesInnerRml
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/InnerRml/InnerRml.rml")};

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}

void UArcRmlSamplesInnerRmlViewModel::SetTyped(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Typed, Value);
}
