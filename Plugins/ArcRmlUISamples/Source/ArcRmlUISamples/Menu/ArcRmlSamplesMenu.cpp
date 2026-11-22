// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "Menu/ArcRmlSamplesMenu.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlSamplesMenu)

#if !UE_BUILD_SHIPPING

#include "ArcRmlSamplesLog.h"
#include "ArcRmlSamplesRunner.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Host/ArcRmlViewportSubsystem.h"

namespace ArcRmlSamplesMenu
{
	ULocalPlayer* CloseLastSample(UObject* WorldContextObject)
	{
		APlayerController* Player = Cast<APlayerController>(WorldContextObject);
		ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		if (!LocalPlayer)
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples menu: no local player controller");
			return nullptr;
		}
		const FArcRmlDocumentRef Closed = ArcRmlSamplesRunner::CloseLastOpened(LocalPlayer, Player->GetWorld());
		if (Closed.IsSet())
		{
			UE_LOGF(LogArcRmlSamples, Display, "ArcRmlSamples menu: closed %ls", *Closed.Path);
		}
		return LocalPlayer;
	}

	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer)
	{
		UArcRmlViewportSubsystem* Viewport = LocalPlayer ? LocalPlayer->GetSubsystem<UArcRmlViewportSubsystem>() : nullptr;
		if (!Viewport)
		{
			return FArcRmlDocumentRef();
		}

		const FArcRmlDocumentRef Document{TEXT("/ArcRmlUISamples/UI/Rml/Menu/SamplesMenu.rml")};
		return Viewport->OpenDocument(Document) ? Document : FArcRmlDocumentRef();
	}
}

#endif

UArcRmlSamplesMenuViewModel::UArcRmlSamplesMenuViewModel()
{
#if !UE_BUILD_SHIPPING
	for (const ArcRmlSamplesRunner::FSampleEntry& Entry : ArcRmlSamplesRunner::GetSamples())
	{
		Samples.Add({Entry.Name});
	}
	for (const ArcRmlSamplesRunner::FBlueprintRoute& Route : ArcRmlSamplesRunner::GetBlueprintRoutes())
	{
		BlueprintSamples.Add({Route.Name});
	}
#endif
}

void UArcRmlSamplesMenuViewModel::OpenSample(UObject* WorldContextObject, const FString& Name)
{
#if !UE_BUILD_SHIPPING
	const ArcRmlSamplesRunner::FSampleEntry* Found = ArcRmlSamplesRunner::FindSample(Name);
	if (!Found)
	{
		UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples menu: unknown sample '%ls'", *Name);
		return;
	}
	if (ULocalPlayer* LocalPlayer = ArcRmlSamplesMenu::CloseLastSample(WorldContextObject))
	{
		ArcRmlSamplesRunner::OpenSample(*Found, LocalPlayer);
	}
#endif
}

void UArcRmlSamplesMenuViewModel::OpenBlueprintSample(UObject* WorldContextObject, const FString& Name)
{
#if !UE_BUILD_SHIPPING
	const ArcRmlSamplesRunner::FBlueprintRoute* Found = ArcRmlSamplesRunner::FindBlueprintRoute(Name);
	if (!Found)
	{
		UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples menu: unknown Blueprint sample '%ls'", *Name);
		return;
	}
	if (ULocalPlayer* LocalPlayer = ArcRmlSamplesMenu::CloseLastSample(WorldContextObject))
	{
		if (!ArcRmlSamplesRunner::OpenBlueprintSample(*Found, LocalPlayer->GetPlayerController(LocalPlayer->GetWorld())))
		{
			UE_LOGF(LogArcRmlSamples, Warning, "ArcRmlSamples menu: %ls has no Open function", Found->Library);
		}
	}
#endif
}
