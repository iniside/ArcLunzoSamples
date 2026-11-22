// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"

class ULocalPlayer;

namespace ArcRmlSamplesUmgEvents
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
	void Close(ULocalPlayer* LocalPlayer);
	bool IsOpen(ULocalPlayer* LocalPlayer);
}
