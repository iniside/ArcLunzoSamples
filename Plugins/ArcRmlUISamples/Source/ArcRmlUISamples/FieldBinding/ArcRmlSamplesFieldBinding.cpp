// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "FieldBinding/ArcRmlSamplesFieldBinding.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesFieldBinding)

void UArcRmlSamplesFieldBindingViewModel::Increment()
{
	const FLinearColor Accents[] = {FLinearColor(0.2f, 0.6f, 1.0f), FLinearColor(1.0f, 0.55f, 0.1f), FLinearColor(0.3f, 0.85f, 0.4f)};

	const int32 NewCount = Count + 1;
	UE_MVVM_SET_PROPERTY_VALUE(Count, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(Progress, (NewCount % 11) / 10.0f);
	UE_MVVM_SET_PROPERTY_VALUE(bEven, NewCount % 2 == 0);
	UE_MVVM_SET_PROPERTY_VALUE(Message, FText::Format(INVTEXT("Clicked {0} times"), NewCount));
	UE_MVVM_SET_PROPERTY_VALUE(Accent, Accents[NewCount % UE_ARRAY_COUNT(Accents)]);
}

namespace ArcRmlSamplesFieldBinding
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/FieldBinding/FieldBinding.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
