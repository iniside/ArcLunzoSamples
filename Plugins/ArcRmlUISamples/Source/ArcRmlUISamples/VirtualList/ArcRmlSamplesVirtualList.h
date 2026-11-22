// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesVirtualList.generated.h"

class ULocalPlayer;

UCLASS()
class UArcRmlSamplesVirtualListViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesVirtualListViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	TArray<int32> Rows;
};

namespace ArcRmlSamplesVirtualList
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
