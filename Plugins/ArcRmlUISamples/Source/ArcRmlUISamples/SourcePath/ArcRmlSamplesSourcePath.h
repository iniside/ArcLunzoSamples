// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesSourcePath.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesSourcePathViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Samples")
	FString Label;
};

namespace ArcRmlSamplesSourcePath
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
