// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ArcRmlLocTestSubsystem.generated.h"

class UArcRmlLocTestViewModel;

/** Registers the Localization test page's view model under context LocTest for the game instance's lifetime. */
UCLASS()
class UArcRmlLocTestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TObjectPtr<UArcRmlLocTestViewModel> ViewModel;
};
