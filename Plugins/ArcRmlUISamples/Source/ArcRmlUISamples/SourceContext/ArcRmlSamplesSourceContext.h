// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesSourceContext.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesSourceContextViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Samples")
	FString Label;
};

namespace ArcRmlSamplesSourceContext
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
