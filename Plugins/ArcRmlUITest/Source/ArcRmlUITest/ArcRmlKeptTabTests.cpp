// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlKeptTabTestHelpers.h"
#include "ArcRmlKeptTabTestTypes.h"
#include "Containers/Ticker.h"
#include "CQTest.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "Preview/ArcRmlPreviewEmbed.h"
#include "UObject/StrongObjectPtr.h"

namespace ArcRmlKeptTabTestsPrivate
{
	using namespace ArcRmlKeptTabTestHelpers;

	const FColor CloneColour(20, 180, 220, 255);

	struct FReplayResult
	{
		int32 DrawCalls = 0;
		int32 RhiCommands = 0;
		TArray<FColor> Pixels;
	};

	bool Replay(FArcRmlTestView& View, FReplayResult& Out)
	{
		if (!View.RenderPixels())
		{
			return false;
		}
		const FIntPoint Size = View.GetPixelSize();
		Out.DrawCalls = View.GetReplayDrawCalls();
		Out.RhiCommands = View.GetReplayRhiCommands();
		Out.Pixels.Reset(Size.X * Size.Y);
		for (int32 Y = 0; Y < Size.Y; ++Y)
		{
			for (int32 X = 0; X < Size.X; ++X)
			{
				Out.Pixels.Add(View.GetPixel(X, Y));
			}
		}
		return true;
	}

	int32 CountDifferentPixels(const FReplayResult& A, const FReplayResult& B)
	{
		if (A.Pixels.Num() != B.Pixels.Num())
		{
			return INDEX_NONE;
		}
		int32 Different = 0;
		for (int32 Index = 0; Index < A.Pixels.Num(); ++Index)
		{
			Different += A.Pixels[Index] == B.Pixels[Index] ? 0 : 1;
		}
		return Different;
	}
}

TEST_CLASS(ArcRmlKeptTabTests, "ArcRmlUI.KeptTabs")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlKeptTabTestHelpers::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool Read()
	{
		return View.Flush() && View.ParseLayout(Elements);
	}

	bool OpenFrame(bool bPad)
	{
		return Assert.IsTrue(ArcRmlKeptTabTestHelpers::OpenFrame(View, bPad), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool Visit(const TCHAR* Relative)
	{
		return Assert.IsTrue(ArcRmlKeptTabTestHelpers::Visit(View, Relative), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View) + ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool SwapPreview(const TCHAR* Relative, TFunctionRef<UObject*(const FArcRmlViewModelDeclaration&)> FindModel)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(ArcRmlKeptTabTestHelpers::Frame);
		const FString TabPath = ArcRmlUITestHelpers::MakePath(Relative);
		TOptional<FArcRmlDocumentHead> Head = ArcRmlPreviewEmbed::ReadTab(TabPath, FramePath);
		if (!Assert.IsTrue(Head.IsSet(), TabPath))
		{
			return false;
		}
		const bool bSwapped = View.SwapWith(TabPath, [&Head, &FramePath, &TabPath, &FindModel](const TSharedRef<FArcRmlUIThread>& Thread, uint32 ViewId, FArcRmlModelBinding& FrameBinding,
			FArcRmlTabModels& Tabs) -> bool
		{
			return ArcRmlPreviewEmbed::SwapTab(Thread, ViewId, FrameBinding, Tabs, FramePath, TabPath, MoveTemp(Head->Declarations), FString(), FArcRmlValue(),
				FArcRmlOnDocumentAction(), FindModel);
		});
		if (!Assert.IsTrue(bSwapped, TabPath))
		{
			return false;
		}
		FTSTicker::GetCoreTicker().Tick(0.0f);
		return Assert.IsTrue(View.Flush() && View.Flush()) && Assert.AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Read();
	}

	bool PressAndRead(const FKey& Key)
	{
		View.PressKey(Key);
		return View.Flush() && View.Flush() && Read();
	}

	FVector2f CentreOf(const TCHAR* DomId) const
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? Element->Rect.Position + Element->Rect.Size * 0.5f : FVector2f::ZeroVector;
	}

	float WaitForScroll(const TCHAR* DomId)
	{
		float Previous = -1.0f;
		int32 Stable = 0;
		float ScrollY = 0.0f;
		for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			if (!Read())
			{
				return -1.0f;
			}
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
			if (!Element)
			{
				return -1.0f;
			}
			ScrollY = Element->Scroll.Y;
			Stable = FMath::IsNearlyEqual(ScrollY, Previous, 0.01f) ? Stable + 1 : 0;
			Previous = ScrollY;
		}
		return ScrollY;
	}

	TEST_METHOD(TabsDeclaringTheSameModel_BothEmbedAndReadIt)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(AreEqual(0, View.GetHeadDiagnostics().Num(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("shared"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Count")), ArcRmlUITestHelpers::MakeInt(7));
		View.SetModelValue(TEXT("owna"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Count")), ArcRmlUITestHelpers::MakeInt(3));
		View.SetModelValue(TEXT("ownb"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Count")), ArcRmlUITestHelpers::MakeInt(4));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("7")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kb-shared")), TEXT("the shown tab B does not read the shared model")));
		ASSERT_THAT(AreEqual(FString(TEXT("4")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kb-own")), TEXT("tab B does not read its own model")));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(AreEqual(FString(TEXT("7")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ka-shared")), TEXT("tab A does not read the shared model")));
		ASSERT_THAT(AreEqual(FString(TEXT("3")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ka-own")), TEXT("tab A does not read its own model")));

		TSharedPtr<FArcRmlModelBinding> Shared;
		TSharedPtr<FArcRmlModelBinding> OwnA;
		TSharedPtr<FArcRmlModelBinding> OwnB;
		const bool bInspected = View.SwapWith(ArcRmlUITestHelpers::MakePath(TabA), [&Shared, &OwnA, &OwnB](const TSharedRef<FArcRmlUIThread>&, uint32, FArcRmlModelBinding&,
			FArcRmlTabModels& Tabs) -> bool
		{
			Shared = Tabs.FindModelBinding(TEXT("shared"));
			OwnA = Tabs.FindModelBinding(TEXT("owna"));
			OwnB = Tabs.FindModelBinding(TEXT("ownb"));
			return false;
		});
		ASSERT_THAT(IsFalse(bInspected));
		ASSERT_THAT(IsTrue(Shared.IsValid() && OwnA.IsValid() && OwnB.IsValid(), TEXT("a tab model has no binding")));
		ASSERT_THAT(IsTrue(Shared != OwnA && Shared != OwnB && OwnA != OwnB, TEXT("the shared model and the tab models are not three bindings")));
		ASSERT_THAT(IsTrue(Shared->DeclaresModel(TEXT("shared")) && !Shared->DeclaresModel(TEXT("owna")) && !Shared->DeclaresModel(TEXT("ownb")), TEXT("the shared binding holds a tab model")));
	}

	TEST_METHOD(CallOnTheSharedModelAndOnEachOwnModel_ReachTheirOneBinding)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		FLogCapture Log;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Visit(TabB)));

		View.ClickElement(TEXT("#kb-ping"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), Log.Describe()));
		ASSERT_THAT(AreEqual(FString(TEXT("shared")), View.GetCalls()[0].ModelName));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("[preview] call('', 'Ping') on 'shared'")), Log.Describe()));

		View.ClickElement(TEXT("#kb-ownping"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("[preview] call('', 'Ping') on 'ownb'")), Log.Describe()));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		View.ClickElement(TEXT("#ka-ownping"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("[preview] call('', 'Ping') on 'owna'")), Log.Describe()));
		View.ClickElement(TEXT("#ka-ping"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(2, Log.CountContaining(TEXT("[preview] call('', 'Ping') on 'shared'")), TEXT("a call on the shared model from tab A did not reach the one binding")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("[preview] call('', 'Ping') on 'ownb'")), TEXT("a call of tab A reached tab B's model")));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("unknown view model")), Log.Describe()));
	}

	FString DescribeStyleComputes()
	{
		return FString::Printf(TEXT("StyleComputes %lld Properties %lld Variables %lld First %lld OwnDefinition %lld OpacityOnly %lld"), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes),
			View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesProperties), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesVariables),
			View.GetPerfCounter(Rml::ArcPerf::Counter::StyleFirstComputes), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesOwnDefinition),
			View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesOpacityOnly));
	}

	TEST_METHOD(SwitchBackToAKeptTab_InstancesAndRestylesNothing)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		RunConsole(TEXT("ArcRml.ClockAdvance 5.0"));
		ASSERT_THAT(IsTrue(SettleFrames(View)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(Visit(TabB)));
		const int64 InstancedByFirstVisit = View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced);
		ASSERT_THAT(IsTrue(InstancedByFirstVisit > 10, FString::Printf(TEXT("the first visit of tab B instanced %lld elements, so the counter is not measuring"), InstancedByFirstVisit)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes) > 0, TEXT("the first visit computed no styles, so the counter is not measuring")));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(int64(3), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes), DescribeStyleComputes()));
		ASSERT_THAT(AreEqual(int64(3), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesProperties), DescribeStyleComputes()));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesVariables), TEXT("showing the kept tab A recomputed variables")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced), TEXT("showing the kept tab A instanced elements")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::TextNodesInstanced), TEXT("showing the kept tab A instanced text nodes")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleSheetsParsed), TEXT("showing the kept tab A parsed a style sheet")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleFirstComputes), TEXT("showing the kept tab A computed the style of a new element")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesOwnDefinition), TEXT("showing the kept tab A matched a new definition")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::DefinitionsChanged), TEXT("showing the kept tab A changed a definition")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::DataViewsCreated), TEXT("showing the kept tab A created data views")));
		const FArcRmlTestLayoutElement* Title = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-title"));
		ASSERT_THAT(IsTrue(Title && Title->bVisible, TEXT("tab A is not visible after the switch back")));
	}

	TEST_METHOD(SwitchBack_KeepsScrollAndFieldText)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));

		const FVector2f ScrollPoint = CentreOf(TEXT("ka-scroll"));
		View.MouseMove(ScrollPoint);
		ASSERT_THAT(IsTrue(View.Flush()));
		View.MouseWheel(ScrollPoint, -1.0f);
		const float ScrollBefore = WaitForScroll(TEXT("ka-scroll"));
		ASSERT_THAT(IsTrue(ScrollBefore > 10.0f, FString::Printf(TEXT("the wheel did not scroll tab A, scroll %g"), ScrollBefore)));

		View.ClickElement(TEXT("#ka-field"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		View.TypeText(TEXT("abc"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("abc")), View.GetSnapshot().TextField.Text, TEXT("the field did not take the text")));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(IsFalse(View.GetSnapshot().TextField.bActive, TEXT("the hidden tab's field still takes text input")));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		const FArcRmlTestLayoutElement* Scroller = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-scroll"));
		ASSERT_THAT(IsNotNull(Scroller));
		ASSERT_THAT(IsNear(ScrollBefore, Scroller->Scroll.Y, 0.5f, TEXT("the scroll offset of tab A did not survive the switch")));

		View.ClickElement(TEXT("#ka-field"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("abc")), View.GetSnapshot().TextField.Text, TEXT("the field text of tab A did not survive the switch")));
	}

	TEST_METHOD(SwitchBack_EntersThePrimaryAreaAgain)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(true)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("a first visit did not enter the primary area")));
		ASSERT_THAT(IsTrue(PressAndRead(EKeys::Gamepad_DPad_Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn2")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(AreEqual(FString(TEXT("kb-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("tab B did not enter its primary area, the area name is shared with tab A")));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty()));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("the kept tab A did not enter its primary area again")));
	}

	TEST_METHOD(SwitchBack_RestartsTheTabAnimation)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(SettleFrames(View) && Read()));
		const FArcRmlTestLayoutElement* Anim = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-anim"));
		ASSERT_THAT(IsNotNull(Anim));
		ASSERT_THAT(IsNear(100.0f, Anim->Rect.Size.X, 1.0f, TEXT("the animation does not start at 100 px")));

		RunConsole(TEXT("ArcRml.ClockAdvance 2.0"));
		ASSERT_THAT(IsTrue(SettleFrames(View) && Read()));
		Anim = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-anim"));
		ASSERT_THAT(IsNear(200.0f, Anim->Rect.Size.X, 1.0f, TEXT("100 to 300 px over 4 s linear is 200 px after 2 s")));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		RunConsole(TEXT("ArcRml.ClockAdvance 1.0"));
		ASSERT_THAT(IsTrue(SettleFrames(View)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(SettleFrames(View) && Read()));
		Anim = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-anim"));
		ASSERT_THAT(IsNear(100.0f, Anim->Rect.Size.X, 1.0f, TEXT("the animation of the shown kept tab did not restart, as Chrome restarts it when an element leaves display: none")));

		RunConsole(TEXT("ArcRml.ClockAdvance 1.0"));
		ASSERT_THAT(IsTrue(SettleFrames(View) && Read()));
		Anim = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-anim"));
		ASSERT_THAT(IsNear(150.0f, Anim->Rect.Size.X, 1.0f, TEXT("the restarted animation does not run from the moment the tab was shown")));
	}

	int64 StyleComputesOfAnAdvanceWhileTheFrameAnimates()
	{
		using namespace ArcRmlKeptTabTestHelpers;
		if (!Assert.IsTrue(View.ResetPerfCounters()))
		{
			return -1;
		}
		RunConsole(TEXT("ArcRml.ClockAdvance 0.5"));
		if (!Assert.IsTrue(SettleFrames(View)))
		{
			return -1;
		}
		return View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes);
	}

	TEST_METHOD(HiddenKeptTab_AnimationComputesNoStyleWhileTheShownTabsUpdate)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(CloseA) && SettleFrames(View)));
		const int64 WithoutTabA = StyleComputesOfAnAdvanceWhileTheFrameAnimates();
		ASSERT_THAT(IsTrue(WithoutTabA > 0, TEXT("the running frame animation computed no style on an advance, so nothing drives an update")));

		ASSERT_THAT(IsTrue(Visit(TabA) && SettleFrames(View)));
		ASSERT_THAT(IsTrue(Visit(CloseA) && SettleFrames(View)));
		ASSERT_THAT(AreEqual(WithoutTabA, StyleComputesOfAnAdvanceWhileTheFrameAnimates(),
			TEXT("the animation of the hidden kept tab A computed its style on an advance, Chrome cancels animations inside display: none")));
	}

	TEST_METHOD(ChangeToATabModelWhileHidden_AppearsAfterShowAndNothingChangesBefore)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Shared(NewObject<UArcRmlKeptTabViewModel>());
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> OwnA(NewObject<UArcRmlKeptTabViewModel>());
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> OwnB(NewObject<UArcRmlKeptTabViewModel>());
		const TFunction<UObject*(const FArcRmlViewModelDeclaration&)> FindModel = [&Shared, &OwnA, &OwnB](const FArcRmlViewModelDeclaration& Declaration) -> UObject*
		{
			if (Declaration.Name == TEXT("shared"))
			{
				return Shared.Get();
			}
			return Declaration.Name == TEXT("owna") ? OwnA.Get() : OwnB.Get();
		};
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(SwapPreview(TabA, FindModel)));
		ASSERT_THAT(IsTrue(SwapPreview(TabB, FindModel)));

		OwnB->SetCount(2);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("2")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kb-own")), TEXT("a change of the shown tab's own model does not appear, so the probe is not valid")));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		OwnA->SetCount(5);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("0")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ka-own")), TEXT("the hidden tab A changed while hidden")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::DataViewsUpdated), TEXT("a data view was updated by a change of the hidden tab's model")));

		Shared->SetCount(9);
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(FString(TEXT("9")), ArcRmlUITestHelpers::GetText(Elements, TEXT("kb-shared")), TEXT("the shared model, which the shown tab B declares, is paused")));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SwapPreview(TabA, FindModel)));
		FTSTicker::GetCoreTicker().Tick(0.0f);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsInstanced), TEXT("showing the kept tab A instanced elements, so it was bound afresh")));
		ASSERT_THAT(AreEqual(FString(TEXT("5")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ka-own")), TEXT("the change made while tab A was hidden did not appear after it was shown")));
		ASSERT_THAT(AreEqual(FString(TEXT("9")), ArcRmlUITestHelpers::GetText(Elements, TEXT("ka-shared"))));
	}

	TEST_METHOD(TabRule_NeverMatchesTheOtherKeptTabOrTheFrame)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		const FArcRmlTestLayoutElement* FrameTitle = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kf-title"));
		ASSERT_THAT(IsNotNull(FrameTitle));
		const FString FrameTitleBefore = FString::Printf(TEXT("%s %s %.1f"), *FrameTitle->BackgroundColor, *FrameTitle->Color, FrameTitle->Rect.Size.Y);
		ASSERT_THAT(IsNear(30.0f, FrameTitle->Rect.Size.Y, 0.5f));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Visit(TabB)));
		for (int32 Round = 0; Round < 2; ++Round)
		{
			const FArcRmlTestLayoutElement* BoxB = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kb-box"));
			ASSERT_THAT(IsNotNull(BoxB));
			ASSERT_THAT(IsNear(130.0f, BoxB->Rect.Size.X, 0.5f, TEXT("tab A's #kb-box rule changed the width of tab B's element")));
			ASSERT_THAT(IsNear(24.0f, BoxB->Rect.Size.Y, 0.5f, TEXT("the frame's .kt-box border reached tab B's element")));
			ASSERT_THAT(AreEqual(FString(TEXT("#00aa00ff")), BoxB->BackgroundColor, TEXT("a rule of tab A or the frame colours tab B's element")));
			const FArcRmlTestLayoutElement* TitleB = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kb-title"));
			ASSERT_THAT(IsNotNull(TitleB));
			ASSERT_THAT(IsNear(TitleB->Rect.Position.X, BoxB->Rect.Position.X, 0.5f, TEXT("the frame's #kb-box margin reached tab B's element")));

			ASSERT_THAT(IsTrue(Visit(TabA)));
			const FArcRmlTestLayoutElement* BoxA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-box"));
			ASSERT_THAT(IsNotNull(BoxA));
			ASSERT_THAT(IsNear(80.0f, BoxA->Rect.Size.X, 0.5f, TEXT("tab B's #ka-box rule changed the width of tab A's element")));
			ASSERT_THAT(IsNear(24.0f, BoxA->Rect.Size.Y, 0.5f, TEXT("the frame's .kt-box border reached tab A's element")));
			ASSERT_THAT(AreEqual(FString(TEXT("#0000aaff")), BoxA->BackgroundColor, TEXT("a rule of tab B or the frame colours tab A's element")));
			const FArcRmlTestLayoutElement* TitleA = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ka-title"));
			ASSERT_THAT(IsNotNull(TitleA));
			ASSERT_THAT(IsNear(TitleA->Rect.Position.X, BoxA->Rect.Position.X, 0.5f, TEXT("the frame's #ka-box margin reached tab A's element")));

			FrameTitle = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kf-title"));
			ASSERT_THAT(IsNotNull(FrameTitle));
			ASSERT_THAT(AreEqual(FrameTitleBefore, FString::Printf(TEXT("%s %s %.1f"), *FrameTitle->BackgroundColor, *FrameTitle->Color, FrameTitle->Rect.Size.Y),
				TEXT("a tab rule changed the frame's #kf-title")));
			ASSERT_THAT(IsTrue(Visit(TabB)));
		}
	}

	bool FocusFrameButtonAndPressDown()
	{
		View.ClickElement(TEXT("#kf-btn2"));
		return View.Flush() && View.Flush() && PressAndRead(EKeys::Gamepad_DPad_Down);
	}

	TEST_METHOD(FocusAreaLookup_NeverReturnsAnElementOfAHiddenTab)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		FLogCapture Log;
		ASSERT_THAT(IsTrue(OpenFrame(true)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(FocusFrameButtonAndPressDown()));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-only-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), FString::Printf(TEXT("the area a-only of the shown tab A was not entered, so the probe is not valid: nav focus '%s', area '%s'"), *ArcRmlUITestHelpers::GetNavFocusDomId(View), *View.GetProbe().ActiveArea)));
		ASSERT_THAT(AreEqual(FString(TEXT("a-only")), View.GetProbe().ActiveArea));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(AreEqual(FString(TEXT("kb-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(FocusFrameButtonAndPressDown()));
		ASSERT_THAT(AreNotEqual(FString(TEXT("ka-only-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View), TEXT("focus went into the area of the hidden tab A")));
		ASSERT_THAT(AreNotEqual(FString(TEXT("a-only")), View.GetProbe().ActiveArea, TEXT("the area of the hidden tab A became the active area")));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("the area of the hidden tab A is being entered")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("area 'a-only' does not exist")), Log.Describe()));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("area 'a-only' is hidden")), TEXT("the area of the hidden tab A was found")));

		ASSERT_THAT(IsTrue(Visit(CloseB)));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), FString::Printf(TEXT("a tab without areas waits to enter the primary area '%s' of the hidden tab"), *View.GetProbe().PendingFocusEnterArea)));
	}

	TEST_METHOD(FocusInsideATab_MovesOutWhenTheTabIsHidden)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		View.ClickElement(TEXT("#ka-field"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-field")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("the click did not focus the field")));

		ASSERT_THAT(IsTrue(Visit(CloseB)));
		ASSERT_THAT(AreNotEqual(FString(TEXT("ka-field")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("focus stayed on an element of the hidden tab A")));
		ASSERT_THAT(AreEqual(FString(TEXT("kf-embed")), ArcRmlUITestHelpers::GetFocusDomId(View), FString::Printf(TEXT("focus did not move to the arc-embed container: path '%s'"), *View.GetProbe().FocusPath)));
	}

	TEST_METHOD(PadCarryFromATab_EndsWhenTheTabIsHidden)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		FLogCapture Log;
		ASSERT_THAT(IsTrue(OpenFrame(true)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(AreEqual(FString(TEXT("ka-btn")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.UIAction(TEXT("IA_UISecondary"));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && Read()));
		ASSERT_THAT(IsTrue(HasClass(Elements, TEXT("ka-btn"), TEXT("drag-source")), TEXT("the pad pick up did not start a carry, so the probe is not valid")));
		ASSERT_THAT(AreEqual(1, FindFlatDraws(View.GetDraws(), ArcRmlKeptTabTestsPrivate::CloneColour).Num(), TEXT("the carry draws no clone")));

		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("ka-btn"), TEXT("drag-source")), TEXT("the carry from the hidden tab A is still marked")));
		ASSERT_THAT(AreEqual(0, FindFlatDraws(View.GetDraws(), ArcRmlKeptTabTestsPrivate::CloneColour).Num(), TEXT("the clone of the carry from the hidden tab A is still drawn")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("reason tab hidden")), Log.Describe()));

		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsFalse(HasClass(Elements, TEXT("ka-btn"), TEXT("drag-source")), TEXT("the carry came back with the tab")));
		ASSERT_THAT(AreEqual(0, FindFlatDraws(View.GetDraws(), ArcRmlKeptTabTestsPrivate::CloneColour).Num(), TEXT("the clone came back with the tab")));
	}

	TEST_METHOD(TwoKeptTabs_DrawAndRhiCommandsEqualTheShownTabAlone)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ArcRmlKeptTabTestsPrivate::FReplayResult Alone;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestsPrivate::Replay(View, Alone)));
		View.Close();

		ArcRmlKeptTabTestsPrivate::FReplayResult Kept;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(TabA)));
		ASSERT_THAT(IsTrue(Visit(TabB)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(ArcRmlKeptTabTestsPrivate::Replay(View, Kept)));

		ASSERT_THAT(IsTrue(Alone.DrawCalls > 0 && Alone.RhiCommands > 0, TEXT("the replay counted no draws, so the probe is not valid")));
		ASSERT_THAT(AreEqual(Alone.DrawCalls, Kept.DrawCalls, TEXT("a hidden kept tab adds draw calls")));
		ASSERT_THAT(AreEqual(Alone.RhiCommands, Kept.RhiCommands, TEXT("a hidden kept tab adds RHI commands")));
		ASSERT_THAT(AreEqual(0, ArcRmlKeptTabTestsPrivate::CountDifferentPixels(Alone, Kept), TEXT("a hidden kept tab changes pixels")));
	}

	TEST_METHOD(OnClose_RunsOncePerHideClearAndClose_AlsoWhenAnotherKeptTabKeepsTheModelBound)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		FLogCapture Log;
		const TStrongObjectPtr<UArcRmlKeptTabViewModel> Shared(NewObject<UArcRmlKeptTabViewModel>());
		const TFunction<UObject*(const FArcRmlViewModelDeclaration&)> FindModel = [&Shared](const FArcRmlViewModelDeclaration& Declaration) -> UObject*
		{
			return Declaration.Name == TEXT("shared") ? Shared.Get() : nullptr;
		};
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(SwapPreview(CloseA, FindModel)));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("call('', 'OnLeftA')")), Log.Describe()));

		ASSERT_THAT(IsTrue(SwapPreview(CloseB, FindModel)));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftA') on 'shared'")), TEXT("hiding tab A did not run its on-close once")));
		ASSERT_THAT(AreEqual(0, Log.CountContaining(TEXT("call('', 'OnLeftB')")), TEXT("tab B's on-close ran while it is shown")));

		ASSERT_THAT(IsTrue(SwapPreview(CloseA, FindModel)));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftB') on 'shared'")), TEXT("hiding tab B did not run its on-close once")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftA')")), TEXT("showing tab A ran its on-close again")));

		View.ClearEmbed();
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(2, Log.CountContaining(TEXT("call('', 'OnLeftA')")), TEXT("clearing the shown tab A did not run its on-close once")));
		ASSERT_THAT(AreEqual(1, Log.CountContaining(TEXT("call('', 'OnLeftB')")), TEXT("clearing ran the on-close of the hidden tab B")));

		ASSERT_THAT(IsTrue(SwapPreview(CloseA, FindModel)));
		ASSERT_THAT(IsTrue(SwapPreview(CloseB, FindModel)));
		ASSERT_THAT(AreEqual(3, Log.CountContaining(TEXT("call('', 'OnLeftA')")), Log.Describe()));
		View.Close();
		ASSERT_THAT(AreEqual(3, Log.CountContaining(TEXT("call('', 'OnLeftA')")), TEXT("closing the frame ran the on-close of the hidden tab A")));
		ASSERT_THAT(AreEqual(2, Log.CountContaining(TEXT("call('', 'OnLeftB')")), TEXT("closing the frame did not run the on-close of the shown tab B once")));
	}

	TEST_METHOD(TabDeclaringAKeptTabsModelDifferently_IsNotEmbeddedAndReportsTheHeadError)
	{
		using namespace ArcRmlKeptTabTestHelpers;
		ASSERT_THAT(IsTrue(OpenFrame(false)));
		ASSERT_THAT(IsTrue(Visit(CloseA)));
		const FString ConflictPath = ArcRmlUITestHelpers::MakePath(Conflict);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, ConflictPath + TEXT(": cannot open: view model 'shared' is declared differently by the kept tab"));
		ASSERT_THAT(IsFalse(View.Embed(ConflictPath), TEXT("a tab declaring the kept tab's model differently was embedded")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(ArcRmlUITestHelpers::MakePath(CloseA), View.GetProbe().EmbeddedPath, TEXT("the shown tab changed")));

		int32 HeadErrors = 0;
		for (const FArcRmlDiagnostic& Diagnostic : View.GetHeadDiagnostics())
		{
			const bool bMatches = Diagnostic.Kind == EArcRmlDiagnosticKind::Head && Diagnostic.Severity == EArcRmlDiagnosticSeverity::Error && Diagnostic.File == ConflictPath
				&& Diagnostic.Message.Contains(TEXT("view model 'shared' is declared differently by the kept tab")) && Diagnostic.Message.Contains(TEXT("'") + ArcRmlUITestHelpers::MakePath(CloseA) + TEXT("'"));
			HeadErrors += bMatches ? 1 : 0;
		}
		ASSERT_THAT(AreEqual(1, HeadErrors, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("kcc-text")), TEXT("the conflicting tab's elements are in the document")));
	}
};
