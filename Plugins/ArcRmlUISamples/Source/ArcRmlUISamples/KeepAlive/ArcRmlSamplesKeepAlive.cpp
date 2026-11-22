// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "KeepAlive/ArcRmlSamplesKeepAlive.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlViewportSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesKeepAlive)

namespace ArcRmlSamplesKeepAlive
{
	const FArcRmlDocumentRef Menu{TEXT("/ArcRmlUISamples/UI/Rml/KeepAlive/Menu.rml")};
	const FArcRmlDocumentRef Kept{TEXT("/ArcRmlUISamples/UI/Rml/KeepAlive/Kept.rml")};
	const FArcRmlDocumentRef Preloaded{TEXT("/ArcRmlUISamples/UI/Rml/KeepAlive/Preloaded.rml")};

	UArcRmlViewportSubsystem* GetViewport(UObject* WorldContextObject)
	{
		const APlayerController* Player = Cast<APlayerController>(WorldContextObject);
		const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
	}

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}
		return Viewport->OpenDocument(Menu) ? Menu : FArcRmlDocumentRef();
	}
}

void UArcRmlSamplesKeepAliveMenuViewModel::Preload(UObject* WorldContextObject)
{
	if (UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesKeepAlive::GetViewport(WorldContextObject))
	{
		Viewport->PreloadDocument(ArcRmlSamplesKeepAlive::Preloaded);
	}
}

void UArcRmlSamplesKeepAliveMenuViewModel::OpenKept(UObject* WorldContextObject)
{
	if (UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesKeepAlive::GetViewport(WorldContextObject))
	{
		Viewport->OpenDocument(ArcRmlSamplesKeepAlive::Kept);
	}
}

void UArcRmlSamplesKeepAliveMenuViewModel::OpenPreloaded(UObject* WorldContextObject)
{
	if (UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesKeepAlive::GetViewport(WorldContextObject))
	{
		Viewport->OpenDocument(ArcRmlSamplesKeepAlive::Preloaded);
	}
}

void UArcRmlSamplesKeepAliveMenuViewModel::DiscardBoth(UObject* WorldContextObject)
{
	if (UArcRmlViewportSubsystem* Viewport = ArcRmlSamplesKeepAlive::GetViewport(WorldContextObject))
	{
		Viewport->DiscardDocument(ArcRmlSamplesKeepAlive::Kept);
		Viewport->DiscardDocument(ArcRmlSamplesKeepAlive::Preloaded);
	}
}

void UArcRmlSamplesKeepAliveKeptViewModel::Add()
{
	UE_MVVM_SET_PROPERTY_VALUE(Count, Count + 1);
}
