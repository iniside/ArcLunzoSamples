// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesLists.generated.h"

class ULocalPlayer;

USTRUCT(BlueprintType)
struct FArcRmlSamplesListsRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	int32 Quantity = 0;
};

UCLASS()
class UArcRmlSamplesListsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesListsViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TArray<FArcRmlSamplesListsRow> Items;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Add();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Remove(int32 Index);

private:
	int32 NextNumber = 1;
};

namespace ArcRmlSamplesLists
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
