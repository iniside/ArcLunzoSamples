// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"

#include "ArcRmlDocumentWidgetTestTypes.generated.h"

UCLASS()
class UArcRmlDocumentWidgetTestHost : public UUserWidget
{
	GENERATED_BODY()
};

UCLASS()
class UArcRmlDocumentWidgetTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleOpenRequested(const FString& Path)
	{
		OpenPaths.Add(Path);
	}

	UFUNCTION()
	void HandleCloseRequested()
	{
		++CloseCount;
	}

	TArray<FString> OpenPaths;
	int32 CloseCount = 0;
};

UCLASS()
class UArcRmlDocumentWidgetTestViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter = SetText, Getter = GetText)
	FString Text;

	void SetText(const FString& InText)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Text, InText);
	}

	FString GetText() const
	{
		return Text;
	}
};
