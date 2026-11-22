// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "ArcRmlDocumentRef.h"

class APlayerController;
class ULocalPlayer;
class UWorld;

namespace ArcRmlSamplesRunner
{
	using FOpenFunction = FArcRmlDocumentRef (*)(ULocalPlayer* LocalPlayer);
	using FCloseFunction = void (*)(ULocalPlayer* LocalPlayer);
	using FIsOpenFunction = bool (*)(ULocalPlayer* LocalPlayer);

	struct FSampleEntry
	{
		const TCHAR* Name;
		FOpenFunction Open;
		TArray<FArcRmlDocumentRef> ExtraDocuments;
		FCloseFunction Close = nullptr;
		FIsOpenFunction IsOpen = nullptr;
	};

	struct FBlueprintRoute
	{
		const TCHAR* Name;
		const TCHAR* Library;
	};

	const TArray<FSampleEntry>& GetSamples();
	const TArray<FBlueprintRoute>& GetBlueprintRoutes();

	const FSampleEntry* FindSample(const FString& Name);
	const FBlueprintRoute* FindBlueprintRoute(const FString& Name);

	void OpenSample(const FSampleEntry& Sample, ULocalPlayer* LocalPlayer);

	bool OpenBlueprintSample(const FBlueprintRoute& Route, APlayerController* Player);

	FArcRmlDocumentRef CloseLastOpened(ULocalPlayer* LocalPlayer, UWorld* World);
}

#endif
