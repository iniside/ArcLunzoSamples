// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "GamepadMenu/ArcRmlSamplesGamepadMenu.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesGamepadMenu)

void UArcRmlSamplesGamepadMenuViewModel::Choose(const FString& Item)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastChoice, FText::FromString(Item));
}

void UArcRmlSamplesGamepadMenuViewModel::ToggleInfo()
{
	UE_MVVM_SET_PROPERTY_VALUE(bInfoShown, !bInfoShown);
}

namespace ArcRmlSamplesGamepadMenu
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/GamepadMenu/GamepadMenu.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
