// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class SWidget;
class ULocalPlayer;

namespace ArcRmlSamplesBackButton
{
	TSharedPtr<SWidget> Add(ULocalPlayer* LocalPlayer);

	void Remove(ULocalPlayer* LocalPlayer, const TSharedPtr<SWidget>& Widget);
}

#endif
