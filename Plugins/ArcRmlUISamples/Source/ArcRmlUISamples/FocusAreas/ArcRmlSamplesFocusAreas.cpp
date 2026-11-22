// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "FocusAreas/ArcRmlSamplesFocusAreas.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesFocusAreas)

void UArcRmlSamplesFocusAreasViewModel::NoTarget(const FString& InDirection, const FString& InArea, const FString& InTargetArea)
{
	Show(InDirection, InArea, InTargetArea, false);
}

void UArcRmlSamplesFocusAreasViewModel::Cancelled(const FString& InDirection, const FString& InArea, const FString& InTargetArea)
{
	Show(InDirection, InArea, InTargetArea, true);
}

void UArcRmlSamplesFocusAreasViewModel::Show(const FString& InDirection, const FString& InArea, const FString& InTargetArea, bool bInCancelled)
{
	UE_MVVM_SET_PROPERTY_VALUE(Direction, InDirection);
	UE_MVVM_SET_PROPERTY_VALUE(Area, InArea);
	UE_MVVM_SET_PROPERTY_VALUE(TargetArea, InTargetArea);
	UE_MVVM_SET_PROPERTY_VALUE(bCancelled, bInCancelled);
	UE_MVVM_SET_PROPERTY_VALUE(EventCount, EventCount + 1);
}

namespace ArcRmlSamplesFocusAreas
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/FocusAreas/FocusAreas.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
