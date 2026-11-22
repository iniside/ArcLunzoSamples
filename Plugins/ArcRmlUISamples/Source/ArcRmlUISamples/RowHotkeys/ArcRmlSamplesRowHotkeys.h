// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesRowHotkeys.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesRowHotkeysViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Favourite = -1;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Used;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SetFavourite(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Use(const FString& Name);
};

namespace ArcRmlSamplesRowHotkeys
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
