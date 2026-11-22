// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "UmgLayout/ArcRmlSamplesUmgLayout.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace ArcRmlSamplesUmgLayoutPrivate
{
	const TCHAR* WidgetClassPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgLayout/WBP_Sample_UmgLayout.WBP_Sample_UmgLayout_C");
	const TCHAR* DocumentPath = TEXT("/ArcRmlUISamples/UI/Rml/UmgLayout/UmgLayout.rml");
	constexpr int32 ZOrder = 1500;

	TWeakObjectPtr<UUserWidget> OpenWidget;
}

namespace ArcRmlSamplesUmgLayout
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		if (ArcRmlSamplesUmgLayoutPrivate::OpenWidget.IsValid())
		{
			Close(LocalPlayer);
		}
		APlayerController* Player = LocalPlayer ? LocalPlayer->GetPlayerController(nullptr) : nullptr;
		UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, ArcRmlSamplesUmgLayoutPrivate::WidgetClassPath);
		if (!Player || !WidgetClass)
		{
			return FArcRmlDocumentRef();
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(Player, WidgetClass);
		if (!Widget)
		{
			return FArcRmlDocumentRef();
		}
		Widget->AddToPlayerScreen(ArcRmlSamplesUmgLayoutPrivate::ZOrder);
		ArcRmlSamplesUmgLayoutPrivate::OpenWidget = Widget;
		return FArcRmlDocumentRef{ArcRmlSamplesUmgLayoutPrivate::DocumentPath};
	}

	void Close(ULocalPlayer* LocalPlayer)
	{
		if (UUserWidget* Widget = ArcRmlSamplesUmgLayoutPrivate::OpenWidget.Get())
		{
			Widget->RemoveFromParent();
		}
		ArcRmlSamplesUmgLayoutPrivate::OpenWidget.Reset();
	}

	bool IsOpen(ULocalPlayer* LocalPlayer)
	{
		const UUserWidget* Widget = ArcRmlSamplesUmgLayoutPrivate::OpenWidget.Get();
		return Widget && Widget->IsInViewport();
	}
}
