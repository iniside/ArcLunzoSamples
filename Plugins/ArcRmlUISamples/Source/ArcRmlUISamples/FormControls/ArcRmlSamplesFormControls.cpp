// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "FormControls/ArcRmlSamplesFormControls.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesFormControls)

namespace ArcRmlSamplesFormControls
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/FormControls/FormControls.rml")};

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

void UArcRmlSamplesFormControlsViewModel::SetSubtitles(bool bValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(bSubtitles, bValue);
}

void UArcRmlSamplesFormControlsViewModel::SetClassName(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(ClassName, Value);
}

void UArcRmlSamplesFormControlsViewModel::SetVolume(int32 Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Volume, Value);
}

void UArcRmlSamplesFormControlsViewModel::SetQuality(const FString& Value)
{
	UE_MVVM_SET_PROPERTY_VALUE(Quality, Value);
}
