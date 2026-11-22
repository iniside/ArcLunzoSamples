// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Component/ArcRmlSamplesComponent.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesComponent)

void UArcRmlSamplesComponentViewModel::Increment()
{
	const int32 NewCount = Count + 1;
	UE_MVVM_SET_PROPERTY_VALUE(Count, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(Title, FText::Format(INVTEXT("Bound title, pressed {0} times"), NewCount));
}

namespace ArcRmlSamplesComponent
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Component/Component.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
