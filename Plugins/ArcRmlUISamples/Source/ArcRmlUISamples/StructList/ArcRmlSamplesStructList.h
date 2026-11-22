// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"
#include "ArcRmlStructList.h"

#include "ArcRmlSamplesStructList.generated.h"

class ULocalPlayer;

USTRUCT(BlueprintType)
struct FArcRmlSamplesStructListRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	int32 Quantity = 0;
};

UCLASS()
class UArcRmlSamplesStructListViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesStructListViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FArcRmlStructList Rows;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Add();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Remove(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Bump(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Reverse();

private:
	void AddRow();

	int32 NextNumber = 1;
};

namespace ArcRmlSamplesStructList
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
