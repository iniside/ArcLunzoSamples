// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FArcRmlCookPathEntry;

namespace UE::Cook
{
	class ICookInfo;
}

namespace ArcRmlGameFeatureActionPaths
{
	void Read(UE::Cook::ICookInfo& CookInfo, TArray<FArcRmlCookPathEntry>& OutEntries);
}
