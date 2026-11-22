// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoSplashViewModel.generated.h"

UCLASS()
class UArcRmlDemoSplashViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("splash");
	}

	virtual void InitScreen(const FString& Param) override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Continue();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bReady = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	float ReadySeconds = 0.0f;
};
