// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesEvents.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesEventsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FText LastEvent = INVTEXT("Nothing yet");

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 Value = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 InnerCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	int32 OuterCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	bool bBackArmed = false;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Pick(const FString& Colour, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Pointer(const FString& Button);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Step(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Inner();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void Outer();

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void ArmBack();

	UFUNCTION(BlueprintCallable, Category = "Samples", meta = (WorldContext = "WorldContextObject"))
	void ShowCaller(UObject* WorldContextObject);
};

namespace ArcRmlSamplesEvents
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
