// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoCraftingViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoSubsystem.h"
#include "HAL/PlatformTime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoCraftingViewModel)

namespace ArcRmlDemoCraftingViewModelPrivate
{
	const FName CraftTimer(TEXT("craft"));
	const TCHAR* const SearchField = TEXT("craftSearch");
	constexpr double TickSeconds = 0.1;
}

void UArcRmlDemoCraftingViewModel::Initialize()
{
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Bag));
	WatchGameField(GET_MEMBER_NAME_CHECKED(UArcRmlDemoGameViewModel, Gold));
	Refresh();
}

void UArcRmlDemoCraftingViewModel::Back()
{
	if (bJob)
	{
		CancelJob();
		return;
	}
	Super::Back();
}

void UArcRmlDemoCraftingViewModel::SelectChip(int32 Index)
{
	if (bJob || ChipFilters.IsEmpty())
	{
		return;
	}
	Chip = FMath::Clamp(Index, 0, ChipFilters.Num() - 1);
	const TArray<int32> Shown = View();
	SelectedRecipe = Shown.IsEmpty() ? 0 : Shown[0];
	Refresh();
}

void UArcRmlDemoCraftingViewModel::SelectRecipe(int32 Index)
{
	if (bJob)
	{
		return;
	}
	const TArray<int32> Shown = View();
	if (Shown.IsValidIndex(Index))
	{
		SelectedRecipe = Shown[Index];
	}
	Craft();
}

void UArcRmlDemoCraftingViewModel::ToggleCanCraft()
{
	if (bJob)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bCanCraftOnly, !bCanCraftOnly);
	KeepSelection();
	Refresh();
}

void UArcRmlDemoCraftingViewModel::Track()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const int32 Recipe = Current();
	if (!bJob && Demo && Recipe != INDEX_NONE)
	{
		Demo->ShowToast(TEXT("info"), TEXT("Tracking ingredients"), Recipes[Recipe].Name + TEXT(" · shown on the HUD"));
	}
}

void UArcRmlDemoCraftingViewModel::Craft()
{
	const int32 Recipe = Current();
	if (bJob || Recipe == INDEX_NONE || RecipeMax(Recipes[Recipe]) < Quantity)
	{
		Refresh();
		return;
	}
	JobRecipe = Recipe;
	JobTotal = Quantity;
	JobDone = 0;
	JobProgress = 0.0f;
	ARC_RML_DEMO_SET(*this, bJob, true);
	Every(ArcRmlDemoCraftingViewModelPrivate::CraftTimer, ArcRmlDemoCraftingViewModelPrivate::TickSeconds, [this]() -> bool
	{
		return TickJob();
	});
	Refresh();
}

void UArcRmlDemoCraftingViewModel::CancelJob()
{
	if (!bJob)
	{
		return;
	}
	CancelTimer(ArcRmlDemoCraftingViewModelPrivate::CraftTimer);
	ARC_RML_DEMO_SET(*this, bJob, false);
	if (UArcRmlDemoSubsystem* Demo = GetDemo())
	{
		Demo->ShowToast(TEXT("info"), TEXT("Crafting cancelled"), FString::Printf(TEXT("%d of %d crafted"), JobDone, JobTotal));
	}
	Refresh();
}

void UArcRmlDemoCraftingViewModel::Stepped(const FString& Field, int32 Delta)
{
	if (Field != TEXT("batch") || bJob)
	{
		return;
	}
	Quantity = FMath::Clamp(Quantity + (Delta < 0 ? -1 : 1), 1, FMath::Max(1, MaxQuantity));
	Refresh();
}

void UArcRmlDemoCraftingViewModel::Selected(const FString& Field, int32 Index)
{
	if (Field == TEXT("crumbs"))
	{
		Back();
	}
}

void UArcRmlDemoCraftingViewModel::GameFieldChanged(FName FieldName)
{
	Refresh();
}

void UArcRmlDemoCraftingViewModel::WriteText(const FString& Field, const FString& Value)
{
	if (Field != ArcRmlDemoCraftingViewModelPrivate::SearchField)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, Query, Value);
	KeepSelection();
	Refresh();
}

int32 UArcRmlDemoCraftingViewModel::RecipeMax(const FArcRmlDemoRecipe& Recipe) const
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game || Recipe.Materials.IsEmpty())
	{
		return 0;
	}
	int32 Max = MAX_int32;
	for (const FArcRmlDemoRecipeMaterial& Material : Recipe.Materials)
	{
		Max = FMath::Min(Max, Game->HaveOf(Material.Material) / FMath::Max(1, Material.Need));
	}
	return Max;
}

TArray<int32> UArcRmlDemoCraftingViewModel::View() const
{
	const FString Type = ChipFilters.IsValidIndex(Chip) ? ChipFilters[Chip].Type : FString();
	const FString Search = Query.TrimStartAndEnd();
	TArray<int32> Shown;
	for (int32 Index = 0; Index < Recipes.Num(); ++Index)
	{
		const FArcRmlDemoRecipe& Recipe = Recipes[Index];
		if ((Type.IsEmpty() || Recipe.Type == Type) && (Search.IsEmpty() || Recipe.Name.Contains(Search, ESearchCase::IgnoreCase))
			&& (!bCanCraftOnly || RecipeMax(Recipe) > 0))
		{
			Shown.Add(Index);
		}
	}
	return Shown;
}

int32 UArcRmlDemoCraftingViewModel::Current() const
{
	const TArray<int32> Shown = View();
	if (Shown.Contains(SelectedRecipe))
	{
		return SelectedRecipe;
	}
	return Shown.IsEmpty() ? INDEX_NONE : Shown[0];
}

void UArcRmlDemoCraftingViewModel::KeepSelection()
{
	const TArray<int32> Shown = View();
	if (!Shown.IsEmpty() && !Shown.Contains(SelectedRecipe))
	{
		SelectedRecipe = Shown[0];
	}
}

bool UArcRmlDemoCraftingViewModel::TickJob()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!bJob || !Demo || !Game || !Recipes.IsValidIndex(JobRecipe))
	{
		ARC_RML_DEMO_SET(*this, bJob, false);
		Refresh();
		return false;
	}
	JobProgress += 100.0f / FMath::Max(1.0f, CraftSeconds * 10.0f);
	if (JobProgress < 100.0f)
	{
		Refresh();
		return true;
	}
	const FArcRmlDemoRecipe& Recipe = Recipes[JobRecipe];
	if (RecipeMax(Recipe) < 1)
	{
		ARC_RML_DEMO_SET(*this, bJob, false);
		Demo->ShowToast(TEXT("warning"), TEXT("Out of ingredients"), Recipe.Name);
		Refresh();
		return false;
	}
	for (const FArcRmlDemoRecipeMaterial& Material : Recipe.Materials)
	{
		Game->UseMaterial(Material.Material, Material.Need);
	}
	FArcRmlDemoItem Crafted;
	Crafted.Id = FString::Printf(TEXT("c%llu%d"), FPlatformTime::Cycles64(), JobDone);
	Crafted.Icon = Recipe.Icon;
	Crafted.Rarity = Recipe.Rarity;
	Crafted.Name = Recipe.Name;
	Crafted.Category = Recipe.Category;
	Crafted.Slot = Recipe.Slot;
	Crafted.Quantity = Recipe.Yields;
	Crafted.Weight = Recipe.Category == TEXT("weapon") || Recipe.Category == TEXT("armor") ? 5.0f : 0.2f;
	Game->AddToBag(Crafted);
	JobDone += 1;
	JobProgress = 0.0f;
	if (JobDone < JobTotal)
	{
		Refresh();
		return true;
	}
	ARC_RML_DEMO_SET(*this, bJob, false);
	Demo->ShowToast(TEXT("success"), FString::Printf(TEXT("Crafted %d × %s"), JobTotal, *Recipe.Name), TEXT("Added to your backpack."));
	Quantity = 1;
	Refresh();
	return false;
}

void UArcRmlDemoCraftingViewModel::Refresh()
{
	const UArcRmlDemoGameViewModel* Game = GetGame();
	if (!Game || Recipes.IsEmpty())
	{
		return;
	}
	const TArray<int32> Shown = View();
	const int32 Selection = Current();
	const FArcRmlDemoRecipe& Recipe = Recipes[Selection != INDEX_NONE ? Selection : 0];
	const int32 Max = Selection != INDEX_NONE ? RecipeMax(Recipe) : 0;
	const bool bShort = !bJob && Selection != INDEX_NONE && Max < Quantity;

	TArray<FArcRmlDemoRecipeRow> Rows;
	for (const int32 Index : Shown)
	{
		const FArcRmlDemoRecipe& Def = Recipes[Index];
		const int32 Count = RecipeMax(Def);
		FArcRmlDemoRecipeRow& Row = Rows.AddDefaulted_GetRef();
		Row.Icon = Def.Icon;
		Row.Name = Def.Name;
		Row.Type = Def.Type;
		Row.Rarity = Def.Rarity;
		Row.RarityLabel = UArcRmlDemoGameViewModel::RarityLabel(Def.Rarity);
		Row.Meta = Count > 0 ? FString::Printf(TEXT("×%d"), Count) : FString(TEXT("Missing"));
		Row.bSelected = Index == Selection;
		Row.bDisabled = Count <= 0 && !Row.bSelected;
	}

	TArray<FArcRmlDemoChipRow> ChipRows;
	for (int32 Index = 0; Index < ChipFilters.Num(); ++Index)
	{
		const FString& Type = ChipFilters[Index].Type;
		FArcRmlDemoChipRow& Row = ChipRows.AddDefaulted_GetRef();
		Row.Label = ChipFilters[Index].Label;
		Row.Count = Recipes.FilterByPredicate([&Type](const FArcRmlDemoRecipe& Def) -> bool { return Type.IsEmpty() || Def.Type == Type; }).Num();
		Row.bActive = Chip == Index;
	}

	TArray<FArcRmlDemoIngredientRow> IngredientRows;
	TArray<FString> Missing;
	for (const FArcRmlDemoRecipeMaterial& Material : Recipe.Materials)
	{
		FArcRmlDemoIngredientRow& Row = IngredientRows.AddDefaulted_GetRef();
		if (const FArcRmlDemoMaterialInfo* Info = Materials.FindByPredicate([&Material](const FArcRmlDemoMaterialInfo& Candidate) -> bool { return Candidate.Name == Material.Material; }))
		{
			Row.Icon = Info->Icon;
			Row.Rarity = Info->Rarity;
		}
		Row.Name = Material.Material;
		const int32 Have = Game->HaveOf(Material.Material);
		const int32 Wanted = Material.Need * Quantity;
		Row.CountLabel = FString::Printf(TEXT("%d / %d"), Have, Wanted);
		Row.bShort = Have < Wanted;
		Row.Where = MaterialSource;
		if (Row.bShort)
		{
			Missing.Add(FString::Printf(TEXT("%d more %s"), Wanted - Have, *Material.Material));
		}
	}

	int32 Craftable = 0;
	for (const FArcRmlDemoRecipe& Def : Recipes)
	{
		Craftable += RecipeMax(Def) > 0 ? 1 : 0;
	}

	FArcRmlDemoItem Made;
	Made.Name = Recipe.Name;
	Made.Icon = Recipe.Icon;
	Made.Rarity = Recipe.Rarity;
	Made.Category = Recipe.Category;
	Made.Slot = Recipe.Slot;
	Made.Quantity = Recipe.Yields;

	FArcRmlDemoCurrencyRow GoldRow = Game->Coins(UArcRmlDemoGameViewModel::FormatNumber(Game->Gold));
	GoldRow.Label = TEXT("Gold");

	ARC_RML_DEMO_SET(*this, Wallet, TArray<FArcRmlDemoCurrencyRow>({GoldRow}));
	ARC_RML_DEMO_SET(*this, Chips, ChipRows);
	ARC_RML_DEMO_SET(*this, RecipeRows, Rows);
	ARC_RML_DEMO_SET(*this, RecipesSub, FString::Printf(TEXT("%d known · %d craftable"), Recipes.Num(), Craftable));
	ARC_RML_DEMO_SET(*this, RecipeName, Recipe.Name);
	ARC_RML_DEMO_SET(*this, RecipeSub, FString::Printf(TEXT("%s · makes %d each · %d craftable"), *Recipe.Type, Recipe.Yields, Max));
	ARC_RML_DEMO_SET(*this, Ingredients, IngredientRows);
	ARC_RML_DEMO_SET(*this, QuantityLabel, FString::FromInt(bJob ? JobTotal : Quantity));
	ARC_RML_DEMO_SET(*this, Result, Game->TipOf(Made));
	ARC_RML_DEMO_SET(*this, bMissing, bShort);
	ARC_RML_DEMO_SET(*this, bCanCraft, Max >= Quantity && Max > 0);
	ARC_RML_DEMO_SET(*this, bHasRecipe, Selection != INDEX_NONE);
	ARC_RML_DEMO_SET(*this, JobPct, bJob ? FMath::RoundToInt(JobProgress) : 0);
	ARC_RML_DEMO_SET(*this, JobLabel, bJob
		? FString::Printf(TEXT("%d / %d · %d s"), JobDone + 1, JobTotal, FMath::CeilToInt(CraftSeconds * (1.0f - JobProgress / 100.0f)))
		: FString());
	ARC_RML_DEMO_SET(*this, ProgressLabel, TEXT("Crafting ") + Recipe.Name);
	ARC_RML_DEMO_SET(*this, MissingBody, Missing.IsEmpty() ? FString() : TEXT("You need ") + FString::Join(Missing, TEXT(", ")) + TEXT("."));
	ARC_RML_DEMO_SET(*this, CraftLabel, bJob ? FString(TEXT("Crafting…")) : Quantity > 1 ? FString::Printf(TEXT("Craft ×%d"), Quantity) : FString(TEXT("Craft")));
}
