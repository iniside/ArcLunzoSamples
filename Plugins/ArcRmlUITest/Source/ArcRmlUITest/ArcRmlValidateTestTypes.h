// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMViewModelContextResolver.h"

#include "ArcRmlValidateTestTypes.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlValidateTestInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Label;

	UPROPERTY(EditAnywhere)
	int32 Level = 0;
};

UCLASS()
class UArcRmlValidateTestViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify)
	FString Title;

	UPROPERTY(BlueprintReadWrite, FieldNotify)
	int32 Count = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify)
	FArcRmlValidateTestInfo Info;

	UPROPERTY(BlueprintReadWrite, FieldNotify)
	TArray<FString> Names;

	UFUNCTION(BlueprintCallable)
	void Ping();

	UFUNCTION(BlueprintCallable)
	void Pick(int32 Index);
};

UCLASS()
class UArcRmlValidateTestDerivedViewModel : public UArcRmlValidateTestViewModel
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify)
	int32 DerivedOnly = 0;
};

UCLASS()
class UArcRmlValidateTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UArcRmlValidateTestViewModel> Stats;
};

UCLASS()
class UArcRmlValidateTestResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString ViewModelTag;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
