// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Events/ArcRmlSamplesEvents.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesEvents)

void UArcRmlSamplesEventsViewModel::Pick(const FString& Colour, int32 Amount)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastEvent, FText::Format(INVTEXT("Picked {0} x {1}"), Amount, FText::FromString(Colour)));
}

void UArcRmlSamplesEventsViewModel::Pointer(const FString& Button)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastEvent, FText::Format(INVTEXT("Mouse button: {0}"), FText::FromString(Button)));
}

void UArcRmlSamplesEventsViewModel::Step(int32 Delta)
{
	UE_MVVM_SET_PROPERTY_VALUE(Value, Value + Delta);
	UE_MVVM_SET_PROPERTY_VALUE(LastEvent, FText::Format(INVTEXT("Key step {0}"), Delta));
}

void UArcRmlSamplesEventsViewModel::Inner()
{
	UE_MVVM_SET_PROPERTY_VALUE(InnerCount, InnerCount + 1);
}

void UArcRmlSamplesEventsViewModel::Outer()
{
	UE_MVVM_SET_PROPERTY_VALUE(OuterCount, OuterCount + 1);
}

void UArcRmlSamplesEventsViewModel::ArmBack()
{
	UE_MVVM_SET_PROPERTY_VALUE(bBackArmed, true);
	UE_MVVM_SET_PROPERTY_VALUE(LastEvent, INVTEXT("Back pressed: press it again to close"));
}

void UArcRmlSamplesEventsViewModel::ShowCaller(UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	UE_MVVM_SET_PROPERTY_VALUE(LastEvent, FText::Format(INVTEXT("Called by {0} at {1} s"), FText::FromString(WorldContextObject->GetName()), FMath::FloorToInt(World->GetTimeSeconds())));
}

namespace ArcRmlSamplesEvents
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Events/Events.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
