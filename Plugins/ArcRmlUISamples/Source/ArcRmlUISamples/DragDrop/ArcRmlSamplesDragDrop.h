// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesDragDrop.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesDragDropViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Hand;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Stash0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Stash1;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Stash2;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Dropped(const FString& Item, const FString& Target);
};

namespace ArcRmlSamplesDragDrop
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
