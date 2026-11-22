// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"
#include "View/MVVMViewModelContextResolver.h"

#include "ArcRmlSamplesSourceResolver.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesSourceResolverViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString Greeting;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FString PlayerName;
};

UCLASS()
class UArcRmlSamplesGreetingResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Samples")
	FString Greeting;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};

namespace ArcRmlSamplesSourceResolver
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
