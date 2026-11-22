// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoTalentsViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoTalentDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Tier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Column = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 MaxRank = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> Parents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bKeystone = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTalentTree
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TMap<FString, int32> StartRanks;
};

UCLASS()
class UArcRmlDemoTalentsViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Learn(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Unlearn(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ResetTree(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Choose(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	int32 BadgeCount() const
	{
		return Available;
	}

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTalentNodeRow> Nodes;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTalentLinkRow> Links;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTierRow> Tiers;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Trees;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 SubTab = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Selected;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoTalentDetail Detail;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bConfirm = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanLearn = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanUnlearn = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanReset = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ResetTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ResetBody;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoModalButtonRow> ModalButtons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoTalentDef> Talents;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoTalentTree> TreeDefs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 StartPoints = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 TierCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 TierCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 ResetGold = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString FirstSelected;

private:
	struct FNode
	{
		int32 Rank = 0;
		bool bOpen = false;
		bool bCanLearn = false;
		bool bCanUnlearn = false;
	};

	struct FTree
	{
		TArray<FNode> Nodes;
		int32 Spent = 0;
	};

	enum class EDirection : uint8
	{
		Up,
		Down,
		Left,
		Right
	};

	FTree BuildTree(int32 Tree) const;
	int32 IndexOf(const FString& Id) const;
	int32 SelectedIndex() const;
	void SelectNode(int32 Index);
	int32 NearestInTier(int32 Tier, int32 Column) const;
	FBox2f NodeBox(int32 Index) const;
	int32 PrototypeNeighbour(int32 Index, EDirection Direction) const;
	int32 GeometryNeighbour(int32 Index, EDirection Direction) const;
	FString NavOverride(int32 Index, EDirection Direction) const;
	void Refresh();

	TArray<TMap<FString, int32>> Ranks;
	int32 Available = 0;
};
