// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "TabBar/ArcRmlSamplesTabBar.h"

#include "Engine/LocalPlayer.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesTabBar)

namespace ArcRmlSamplesTabBar
{
	const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/TabBar/TabBar.rml")};

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

void UArcRmlSamplesTabBarViewModel::Select(int32 Index)
{
	UE_MVVM_SET_PROPERTY_VALUE(Selected, Index);
}
