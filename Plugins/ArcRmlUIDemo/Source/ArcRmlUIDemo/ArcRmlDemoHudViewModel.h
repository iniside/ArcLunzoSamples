// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoHudViewModel.generated.h"

UCLASS()
class UArcRmlDemoHudViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("hud");
	}

	virtual void Initialize() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void OpenVendor();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void OpenCrafting();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void EndMatch();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bMultiplayer = false;

protected:
	virtual void GameFieldChanged(FName FieldName) override;

private:
	void Refresh();
	void PushFromHud(const TCHAR* Screen);
};
