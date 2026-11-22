// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ArcRmlDemoSettings.generated.h"

class UArcRmlDemoGameViewModel;
class UArcRmlDemoHubViewModel;
class UArcRmlDemoUIViewModel;
class UWorld;

UCLASS(Config = ArcRmlUIDemo, DefaultConfig, meta = (DisplayName = "ArcRmlUI Demo"))
class UArcRmlDemoSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "View Models")
	TSoftClassPtr<UArcRmlDemoUIViewModel> UIViewModelClass;

	UPROPERTY(Config, EditAnywhere, Category = "View Models")
	TSoftClassPtr<UArcRmlDemoGameViewModel> GameViewModelClass;

	UPROPERTY(Config, EditAnywhere, Category = "View Models")
	TSoftClassPtr<UArcRmlDemoHubViewModel> HubViewModelClass;

	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> MenuMap;

	UPROPERTY(Config, EditAnywhere, Category = "Maps")
	TSoftObjectPtr<UWorld> GameMap;
};
