// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "VirtualList/ArcRmlSamplesVirtualList.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesVirtualList)

namespace ArcRmlSamplesVirtualList
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/VirtualList/VirtualList.rml")};

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

UArcRmlSamplesVirtualListViewModel::UArcRmlSamplesVirtualListViewModel()
{
	constexpr int32 RowCount = 10000;
	Rows.Reserve(RowCount);
	for (int32 Index = 0; Index < RowCount; ++Index)
	{
		Rows.Add(Index + 1);
	}
}
