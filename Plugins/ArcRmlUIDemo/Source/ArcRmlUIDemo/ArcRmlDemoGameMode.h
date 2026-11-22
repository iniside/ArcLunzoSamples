// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

#include "ArcRmlDemoGameMode.generated.h"

class UArcRmlDemoSubsystem;

UCLASS()
class AArcRmlDemoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArcRmlDemoPlayerController();
};

UCLASS()
class AArcRmlDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArcRmlDemoGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "ArcRmlDemo")
	FString StartScreen;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UArcRmlDemoSubsystem* FindDemo() const;
};
