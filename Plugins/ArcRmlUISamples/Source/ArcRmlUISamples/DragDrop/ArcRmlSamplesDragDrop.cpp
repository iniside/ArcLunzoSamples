// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "DragDrop/ArcRmlSamplesDragDrop.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesDragDrop)

namespace ArcRmlSamplesDragDrop
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/DragDrop/DragDrop.rml")};

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

void UArcRmlSamplesDragDropViewModel::Dropped(const FString& Item, const FString& Target)
{
	if (Target == TEXT("Hand"))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Hand, Item);
	}
	else if (Target == TEXT("Stash0"))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Stash0, Item);
	}
	else if (Target == TEXT("Stash1"))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Stash1, Item);
	}
	else if (Target == TEXT("Stash2"))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Stash2, Item);
	}
}
