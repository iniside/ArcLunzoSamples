// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlKeptTabTestTypes.generated.h"

UCLASS()
class UArcRmlKeptTabViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter = SetCount, Getter = GetCount)
	int32 Count = 0;

	UFUNCTION(BlueprintCallable)
	void Ping();

	UFUNCTION(BlueprintCallable)
	void OnLeftA();

	UFUNCTION(BlueprintCallable)
	void OnLeftB();

	void SetCount(int32 InCount);
	int32 GetCount() const;
};
