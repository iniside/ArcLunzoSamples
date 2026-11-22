// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "CQTest.h"
#include "Host/ArcRmlReloadPlan.h"

namespace ArcRmlReloadPlanTestsPrivate
{
	FArcRmlReloadEntry MakeEntry(const FString& ViewPath, int32 LayerIndex, uint64 OpenOrder, bool bHeadsRead)
	{
		FArcRmlReloadEntry Entry;
		Entry.ViewPath = ViewPath;
		Entry.LayerIndex = LayerIndex;
		Entry.OpenOrder = OpenOrder;
		Entry.bHeadsRead = bHeadsRead;
		Entry.ReopenViewPath = bHeadsRead ? ViewPath : FString();
		return Entry;
	}

	FArcRmlReloadEntry MakeTabEntry(const FString& ViewPath, int32 LayerIndex, uint64 OpenOrder, const FString& FramePath)
	{
		FArcRmlReloadEntry Entry = MakeEntry(ViewPath, LayerIndex, OpenOrder, true);
		Entry.ReopenViewPath = FramePath;
		Entry.bReopensTab = true;
		return Entry;
	}

	EArcRmlReloadAction FindAction(TConstArrayView<FArcRmlReloadStep> Steps, TConstArrayView<FArcRmlReloadEntry> Entries, const FString& ViewPath)
	{
		for (const FArcRmlReloadStep& Step : Steps)
		{
			if (Entries[Step.Entry].ViewPath == ViewPath)
			{
				return Step.Action;
			}
		}
		return EArcRmlReloadAction::Close;
	}

	const TCHAR* GetActionName(EArcRmlReloadAction Action)
	{
		switch (Action)
		{
		case EArcRmlReloadAction::Close:
			return TEXT("Close");
		case EArcRmlReloadAction::Keep:
			return TEXT("Keep");
		case EArcRmlReloadAction::Reopen:
			return TEXT("Reopen");
		}
		return TEXT("Unknown");
	}

	FString Describe(TConstArrayView<FArcRmlReloadStep> Steps, TConstArrayView<FArcRmlReloadEntry> Entries)
	{
		TArray<FString> Lines;
		for (const FArcRmlReloadStep& Step : Steps)
		{
			Lines.Add(FString::Printf(TEXT("%s=%s"), *Entries[Step.Entry].ViewPath, GetActionName(Step.Action)));
		}
		return FString::Join(Lines, TEXT(", "));
	}
}

TEST_CLASS(ArcRmlReloadPlanTests, "ArcRmlUI.ReloadPlan")
{
	void ExpectAction(TConstArrayView<FArcRmlReloadStep> Steps, TConstArrayView<FArcRmlReloadEntry> Entries, const FString& ViewPath, EArcRmlReloadAction Expected)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		const EArcRmlReloadAction Actual = FindAction(Steps, Entries, ViewPath);
		ASSERT_THAT(AreEqual(FString(GetActionName(Expected)), FString(GetActionName(Actual)), FString::Printf(TEXT("%s: %s"), *ViewPath, *Describe(Steps, Entries))));
	}

	TEST_METHOD(NewerViewBrokenInALayer_KeepsTheWholeLayer)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> Entries;
		Entries.Add(MakeEntry(TEXT("/Test/Dialog.rml"), 2, 5, false));
		Entries.Add(MakeEntry(TEXT("/Test/Inventory.rml"), 2, 3, true));
		Entries.Add(MakeEntry(TEXT("/Test/Hud.rml"), 0, 1, true));
		const TArray<FArcRmlReloadStep> Steps = ArcRmlReloadPlan::Plan(Entries);
		ExpectAction(Steps, Entries, TEXT("/Test/Dialog.rml"), EArcRmlReloadAction::Keep);
		ExpectAction(Steps, Entries, TEXT("/Test/Inventory.rml"), EArcRmlReloadAction::Keep);
		ExpectAction(Steps, Entries, TEXT("/Test/Hud.rml"), EArcRmlReloadAction::Reopen);
	}

	TEST_METHOD(BrokenViewInOtherLayer_KeepsOnlyItsLayer)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> Entries;
		Entries.Add(MakeEntry(TEXT("/Test/Menu.rml"), 1, 2, true));
		Entries.Add(MakeEntry(TEXT("/Test/Modal.rml"), 3, 4, false));
		const TArray<FArcRmlReloadStep> Steps = ArcRmlReloadPlan::Plan(Entries);
		ExpectAction(Steps, Entries, TEXT("/Test/Menu.rml"), EArcRmlReloadAction::Reopen);
		ExpectAction(Steps, Entries, TEXT("/Test/Modal.rml"), EArcRmlReloadAction::Keep);
	}

	TEST_METHOD(BrokenTabInFrame_KeepsTheFrame)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> Entries;
		Entries.Add(MakeEntry(TEXT("/Test/Frame.rml"), 1, 2, false));
		Entries.Add(MakeEntry(TEXT("/Test/Hud.rml"), 0, 1, true));
		const TArray<FArcRmlReloadStep> Steps = ArcRmlReloadPlan::Plan(Entries);
		ExpectAction(Steps, Entries, TEXT("/Test/Frame.rml"), EArcRmlReloadAction::Keep);
		ExpectAction(Steps, Entries, TEXT("/Test/Hud.rml"), EArcRmlReloadAction::Reopen);
	}

	TEST_METHOD(DocumentBecomingTabOfAnOpenFrame_OpensTheFrameOnce)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> NewerTab;
		NewerTab.Add(MakeEntry(TEXT("/Test/Frame.rml"), 1, 1, true));
		NewerTab.Add(MakeTabEntry(TEXT("/Test/Document.rml"), 1, 2, TEXT("/Test/Frame.rml")));
		const TArray<FArcRmlReloadStep> NewerSteps = ArcRmlReloadPlan::Plan(NewerTab);
		ExpectAction(NewerSteps, NewerTab, TEXT("/Test/Frame.rml"), EArcRmlReloadAction::Reopen);
		ExpectAction(NewerSteps, NewerTab, TEXT("/Test/Document.rml"), EArcRmlReloadAction::Reopen);

		TArray<FArcRmlReloadEntry> OlderTab;
		OlderTab.Add(MakeTabEntry(TEXT("/Test/Document.rml"), 1, 1, TEXT("/Test/Frame.rml")));
		OlderTab.Add(MakeEntry(TEXT("/Test/Frame.rml"), 1, 2, true));
		const TArray<FArcRmlReloadStep> OlderSteps = ArcRmlReloadPlan::Plan(OlderTab);
		ExpectAction(OlderSteps, OlderTab, TEXT("/Test/Document.rml"), EArcRmlReloadAction::Reopen);
		ExpectAction(OlderSteps, OlderTab, TEXT("/Test/Frame.rml"), EArcRmlReloadAction::Close);
	}

	TEST_METHOD(LeavingViewInAKeptLayer_IsClosed)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> Entries;
		Entries.Add(MakeEntry(TEXT("/Test/Broken.rml"), 2, 2, false));
		FArcRmlReloadEntry Leaving = MakeEntry(TEXT("/Test/Leaving.rml"), 2, 1, false);
		Leaving.bLeaving = true;
		Entries.Add(Leaving);
		const TArray<FArcRmlReloadStep> Steps = ArcRmlReloadPlan::Plan(Entries);
		ExpectAction(Steps, Entries, TEXT("/Test/Broken.rml"), EArcRmlReloadAction::Keep);
		ExpectAction(Steps, Entries, TEXT("/Test/Leaving.rml"), EArcRmlReloadAction::Close);
	}

	TEST_METHOD(Reopens_RunInLayerThenOpenOrder)
	{
		using namespace ArcRmlReloadPlanTestsPrivate;
		TArray<FArcRmlReloadEntry> Entries;
		Entries.Add(MakeEntry(TEXT("/Test/C.rml"), 2, 1, true));
		Entries.Add(MakeEntry(TEXT("/Test/B.rml"), 1, 7, true));
		Entries.Add(MakeEntry(TEXT("/Test/A.rml"), 1, 3, true));
		const TArray<FArcRmlReloadStep> Steps = ArcRmlReloadPlan::Plan(Entries);
		ASSERT_THAT(AreEqual(3, Steps.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("/Test/A.rml")), Entries[Steps[0].Entry].ViewPath, Describe(Steps, Entries)));
		ASSERT_THAT(AreEqual(FString(TEXT("/Test/B.rml")), Entries[Steps[1].Entry].ViewPath, Describe(Steps, Entries)));
		ASSERT_THAT(AreEqual(FString(TEXT("/Test/C.rml")), Entries[Steps[2].Entry].ViewPath, Describe(Steps, Entries)));
	}
};
