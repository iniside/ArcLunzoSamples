// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ArcRmlTestAssets.generated.h"

UCLASS()
class UArcRmlTestAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ArcRmlUI|Tests")
	static FString CreateLocTestTables(int32& OutSavedTables);
};
