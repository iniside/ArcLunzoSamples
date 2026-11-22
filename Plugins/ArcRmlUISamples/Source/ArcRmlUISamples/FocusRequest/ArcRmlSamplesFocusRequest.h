// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesFocusRequest.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesFocusRequestViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 NameFocusSerial = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 EmailFocusSerial = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 CodeFocusSerial = 0;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void FocusName();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void FocusEmail();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void FocusCode();
};

namespace ArcRmlSamplesFocusRequest
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
