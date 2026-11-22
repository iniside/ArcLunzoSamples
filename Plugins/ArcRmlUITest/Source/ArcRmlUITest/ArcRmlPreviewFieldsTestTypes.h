// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlStructList.h"
#include "MVVMViewModelBase.h"
#include "UObject/Interface.h"

#include "ArcRmlPreviewFieldsTestTypes.generated.h"

UINTERFACE()
class UArcRmlPreviewFieldsTestTarget : public UInterface
{
	GENERATED_BODY()
};

class IArcRmlPreviewFieldsTestTarget
{
	GENERATED_BODY()
};

UCLASS()
class UArcRmlPreviewFieldsTestTargetObject : public UObject, public IArcRmlPreviewFieldsTestTarget
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FArcRmlPreviewFieldsTestRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FArcRmlPreviewFieldsTestGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FArcRmlStructList Rows;

	UPROPERTY(EditAnywhere)
	int32 Size = 0;
};

USTRUCT(BlueprintType)
struct FArcRmlPreviewFieldsTestNested
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FArcRmlPreviewFieldsTestGroup Group;
};

UCLASS()
class UArcRmlPreviewFieldsTestViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter = SetOffset, Getter = GetOffset)
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	TArray<int32> Numbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	TMap<FString, int32> Scores;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	TSet<FName> Labels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	TSubclassOf<UObject> Kind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	TScriptInterface<IArcRmlPreviewFieldsTestTarget> Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	FArcRmlPreviewFieldsTestGroup Group;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify)
	int32 Base = 1;

	UFUNCTION(BlueprintPure, FieldNotify)
	int32 GetTotal() const;

	void SetOffset(const FVector2D& InOffset);
	FVector2D GetOffset() const;

	UFUNCTION(BlueprintCallable)
	void SetNumbers(const TArray<int32>& InNumbers);

	int32 OffsetSetterCalls = 0;
	int32 NumbersSetterCalls = 0;
	mutable int32 TotalCalls = 0;
};
