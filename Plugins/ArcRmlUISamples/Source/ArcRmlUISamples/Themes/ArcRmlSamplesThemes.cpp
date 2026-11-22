// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Themes/ArcRmlSamplesThemes.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesThemes)

void UArcRmlSamplesThemesViewModel::SetTheme(const FString& Name)
{
	UE_MVVM_SET_PROPERTY_VALUE(Theme, Name);
}

namespace ArcRmlSamplesThemes
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Themes/Themes.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
