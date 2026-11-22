// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoCharacterViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoAttributeDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Base = 0;
};

UCLASS()
class UArcRmlDemoCharacterViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Change(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Unequip(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void AttributeStep(int32 Index, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void AttributeInfo(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ApplyPoints();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ResetPoints();

	int32 BadgeCount() const;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoEquipRow> EquipLeft;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoEquipRow> EquipRight;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoAttributeRow> Attributes;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bUnspent = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Remaining;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString PointsSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bNoPending = true;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanAdd = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanUnequip = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoStatGroup> Stats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoAttributeDef> AttributeDefs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 StartPoints = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString NoPointsSubtitle;

protected:
	virtual void GameFieldChanged(FName FieldName) override;

private:
	int32 PendingTotal() const;
	void Refresh();

	int32 Pool = 0;
	TArray<int32> Base;
	TArray<int32> Pending;
};
