// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesNestedViewModel.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesNestedChildViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Name;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Points = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void AddPoint();
};

UCLASS()
class UArcRmlSamplesNestedParentViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesNestedParentViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TObjectPtr<UArcRmlSamplesNestedChildViewModel> Child;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TArray<TObjectPtr<UArcRmlSamplesNestedChildViewModel>> Children;
};

namespace ArcRmlSamplesNestedViewModel
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
