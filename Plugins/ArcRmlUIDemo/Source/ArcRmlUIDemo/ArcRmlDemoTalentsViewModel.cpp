// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDemoTalentsViewModel.h"

#include "ArcRmlDemoGameViewModel.h"
#include "ArcRmlDemoHubViewModel.h"
#include "ArcRmlDemoSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArcRmlDemoTalentsViewModel)

namespace ArcRmlDemoTalentsViewModelPrivate
{
	FArcRmlDemoFactRow Fact(const FString& Label, const FString& Value)
	{
		FArcRmlDemoFactRow Result;
		Result.Label = Label;
		Result.Value = Value;
		return Result;
	}

	FArcRmlDemoModalButtonRow Button(const TCHAR* Label, const TCHAR* Key = TEXT(""))
	{
		FArcRmlDemoModalButtonRow Result;
		Result.Label = Label;
		Result.Key = Key;
		return Result;
	}

	int32 TalentX(int32 Column)
	{
		return 180 + Column * 140;
	}

	int32 TalentY(int32 Tier)
	{
		return 64 + Tier * 126;
	}
}

void UArcRmlDemoTalentsViewModel::Initialize()
{
	Ranks.Reset();
	for (const FArcRmlDemoTalentTree& Def : TreeDefs)
	{
		Ranks.Add(Def.StartRanks);
	}
	Available = StartPoints;
	Selected = FirstSelected;
	Refresh();
}

void UArcRmlDemoTalentsViewModel::Back()
{
	if (bConfirm)
	{
		Choose(0);
		return;
	}
	Super::Back();
}

void UArcRmlDemoTalentsViewModel::Learn(int32 Index)
{
	if (bConfirm || !Ranks.IsValidIndex(SubTab))
	{
		return;
	}
	SelectNode(Index);
	const int32 Node = SelectedIndex();
	const FTree Tree = BuildTree(SubTab);
	if (Tree.Nodes.IsValidIndex(Node) && Tree.Nodes[Node].bCanLearn)
	{
		Ranks[SubTab].Add(Talents[Node].Id, Tree.Nodes[Node].Rank + 1);
		Available -= 1;
	}
	Refresh();
}

void UArcRmlDemoTalentsViewModel::Unlearn(int32 Index)
{
	if (bConfirm || !Ranks.IsValidIndex(SubTab))
	{
		return;
	}
	SelectNode(Index);
	const int32 Node = SelectedIndex();
	const FTree Tree = BuildTree(SubTab);
	if (Tree.Nodes.IsValidIndex(Node) && Tree.Nodes[Node].bCanUnlearn)
	{
		Ranks[SubTab].Add(Talents[Node].Id, Tree.Nodes[Node].Rank - 1);
		Available += 1;
	}
	Refresh();
}

void UArcRmlDemoTalentsViewModel::ResetTree(int32 Index)
{
	if (bConfirm || BuildTree(SubTab).Spent <= 0)
	{
		return;
	}
	SelectNode(Index);
	ARC_RML_DEMO_SET(*this, bConfirm, true);
	Refresh();
}

void UArcRmlDemoTalentsViewModel::Choose(int32 Index)
{
	UArcRmlDemoSubsystem* Demo = GetDemo();
	UArcRmlDemoGameViewModel* Game = GetGame();
	if (!bConfirm || !Demo || !Game)
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, bConfirm, false);
	if (Index == 1 && Ranks.IsValidIndex(SubTab))
	{
		const int32 Spent = BuildTree(SubTab).Spent;
		Ranks[SubTab].Reset();
		Available += Spent;
		Game->SpendGold(FMath::Min(Game->Gold, ResetGold));
		Demo->ShowToast(TEXT("warning"), TreeDefs[SubTab].Label + TEXT(" tree reset"),
			FString::Printf(TEXT("%d points refunded · %s gold"), Spent, *UArcRmlDemoGameViewModel::FormatNumber(ResetGold)));
	}
	Refresh();
}

void UArcRmlDemoTalentsViewModel::TabSelected(const FString& Bar, int32 Index)
{
	if (bConfirm || Bar != TEXT("sub") || !TreeDefs.IsValidIndex(Index))
	{
		return;
	}
	ARC_RML_DEMO_SET(*this, SubTab, Index);
	Refresh();
}

UArcRmlDemoTalentsViewModel::FTree UArcRmlDemoTalentsViewModel::BuildTree(int32 TreeIndex) const
{
	FTree Result;
	const TMap<FString, int32>* TreeRanks = Ranks.IsValidIndex(TreeIndex) ? &Ranks[TreeIndex] : nullptr;
	TMap<FString, int32> RankOf;
	for (const FArcRmlDemoTalentDef& Def : Talents)
	{
		const int32 Rank = TreeRanks ? TreeRanks->FindRef(Def.Id) : 0;
		RankOf.Add(Def.Id, Rank);
		Result.Spent += Rank;
	}
	for (const FArcRmlDemoTalentDef& Def : Talents)
	{
		FNode Node;
		Node.Rank = RankOf[Def.Id];
		bool bParentOn = Def.Parents.IsEmpty();
		for (const FString& Parent : Def.Parents)
		{
			bParentOn |= RankOf.FindRef(Parent) > 0;
		}
		Node.bOpen = bParentOn && Result.Spent >= Def.Tier * TierCost;
		Node.bCanLearn = Node.bOpen && Node.Rank < Def.MaxRank && Available > 0;
		bool bChildDepends = false;
		bool bTierBreak = false;
		for (const FArcRmlDemoTalentDef& Other : Talents)
		{
			const int32 OtherRank = RankOf[Other.Id];
			if (OtherRank > 0 && Other.Parents.Contains(Def.Id))
			{
				bool bOtherParent = false;
				for (const FString& Parent : Other.Parents)
				{
					bOtherParent |= Parent != Def.Id && RankOf.FindRef(Parent) > 0;
				}
				bChildDepends |= !bOtherParent;
			}
			bTierBreak |= OtherRank > 0 && Other.Tier > Def.Tier && Result.Spent - 1 < Other.Tier * TierCost;
		}
		Node.bCanUnlearn = Node.Rank > 0 && !(Node.Rank == 1 && bChildDepends) && !bTierBreak;
		Result.Nodes.Add(Node);
	}
	return Result;
}

int32 UArcRmlDemoTalentsViewModel::IndexOf(const FString& Id) const
{
	return Talents.IndexOfByPredicate([&Id](const FArcRmlDemoTalentDef& Def) -> bool { return Def.Id == Id; });
}

int32 UArcRmlDemoTalentsViewModel::SelectedIndex() const
{
	return FMath::Max(0, IndexOf(Selected));
}

void UArcRmlDemoTalentsViewModel::SelectNode(int32 Index)
{
	if (Talents.IsValidIndex(Index))
	{
		ARC_RML_DEMO_SET(*this, Selected, Talents[Index].Id);
	}
}

int32 UArcRmlDemoTalentsViewModel::NearestInTier(int32 Tier, int32 Column) const
{
	int32 Best = INDEX_NONE;
	for (int32 Other = 0; Other < Talents.Num(); ++Other)
	{
		if (Talents[Other].Tier != Tier)
		{
			continue;
		}
		const int32 Distance = FMath::Abs(Talents[Other].Column - Column);
		const int32 BestDistance = Best == INDEX_NONE ? MAX_int32 : FMath::Abs(Talents[Best].Column - Column);
		if (Distance < BestDistance || (Distance == BestDistance && Talents[Other].Column < Talents[Best].Column))
		{
			Best = Other;
		}
	}
	return Best;
}

FBox2f UArcRmlDemoTalentsViewModel::NodeBox(int32 Index) const
{
	using namespace ArcRmlDemoTalentsViewModelPrivate;

	const FArcRmlDemoTalentDef& Def = Talents[Index];
	const float Half = Def.bKeystone ? 38.0f : 28.0f;
	const FVector2f Center(TalentX(Def.Column), TalentY(Def.Tier));
	return FBox2f(Center - FVector2f(Half, Half), Center + FVector2f(Half, Half));
}

// The prototype's explicit neighbour (TALENT_NAV, GameUI.dc.html:187-196); INDEX_NONE when it names no node.
int32 UArcRmlDemoTalentsViewModel::PrototypeNeighbour(int32 Index, EDirection Direction) const
{
	const FArcRmlDemoTalentDef& Node = Talents[Index];
	if (Direction == EDirection::Up || Direction == EDirection::Down)
	{
		return NearestInTier(Node.Tier + (Direction == EDirection::Up ? -1 : 1), Node.Column);
	}
	int32 Neighbour = INDEX_NONE;
	for (int32 Other = 0; Other < Talents.Num(); ++Other)
	{
		if (Talents[Other].Tier != Node.Tier || Other == Index)
		{
			continue;
		}
		const int32 Step = Talents[Other].Column - Node.Column;
		const bool bSide = Direction == EDirection::Left ? Step < 0 : Step > 0;
		if (bSide && (Neighbour == INDEX_NONE || FMath::Abs(Step) < FMath::Abs(Talents[Neighbour].Column - Node.Column)))
		{
			Neighbour = Other;
		}
	}
	return Neighbour;
}

// The node spatial navigation picks: the fork's score (Core/ElementDocument.cpp GetNavigationHeuristic) on the node boxes.
int32 UArcRmlDemoTalentsViewModel::GeometryNeighbour(int32 Index, EDirection Direction) const
{
	const FBox2f Source = NodeBox(Index);
	int32 Best = INDEX_NONE;
	int64 BestScore = MAX_int64;
	for (int32 Other = 0; Other < Talents.Num(); ++Other)
	{
		if (Other == Index)
		{
			continue;
		}
		const FBox2f Target = NodeBox(Other);
		const bool bVertical = Direction == EDirection::Up || Direction == EDirection::Down;
		const FBox2f& Near = Direction == EDirection::Up || Direction == EDirection::Left ? Source : Target;
		const FBox2f& Far = Direction == EDirection::Up || Direction == EDirection::Left ? Target : Source;
		const int32 Axis = bVertical ? 1 : 0;
		const int32 Cross = bVertical ? 0 : 1;
		const int64 Main = static_cast<int64>(Near.Min[Axis] - Far.Max[Axis]);
		if (Main < 0)
		{
			continue;
		}
		const int64 CrossGap = static_cast<int64>(FMath::Max(0.0f, Target.Min[Cross] - Source.Max[Cross])) + static_cast<int64>(FMath::Max(0.0f, Source.Min[Cross] - Target.Max[Cross]));
		const int64 Score = Main + 10000 * CrossGap;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Other;
		}
	}
	return Best;
}

FString UArcRmlDemoTalentsViewModel::NavOverride(int32 Index, EDirection Direction) const
{
	const int32 Prototype = PrototypeNeighbour(Index, Direction);
	if (Prototype == INDEX_NONE || Prototype == GeometryNeighbour(Index, Direction))
	{
		return FString();
	}
	return TEXT("tn-") + Talents[Prototype].Id;
}

void UArcRmlDemoTalentsViewModel::Refresh()
{
	using namespace ArcRmlDemoTalentsViewModelPrivate;

	if (Talents.IsEmpty() || !TreeDefs.IsValidIndex(SubTab))
	{
		return;
	}
	const FTree Tree = BuildTree(SubTab);

	TArray<FArcRmlDemoTalentNodeRow> NodeRows;
	TArray<FArcRmlDemoTalentLinkRow> LinkRows;
	for (int32 Index = 0; Index < Talents.Num(); ++Index)
	{
		const FNode& Node = Tree.Nodes[Index];
		const FArcRmlDemoTalentDef& Def = Talents[Index];
		FArcRmlDemoTalentNodeRow& Row = NodeRows.AddDefaulted_GetRef();
		Row.Id = Def.Id;
		Row.Tier = Def.Tier;
		Row.Column = Def.Column;
		Row.Icon = Def.Icon;
		Row.Rank = Node.Rank;
		Row.MaxRank = Def.MaxRank;
		Row.State = Node.Rank >= Def.MaxRank ? TEXT("maxed") : Node.Rank > 0 ? TEXT("allocated") : Node.bOpen ? TEXT("available") : TEXT("locked");
		Row.bKeystone = Def.bKeystone;
		Row.X = TalentX(Def.Column);
		Row.Y = TalentY(Def.Tier);
		Row.NavUp = NavOverride(Index, EDirection::Up);
		Row.NavDown = NavOverride(Index, EDirection::Down);
		Row.NavLeft = NavOverride(Index, EDirection::Left);
		Row.NavRight = NavOverride(Index, EDirection::Right);
		for (const FString& Parent : Def.Parents)
		{
			const int32 ParentIndex = IndexOf(Parent);
			if (!Talents.IsValidIndex(ParentIndex))
			{
				continue;
			}
			const FArcRmlDemoTalentDef& From = Talents[ParentIndex];
			const float DeltaX = TalentX(Def.Column) - TalentX(From.Column);
			const float DeltaY = TalentY(Def.Tier) - TalentY(From.Tier);
			FArcRmlDemoTalentLinkRow& Link = LinkRows.AddDefaulted_GetRef();
			Link.X1 = TalentX(From.Column);
			Link.Y1 = TalentY(From.Tier);
			Link.Len = FMath::Sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
			Link.Ang = FMath::RadiansToDegrees(FMath::Atan2(DeltaY, DeltaX));
			Link.bActive = Tree.Nodes[ParentIndex].Rank > 0 && Node.Rank > 0;
		}
	}

	TArray<FArcRmlDemoTierRow> TierRows;
	for (int32 Tier = 0; Tier < TierCount; ++Tier)
	{
		FArcRmlDemoTierRow& Row = TierRows.AddDefaulted_GetRef();
		Row.Label = FString::Printf(TEXT("Tier %d"), Tier + 1);
		Row.Req = FString::Printf(TEXT("%d pts"), Tier * TierCost);
		Row.Top = TalentY(Tier) - 63;
		Row.bLocked = Tree.Spent < Tier * TierCost;
		Row.bFirst = Tier == 0;
	}

	TArray<FArcRmlDemoTabRow> TreeRows;
	for (int32 Index = 0; Index < TreeDefs.Num(); ++Index)
	{
		FArcRmlDemoTabRow& Row = TreeRows.AddDefaulted_GetRef();
		Row.Label = TreeDefs[Index].Label;
		Row.Icon = TreeDefs[Index].Icon;
		Row.Badge = FString::FromInt(BuildTree(Index).Spent);
	}

	const int32 Shown = SelectedIndex();
	const FNode& Node = Tree.Nodes[Shown];
	const FArcRmlDemoTalentDef& Def = Talents[Shown];
	const FString& TreeLabel = TreeDefs[SubTab].Label;
	const int32 Per = 4 + Def.Tier;
	TArray<FString> Requires;
	for (const FString& Parent : Def.Parents)
	{
		const int32 ParentIndex = IndexOf(Parent);
		if (Talents.IsValidIndex(ParentIndex))
		{
			Requires.Add(Talents[ParentIndex].Name);
		}
	}
	FArcRmlDemoTalentDetail Info;
	Info.Name = Def.Name.IsEmpty() ? FString(TEXT("Talent")) : Def.Name;
	Info.Sub = FString::Printf(TEXT("%s · %s · Tier %d"), Def.bKeystone ? TEXT("Keystone") : TEXT("Passive"), *TreeLabel, Def.Tier + 1);
	Info.Icon = Def.Icon;
	Info.Rank = Node.Rank;
	Info.MaxRank = Def.MaxRank;
	Info.RankLabel = FString::Printf(TEXT("%d / %d"), Node.Rank, Def.MaxRank);
	Info.Current = Node.Rank ? FString::Printf(TEXT("%s grants +%d%% to its effect."), *Info.Name, Per * Node.Rank) : FString(TEXT("Not learned yet."));
	Info.Next = Node.Rank < Def.MaxRank ? FString::Printf(TEXT("Rank %d: +%d%%."), Node.Rank + 1, Per * (Node.Rank + 1)) : FString(TEXT("Maximum rank."));
	Info.Facts = {Fact(TEXT("Requires"), Requires.IsEmpty() ? FString(TEXT("—")) : FString::Join(Requires, TEXT(" or "))), Fact(TEXT("Cost"), TEXT("1 point")),
		Fact(TEXT("Tier unlock"), FString::Printf(TEXT("%d points in tree"), Def.Tier * TierCost))};

	ARC_RML_DEMO_SET(*this, Nodes, NodeRows);
	ARC_RML_DEMO_SET(*this, Links, LinkRows);
	ARC_RML_DEMO_SET(*this, Tiers, TierRows);
	ARC_RML_DEMO_SET(*this, Trees, TreeRows);
	ARC_RML_DEMO_SET(*this, Selected, Def.Id);
	ARC_RML_DEMO_SET(*this, Detail, Info);
	ARC_RML_DEMO_SET(*this, bCanLearn, Node.bCanLearn);
	ARC_RML_DEMO_SET(*this, bCanUnlearn, Node.bCanUnlearn);
	ARC_RML_DEMO_SET(*this, bCanReset, Tree.Spent > 0);
	ARC_RML_DEMO_SET(*this, ResetTitle, FString::Printf(TEXT("Reset the %s tree?"), *TreeLabel));
	ARC_RML_DEMO_SET(*this, ResetBody, FString::Printf(TEXT("All %d points spent in %s are refunded and every talent in this tree is unlearned. Resetting costs %s gold."),
		Tree.Spent, *TreeLabel, *UArcRmlDemoGameViewModel::FormatNumber(ResetGold)));
	ARC_RML_DEMO_SET(*this, ModalButtons, TArray<FArcRmlDemoModalButtonRow>({Button(TEXT("Cancel")), Button(TEXT("Reset tree"), TEXT("reset"))}));
	UArcRmlDemoHubViewModel::RefreshFor(GetDemo());
}
