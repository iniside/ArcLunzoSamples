// Copyright 2026 Lukasz Baran. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArcRmlDemoRows.h"
#include "ArcRmlDemoScreenViewModel.h"

#include "ArcRmlDemoSocialViewModel.generated.h"

USTRUCT(BlueprintType)
struct FArcRmlDemoFriend
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Presence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	bool bInGame = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Party;
};

USTRUCT(BlueprintType)
struct FArcRmlDemoPresenceGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArcRmlDemo")
	FString Label;
};

UCLASS()
class UArcRmlDemoSocialViewModel : public UArcRmlDemoScreenViewModel
{
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Back() override;

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void OpenMenu(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Selected(const FString& Field, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Invite(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Message(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void SendRequest();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Accept(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void Decline(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void LeaveParty();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void ToggleReady();

	UFUNCTION(BlueprintCallable, Category = "ArcRmlDemo")
	void TabSelected(const FString& Bar, int32 Index);

	int32 BadgeCount() const
	{
		return PendingRequests.Num();
	}

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoFriendGroup> Groups;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString FriendsSub;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoPlayerRow> Party;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoRequestRow> Requests;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	TArray<FArcRmlDemoTabRow> Tabs;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 SubTab = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	int32 MenuRow = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString Query;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString AddId;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bNoResults = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	FString EmptyTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanInvite = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ArcRmlDemo")
	bool bCanSend = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoFriend> StartFriends;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoPlayerRow> StartParty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoRequestRow> StartRequests;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoMenuRow> FriendMenu;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FArcRmlDemoPresenceGroup> PresenceGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	TArray<FString> TabLabels;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	FString PlayerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcRmlDemo|Data")
	int32 PlayerLevel = 0;

protected:
	virtual void WriteText(const FString& Field, const FString& Value) override;

private:
	TArray<int32> View() const;
	const FArcRmlDemoFriend* FriendAt(int32 ViewIndex) const;
	int32 EmptyPartySlot() const;
	void InviteFriend(const FArcRmlDemoFriend& Friend);
	void CloseMenu();
	void Refresh();

	TArray<FArcRmlDemoFriend> Friends;
	TArray<FArcRmlDemoPlayerRow> Members;
	TArray<FArcRmlDemoRequestRow> PendingRequests;
};
