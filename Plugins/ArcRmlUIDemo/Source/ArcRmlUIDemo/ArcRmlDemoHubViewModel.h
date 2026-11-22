// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoHubViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoHubTab
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Document;
};

UCLASS()
class UArcRmlDemoHubViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("hub");
	}

	virtual void Initialize() override;
	virtual void InitScreen(const FString& Param) override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	void SelectTab(const FString& Document);
	void Refresh();

	static void RefreshFor(UArcRmlDemoSubsystem* Demo);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Tabs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 Tab = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> Currency;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoHubTab> TabDefs;

protected:
	virtual void GameFieldChanged(FName FieldName) override;
};
