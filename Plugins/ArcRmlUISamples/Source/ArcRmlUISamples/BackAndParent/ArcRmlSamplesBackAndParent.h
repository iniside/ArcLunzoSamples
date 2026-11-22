// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesBackAndParent.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesBackAndParentViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Selected;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 UseCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 ExitCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 BackCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Select(const FString& Item);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Use();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Exit();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void CountBack();

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void PressBack(UObject* WorldContextObject);
};

namespace ArcRmlSamplesBackAndParent
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
