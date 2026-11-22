// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

class SWidget;
class UArcRmlSamplesSourcePathViewModel;
class ULocalPlayer;

#include "ArcRmlSamplesGameMode.generated.h"

UCLASS()
class AArcRmlSamplesPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArcRmlSamplesPlayerController();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	TObjectPtr<UArcRmlSamplesSourcePathViewModel> SampleViewModel;

private:
	TSharedPtr<SWidget> BackButton;
	TWeakObjectPtr<ULocalPlayer> BackButtonPlayer;
};

UCLASS()
class AArcRmlSamplesGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArcRmlSamplesGameMode();
};
