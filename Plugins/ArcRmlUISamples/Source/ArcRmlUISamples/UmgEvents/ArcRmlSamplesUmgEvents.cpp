// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "UmgEvents/ArcRmlSamplesUmgEvents.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace ArcRmlSamplesUmgEventsPrivate
{
	const TCHAR* WidgetClassPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgEvents/WBP_Sample_UmgEvents.WBP_Sample_UmgEvents_C");
	const TCHAR* DocumentPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgEvents/UmgEvents.rml");
	constexpr int32 ZOrder = 1500;

	TWeakObjectPtr<UUserWidget> OpenWidget;
}

namespace ArcRmlSamplesUmgEvents
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		if (ArcRmlSamplesUmgEventsPrivate::OpenWidget.IsValid())
		{
			Close(LocalPlayer);
		}
		APlayerController* Player = LocalPlayer ? LocalPlayer->GetPlayerController(nullptr) : nullptr;
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, ArcRmlSamplesUmgEventsPrivate::WidgetClassPath);
		if (!Player || !WidgetClass)
		{
			return FArcRmlDocumentRef();
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(Player, WidgetClass);
		if (!Widget)
		{
			return FArcRmlDocumentRef();
		}
		Widget->AddToPlayerScreen(ArcRmlSamplesUmgEventsPrivate::ZOrder);
		ArcRmlSamplesUmgEventsPrivate::OpenWidget = Widget;
		return FArcRmlDocumentRef{ArcRmlSamplesUmgEventsPrivate::DocumentPath};
	}

	void Close(ULocalPlayer* LocalPlayer)
	{
		if (UUserWidget* Widget = ArcRmlSamplesUmgEventsPrivate::OpenWidget.Get())
		{
			Widget->RemoveFromParent();
		}
		ArcRmlSamplesUmgEventsPrivate::OpenWidget.Reset();
	}

	bool IsOpen(ULocalPlayer* LocalPlayer)
	{
		const UUserWidget* Widget = ArcRmlSamplesUmgEventsPrivate::OpenWidget.Get();
		return Widget && Widget->IsInViewport();
	}
}
