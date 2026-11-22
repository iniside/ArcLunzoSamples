// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDocumentRef.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlSamplesAliases.generated.h"

class ULocalPlayer;

USTRUCT(BlueprintType)
struct FArcRmlSamplesAliasesHero
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	FString Title;

	UPROPERTY(BlueprintReadOnly, Category = "Samples")
	int32 Level = 1;
};

UCLASS()
class UArcRmlSamplesAliasesViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UArcRmlSamplesAliasesViewModel();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Samples")
	FArcRmlSamplesAliasesHero Selected;

	UFUNCTION(BlueprintCallable, Category = "Samples")
	void SelectNext();

private:
	TArray<FArcRmlSamplesAliasesHero> Heroes;
	int32 SelectedIndex = 0;
};

namespace ArcRmlSamplesAliases
{
	FArcRmlDocumentRef Open(ULocalPlayer* LocalPlayer);
}
