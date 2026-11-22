// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoCraftingViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoRecipeMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Need = 1;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Slot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRecipeMaterial> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Yields = 1;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoCraftChip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoMaterialInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Rarity;
};

UCLASS()
class UArcRmlDemoCraftingViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual FString GetScreenKey() const override
	{
		return TEXT("crafting");
	}

	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SelectChip(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SelectRecipe(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ToggleCanCraft();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Track();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Craft();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void CancelJob();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Stepped(const FString& Field, int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Selected(const FString& Field, int32 Index);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoCurrencyRow> Wallet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FString> Crumbs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Query;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanCraftOnly = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoChipRow> Chips;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRecipeRow> RecipeRows;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RecipesSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RecipeName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString RecipeSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoIngredientRow> Ingredients;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString QuantityLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoFactRow> Facts;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FArcRmlDemoItemTooltip Result;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bJob = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bMissing = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanCraft = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bHasRecipe = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 JobPct = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString JobLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString ProgressLabel;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString MissingBody;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString CraftLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoRecipe> Recipes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoCraftChip> ChipFilters;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoMaterialInfo> Materials;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString MaterialSource;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	float CraftSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 MaxQuantity = 0;

protected:
	virtual void GameFieldChanged(FName FieldName) override;
	virtual void WriteText(const FString& Field, const FString& Value) override;

private:
	int32 RecipeMax(const FArcRmlDemoRecipe& Recipe) const;
	TArray<int32> View() const;
	int32 Current() const;
	void KeepSelection();
	bool TickJob();
	void Refresh();

	int32 Chip = 0;
	int32 SelectedRecipe = 0;
	int32 Quantity = 1;
	int32 JobRecipe = INDEX_NONE;
	int32 JobTotal = 0;
	int32 JobDone = 0;
	float JobProgress = 0.0f;
};
