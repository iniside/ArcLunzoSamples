// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoViewModelBase.h"

#include "ArcRmlDemoScreenViewModel.generated.h"

class UArcRmlDemoGameViewModel;

UCLASS(Abstract)
class UArcRmlDemoScreenViewModel : public UArcRmlDemoViewModelBase
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const
	{
		return FString();
	}

	virtual void Initialize()
	{
	}

	virtual void Shutdown();

	virtual void InitScreen(const FString& Param)
	{
	}

	virtual void ScreenActivated()
	{
	}

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	virtual void Back();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TextInput(const FString& Field, const FString& Value);

	UArcRmlDemoGameViewModel* GetGame() const;

protected:
	void WatchGameField(FName FieldName);

	virtual void GameFieldChanged(FName FieldName)
	{
	}

	virtual void WriteText(const FString& Field, const FString& Value)
	{
	}

	void Later(FName Id, double Seconds, TFunction<void()> Fn);
	void Every(FName Id, double Seconds, TFunction<bool()> Fn);
	void CancelTimer(FName Id);

private:
	void HandleGameFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);
};
