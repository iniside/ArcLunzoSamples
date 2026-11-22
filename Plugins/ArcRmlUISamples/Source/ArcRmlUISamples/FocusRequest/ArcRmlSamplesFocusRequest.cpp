// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "FocusRequest/ArcRmlSamplesFocusRequest.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesFocusRequest)

namespace ArcRmlSamplesFocusRequest
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/FocusRequest/FocusRequest.rml")};

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


void UArcRmlSamplesFocusRequestViewModel::FocusName()
{
	UE_MVVM_SET_PROPERTY_VALUE(NameFocusSerial, NameFocusSerial + 1);
}

void UArcRmlSamplesFocusRequestViewModel::FocusEmail()
{
	UE_MVVM_SET_PROPERTY_VALUE(EmailFocusSerial, EmailFocusSerial + 1);
}

void UArcRmlSamplesFocusRequestViewModel::FocusCode()
{
	UE_MVVM_SET_PROPERTY_VALUE(CodeFocusSerial, CodeFocusSerial + 1);
}
