// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoSocialViewModel.h"

#include "ArcRmlDemoHubViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoSocialViewModel)

namespace ArcRmlDemoSocialViewModelPrivate
{
	const TCHAR* const SearchField = TEXT("friendsSearch");
	const TCHAR* const AddField = TEXT("friendsAdd");

	FString Initials(const FString& Name)
	{
		FString Clean;
		for (const TCHAR Char : Name)
		{
			Clean.AppendChar(FChar::IsAlpha(Char) || Char == TEXT(' ') ? Char : TEXT(' '));
		}
		TArray<FString> Words;
		FString Word;
		for (const TCHAR Char : Clean.TrimStartAndEnd())
		{
			if (Char == TEXT(' ') || (FChar::IsUpper(Char) && !Word.IsEmpty()))
			{
				if (!Word.IsEmpty())
				{
					Words.Add(Word);
				}
				Word.Reset();
				if (Char == TEXT(' '))
				{
					continue;
				}
			}
			Word.AppendChar(Char);
		}
		if (!Word.IsEmpty())
		{
			Words.Add(Word);
		}
		FString Result;
		for (int32 Index = 0; Index < Words.Num() && Index < 2; ++Index)
		{
			Result.AppendChar(Words[Index][0]);
		}
		Result = Result.ToUpper();
		return Result.Len() > 1 ? Result : Name.Left(2).ToUpper();
	}
}

void UArcRmlDemoSocialViewModel::Initialize()
{
	Friends = StartFriends;
	Members = StartParty;
	PendingRequests = StartRequests;
	Refresh();
}

void UArcRmlDemoSocialViewModel::Back()
{
	if (MenuRow != INDEX_NONE)
	{
		CloseMenu();
		Refresh();
		return;
	}
	Super::Back();
}

void UArcRmlDemoSocialViewModel::OpenMenu(int32 Index)
{
	if (MenuRow != INDEX_NONE || !FriendAt(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, MenuRow, Index);
	Refresh();
}

void UArcRmlDemoSocialViewModel::Selected(const FString& Field, int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const FArcRmlDemoFriend* Found = FriendAt(MenuRow);
	if (Field != TEXT("menu") || !Demo || !Found)
	{
		return;
	}
	const FArcRmlDemoFriend Chosen = *Found;
	const FString Key = FriendMenu.IsValidIndex(Index) ? FriendMenu[Index].Key : FString();
	CloseMenu();
	if (Key == TEXT("invite"))
	{
		InviteFriend(Chosen);
	}
	else if (Key == TEXT("message"))
	{
		Demo->ShowToast(TEXT("info"), TEXT("Message to ") + Chosen.Name, TEXT("Chat opens in the social overlay."));
	}
	else if (Key == TEXT("profile"))
	{
		Demo->ShowToast(TEXT("info"), Chosen.Name + TEXT(" #") + Chosen.Tag, Chosen.Status);
	}
	else if (Key == TEXT("join"))
	{
		if (Chosen.bInGame)
		{
			Demo->ShowToast(TEXT("info"), TEXT("Joining ") + Chosen.Name, Chosen.Status);
		}
		else
		{
			Demo->ShowToast(TEXT("warning"), Chosen.Name + TEXT(" is not in a game"));
		}
	}
	else if (Key == TEXT("remove") || Key == TEXT("block"))
	{
		const bool bBlock = Key == TEXT("block");
		Friends.RemoveAll([&Chosen](const FArcRmlDemoFriend& Other) -> bool { return Other.Name == Chosen.Name; });
		Demo->ShowToast(bBlock ? TEXT("error") : TEXT("info"), (bBlock ? TEXT("Blocked ") : TEXT("Removed ")) + Chosen.Name);
	}
	Refresh();
}

void UArcRmlDemoSocialViewModel::Invite(int32 Index)
{
	if (MenuRow != INDEX_NONE)
	{
		return;
	}
	if (const FArcRmlDemoFriend* Found = FriendAt(Index))
	{
		const FArcRmlDemoFriend Chosen = *Found;
		InviteFriend(Chosen);
		Refresh();
	}
}

void UArcRmlDemoSocialViewModel::Message(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const FArcRmlDemoFriend* Found = FriendAt(Index);
	if (MenuRow != INDEX_NONE || !Demo || !Found || Found->Presence == TEXT("offline"))
	{
		return;
	}
	Demo->ShowToast(TEXT("info"), TEXT("Message to ") + Found->Name, TEXT("Chat opens in the social overlay."));
}

void UArcRmlDemoSocialViewModel::SendRequest()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const FString Id = !AddId.TrimStartAndEnd().IsEmpty() ? AddId.TrimStartAndEnd() : Query.TrimStartAndEnd();
	if (!Demo || Id.IsEmpty())
	{
		return;
	}
	Demo->ShowToast(TEXT("success"), TEXT("Friend request sent"), Id);
	ARC_RML_DEMO_SET(*this, AddId, FString());
	Refresh();
}

void UArcRmlDemoSocialViewModel::Accept(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo || !PendingRequests.IsValidIndex(Index))
	{
		return;
	}
	const FArcRmlDemoRequestRow Accepted = PendingRequests[Index];
	PendingRequests.RemoveAt(Index);
	FArcRmlDemoFriend Added;
	Added.Name = Accepted.Name;
	Added.Tag = Accepted.Tag.Replace(TEXT("#"), TEXT(""));
	Added.Presence = Accepted.Presence;
	Added.Status = TEXT("New friend");
	Friends.Insert(Added, 0);
	TArray<FString> Order;
	for (const FArcRmlDemoPresenceGroup& Group : PresenceGroups)
	{
		Order.Add(Group.Key);
	}
	Friends.StableSort([&Order](const FArcRmlDemoFriend& A, const FArcRmlDemoFriend& B) -> bool { return Order.IndexOfByKey(A.Presence) < Order.IndexOfByKey(B.Presence); });
	Demo->ShowToast(TEXT("success"), Accepted.Name + TEXT(" is now your friend"));
	Refresh();
}

void UArcRmlDemoSocialViewModel::Decline(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo || !PendingRequests.IsValidIndex(Index))
	{
		return;
	}
	const FString Name = PendingRequests[Index].Name;
	PendingRequests.RemoveAt(Index);
	Demo->ShowToast(TEXT("info"), TEXT("Request declined"), Name);
	Refresh();
}

void UArcRmlDemoSocialViewModel::LeaveParty()
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	if (!Demo)
	{
		return;
	}
	Members.Reset();
	FArcRmlDemoPlayerRow& Self = Members.AddDefaulted_GetRef();
	Self.Label = PlayerName;
	Self.Level = PlayerLevel;
	Self.Role = TEXT("You");
	Self.Status = TEXT("not-ready");
	Self.bHost = true;
	for (int32 Index = 1; Index < StartParty.Num(); ++Index)
	{
		Members.AddDefaulted_GetRef().Status = TEXT("empty");
	}
	Demo->ShowToast(TEXT("info"), TEXT("You left the party"));
	Refresh();
}

void UArcRmlDemoSocialViewModel::ToggleReady()
{
	FArcRmlDemoPlayerRow* Self = Members.FindByPredicate([this](const FArcRmlDemoPlayerRow& Member) -> bool { return Member.Label == PlayerName; });
	if (!Self)
	{
		return;
	}
	Self->Status = Self->Status == TEXT("ready") ? TEXT("not-ready") : TEXT("ready");
	Refresh();
}

void UArcRmlDemoSocialViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (MenuRow != INDEX_NONE || Bar != TEXT("sub") || !TabLabels.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, SubTab, Index);
}

void UArcRmlDemoSocialViewModel::WriteText(const FString& Field, const FString& Value)
{
	using namespace ArcRmlDemoSocialViewModelPrivate;

	if (Field == SearchField)
	{
		ARC_RML_DEMO_SET(*this, Query, Value);
		Refresh();
	}
	else if (Field == AddField)
	{
		ARC_RML_DEMO_SET(*this, AddId, Value);
		Refresh();
	}
}

TArray<int32> UArcRmlDemoSocialViewModel::View() const
{
	const FString Search = Query.TrimStartAndEnd();
	TArray<int32> Result;
	for (int32 Index = 0; Index < Friends.Num(); ++Index)
	{
		if (Search.IsEmpty() || Friends[Index].Name.Contains(Search, ESearchCase::IgnoreCase))
		{
			Result.Add(Index);
		}
	}
	return Result;
}

const FArcRmlDemoFriend* UArcRmlDemoSocialViewModel::FriendAt(int32 ViewIndex) const
{
	const TArray<int32> Shown = View();
	return Shown.IsValidIndex(ViewIndex) ? &Friends[Shown[ViewIndex]] : nullptr;
}

int32 UArcRmlDemoSocialViewModel::EmptyPartySlot() const
{
	return Members.IndexOfByPredicate([](const FArcRmlDemoPlayerRow& Member) -> bool { return Member.Status == TEXT("empty"); });
}

void UArcRmlDemoSocialViewModel::InviteFriend(const FArcRmlDemoFriend& Invited)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	const int32 Empty = EmptyPartySlot();
	const bool bInParty = Members.ContainsByPredicate([&Invited](const FArcRmlDemoPlayerRow& Member) -> bool { return Member.Label == Invited.Name; });
	if (!Demo || Empty == INDEX_NONE || bInParty)
	{
		return;
	}
	FArcRmlDemoPlayerRow& Slot = Members[Empty];
	Slot.Label = Invited.Name;
	Slot.Level = 20 + (Invited.Name.Len() * 3) % 25;
	Slot.Role = TEXT("Invited");
	Slot.Status = TEXT("not-ready");
	Slot.bHost = false;
	Demo->ShowToast(TEXT("success"), TEXT("Invited ") + Invited.Name);
}

void UArcRmlDemoSocialViewModel::CloseMenu()
{
	ARC_RML_DEMO_SET(*this, MenuRow, INDEX_NONE);
}

void UArcRmlDemoSocialViewModel::Refresh()
{
	using namespace ArcRmlDemoSocialViewModelPrivate;

	const TArray<int32> Shown = View();
	TArray<FArcRmlDemoFriendGroup> GroupRows;
	int32 Online = 0;
	if (!Shown.IsEmpty())
	{
		for (const FArcRmlDemoPresenceGroup& Def : PresenceGroups)
		{
			FArcRmlDemoFriendGroup& Group = GroupRows.AddDefaulted_GetRef();
			Group.Key = Def.Key;
			Group.Label = Def.Label;
			for (int32 ViewIndex = 0; ViewIndex < Shown.Num(); ++ViewIndex)
			{
				const FArcRmlDemoFriend& Entry = Friends[Shown[ViewIndex]];
				if (Entry.Presence != Def.Key)
				{
					continue;
				}
				FArcRmlDemoFriendRow& Row = Group.Rows.AddDefaulted_GetRef();
				Row.Name = Entry.Name;
				Row.Tag = Entry.Tag.StartsWith(TEXT("#")) ? Entry.Tag : TEXT("#") + Entry.Tag;
				Row.Initials = Initials(Entry.Name);
				Row.Presence = Entry.Presence;
				Row.Status = Entry.Status;
				Row.bInGame = Entry.bInGame;
				Row.Party = Entry.Party;
				Row.Index = ViewIndex;
				if (ViewIndex == MenuRow)
				{
					Row.Menu = FriendMenu;
				}
			}
			Group.Count = Group.Rows.Num();
		}
	}
	for (const int32 Index : Shown)
	{
		Online += Friends[Index].Presence == TEXT("online") ? 1 : 0;
	}

	TArray<FArcRmlDemoPlayerRow> PartyRows = Members;
	int32 InParty = 0;
	for (FArcRmlDemoPlayerRow& Row : PartyRows)
	{
		Row.Initials = Row.Label.Left(2).ToUpper();
		InParty += Row.Status != TEXT("empty") ? 1 : 0;
	}

	TArray<FArcRmlDemoTabRow> TabRows;
	for (int32 Index = 0; Index < TabLabels.Num(); ++Index)
	{
		const int32 Count = Index == 0 ? Friends.Num() : Index == 1 ? InParty : Index == 2 ? PendingRequests.Num() : 0;
		FArcRmlDemoTabRow& Row = TabRows.AddDefaulted_GetRef();
		Row.Label = TabLabels[Index];
		Row.Badge = Count > 0 ? FString::FromInt(Count) : FString();
	}

	ARC_RML_DEMO_SET(*this, Groups, GroupRows);
	ARC_RML_DEMO_SET(*this, FriendsSub, FString::Printf(TEXT("%d friends · %d online"), Shown.Num(), Online));
	ARC_RML_DEMO_SET(*this, Party, PartyRows);
	ARC_RML_DEMO_SET(*this, Requests, PendingRequests);
	ARC_RML_DEMO_SET(*this, Tabs, TabRows);
	ARC_RML_DEMO_SET(*this, bNoResults, Shown.IsEmpty());
	ARC_RML_DEMO_SET(*this, EmptyTitle, TEXT("No friends match “") + Query.TrimStartAndEnd() + TEXT("”"));
	ARC_RML_DEMO_SET(*this, bCanInvite, EmptyPartySlot() != INDEX_NONE);
	ARC_RML_DEMO_SET(*this, bCanSend, !AddId.TrimStartAndEnd().IsEmpty());
	UArcRmlDemoHubViewModel::RefreshFor(GetDemo());
}
