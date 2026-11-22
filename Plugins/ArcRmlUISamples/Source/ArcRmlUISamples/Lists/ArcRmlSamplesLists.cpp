// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Lists/ArcRmlSamplesLists.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesLists)

UArcRmlSamplesListsViewModel::UArcRmlSamplesListsViewModel()
{
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FArcRmlSamplesListsRow Row;
		Row.Name = FString::Printf(TEXT("Item %d"), NextNumber);
		Row.Quantity = NextNumber;
		++NextNumber;
		Items.Add(Row);
	}
}

void UArcRmlSamplesListsViewModel::Add()
{
	FArcRmlSamplesListsRow Row;
	Row.Name = FString::Printf(TEXT("Item %d"), NextNumber);
	Row.Quantity = NextNumber;
	++NextNumber;
	Items.Add(Row);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Items);
}

void UArcRmlSamplesListsViewModel::Remove(int32 Index)
{
	if (Items.IsValidIndex(Index))
	{
		Items.RemoveAt(Index);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Items);
	}
}

namespace ArcRmlSamplesLists
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Lists/Lists.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}
