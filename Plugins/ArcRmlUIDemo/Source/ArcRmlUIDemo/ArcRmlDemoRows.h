// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ArcRmlImageRef.h"

#include "ArcRmlDemoRows.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoModalButtonRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Variant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDisabled = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTabRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Badge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDisabled = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoCurrencyRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Value;

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	bool bInsufficient = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoMenuRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString KeyLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDisabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDanger = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bSeparator = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoRewardRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Rarity;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoDevRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoStatLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Delta;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDeltaGood = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSetLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label;

	UPROPERTY()
	bool bActive = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoReqLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label;

	UPROPERTY()
	bool bMet = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoItemTooltip
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	FString RarityLabel;

	UPROPERTY()
	FString Type;

	UPROPERTY()
	TArray<FArcRmlDemoStatLine> Primary;

	UPROPERTY()
	TArray<FArcRmlDemoStatLine> Stats;

	UPROPERTY()
	bool bHasSet = false;

	UPROPERTY()
	FString SetName;

	UPROPERTY()
	FString SetProgress;

	UPROPERTY()
	TArray<FArcRmlDemoSetLine> SetLines;

	UPROPERTY()
	FString Flavour;

	UPROPERTY()
	TArray<FArcRmlDemoReqLine> Reqs;

	UPROPERTY()
	TArray<FArcRmlDemoCurrencyRow> SellItems;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoToast
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bVisible = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Tone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Progress;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoDevBar
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FArcRmlDemoDevRow> Screens;

	UPROPERTY()
	FString Session;

	UPROPERTY()
	FString Current;

	UPROPERTY()
	FString Status;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoMenuItemRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Badge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Meta;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDisabled = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSettingRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	FString Type;

	UPROPERTY()
	bool bOn = false;

	UPROPERTY()
	float Value = 0.0f;

	UPROPERTY()
	float Min = 0.0f;

	UPROPERTY()
	float Max = 0.0f;

	UPROPERTY()
	FString ValueLabel;

	UPROPERTY()
	int32 Index = 0;

	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	TArray<FString> Options;

	UPROPERTY()
	bool bOpen = false;

	UPROPERTY()
	FString KeyState;

	UPROPERTY()
	FString ConflictLabel;

	UPROPERTY()
	bool bChanged = false;

	UPROPERTY()
	bool bLocked = false;

	UPROPERTY()
	int32 Row = 0;

	UPROPERTY()
	FString Placeholder;

	UPROPERTY()
	FString Tag;

	UPROPERTY()
	FString TagTone;

	UPROPERTY()
	FString Field;

	UPROPERTY()
	int32 Step = 0;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSettingGroup
{
	GENERATED_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	TArray<FArcRmlDemoSettingRow> Rows;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoCreditSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> Names;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoSaveSlotRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString State;

	UPROPERTY()
	FString SlotLabel;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Location;

	UPROPERTY()
	FString Playtime;

	UPROPERTY()
	FString Date;

	UPROPERTY()
	FString Chapter;

	UPROPERTY()
	FString Difficulty;

	UPROPERTY()
	FString EmptyHint;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoServerRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Region;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Mode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Map;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Players = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 MaxPlayers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Ping = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bFavorite = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoPlayerRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Initials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bHost = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoItemSlotRow
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Pos = -1;

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FArcRmlImageRef EmptyIcon;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	FString Durability;

	UPROPERTY()
	bool bNew = false;

	UPROPERTY()
	bool bLocked = false;

	UPROPERTY()
	bool bGhost = false;

	UPROPERTY()
	bool bSelected = false;

	UPROPERTY()
	bool bConsumable = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoChipRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	bool bActive = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoEquipRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString SlotKey;

	UPROPERTY()
	FString Slot;

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Durability;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoAttributeRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	int32 Value = 0;

	UPROPERTY()
	int32 Pending = 0;

	UPROPERTY()
	FString Delta;

	UPROPERTY()
	bool bCanInc = false;

	UPROPERTY()
	bool bCanDec = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTalentNodeRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	int32 Tier = 0;

	UPROPERTY()
	int32 Column = 0;

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	int32 Rank = 0;

	UPROPERTY()
	int32 MaxRank = 0;

	UPROPERTY()
	FString State;

	UPROPERTY()
	bool bKeystone = false;

	UPROPERTY()
	int32 X = 0;

	UPROPERTY()
	int32 Y = 0;

	UPROPERTY()
	FString NavUp;

	UPROPERTY()
	FString NavDown;

	UPROPERTY()
	FString NavLeft;

	UPROPERTY()
	FString NavRight;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoFactRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Value;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTalentDetail
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Sub;

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	int32 Rank = 0;

	UPROPERTY()
	int32 MaxRank = 0;

	UPROPERTY()
	FString RankLabel;

	UPROPERTY()
	FString Current;

	UPROPERTY()
	FString Next;

	UPROPERTY()
	TArray<FArcRmlDemoFactRow> Facts;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoQuestRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Kind;

	UPROPERTY()
	int32 Level = 0;

	UPROPERTY()
	FString Region;

	UPROPERTY()
	FString Progress;

	UPROPERTY()
	FString State;

	UPROPERTY()
	bool bTracked = false;

	UPROPERTY()
	TArray<FArcRmlDemoRewardRow> Rewards;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoFriendRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Tag;

	UPROPERTY()
	FString Initials;

	UPROPERTY()
	FString Presence;

	UPROPERTY()
	FString Status;

	UPROPERTY()
	bool bInGame = false;

	UPROPERTY()
	FString Party;
	UPROPERTY()
	int32 Index = 0;

	UPROPERTY()
	TArray<FArcRmlDemoMenuRow> Menu;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoRequestRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Initials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Presence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Status;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoAchievementRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	FString State;

	UPROPERTY()
	int32 Value = 0;

	UPROPERTY()
	int32 Max = 0;

	UPROPERTY()
	FString Date;

	UPROPERTY()
	FString RarityLabel;

	UPROPERTY()
	float Pct = 0.0f;

	UPROPERTY()
	FString Tier;

	UPROPERTY()
	FString Points;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoStockRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	TArray<FArcRmlDemoCurrencyRow> Price;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	bool bUnusable = false;

	UPROPERTY()
	bool bSelected = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoRecipeRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	FString RarityLabel;

	UPROPERTY()
	FString Meta;

	UPROPERTY()
	bool bDisabled = false;

	UPROPERTY()
	bool bSelected = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoIngredientRow
{
	GENERATED_BODY()

	UPROPERTY()
	FArcRmlImageRef Icon;

	UPROPERTY()
	FString Rarity;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString CountLabel;

	UPROPERTY()
	bool bShort = false;

	UPROPERTY()
	FString Where;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoScoreRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Rank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Initials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FString> Cols;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Ping = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bLocal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bMvp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bDisconnected = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTeamRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Sub;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Team;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoScoreRow> Rows;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoFriendGroup
{
	GENERATED_BODY()

	UPROPERTY()
	FString Key;

	UPROPERTY()
	FString Label;

	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	TArray<FArcRmlDemoFriendRow> Rows;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoObjectiveRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Current = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	int32 Total = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString State = TEXT("active");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Optional;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoRewardItemRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bUnusable = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTalentLinkRow
{
	GENERATED_BODY()

	UPROPERTY()
	float X1 = 0.0f;

	UPROPERTY()
	float Y1 = 0.0f;

	UPROPERTY()
	float Len = 0.0f;

	UPROPERTY()
	float Ang = 0.0f;

	UPROPERTY()
	bool bActive = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoTierRow
{
	GENERATED_BODY()

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FString Req;

	UPROPERTY()
	int32 Top = 0;

	UPROPERTY()
	bool bLocked = false;

	UPROPERTY()
	bool bFirst = false;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoStatGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoStatLine> Rows;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoCategoryRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FArcRmlImageRef Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Sub;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Meta;
};
