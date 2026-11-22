// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "RowHotkeys/ArcRmlSamplesRowHotkeys.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesRowHotkeys)

namespace ArcRmlSamplesRowHotkeys
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/RowHotkeys/RowHotkeys.rml")};

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


void UArcRmlSamplesRowHotkeysViewModel::SetFavourite(int32 Index)
{
	UE_MVVM_SET_PROPERTY_VALUE(Favourite, Index);
}

void UArcRmlSamplesRowHotkeysViewModel::Use(const FString& Name)
{
	UE_MVVM_SET_PROPERTY_VALUE(Used, Name);
}
