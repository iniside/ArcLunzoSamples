// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlInputGateTestsPrivate
{
	const TCHAR* const None = TEXT("");

	FString Expect(bool bGate, const TCHAR* ClickedId)
	{
		return FString::Printf(TEXT("gate=%d click='%s'"), bGate ? 1 : 0, ClickedId);
	}

	FArcRmlInputEvent MakePointer(EArcRmlInputType Type, FVector2f Point)
	{
		FArcRmlInputEvent Event;
		Event.Type = Type;
		Event.Position = ArcRmlPointer::ToRmlPoint(Point);
		Event.Button = 0;
		return Event;
	}
}

TEST_CLASS(ArcRmlInputGateTests, "ArcRmlUI.InputGate")
{
	FArcRmlTestView View;

	BEFORE_EACH()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Input/TransformedGateTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.Flush() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	FString Observe(FVector2f Point)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		const bool bGate = View.IsInteractiveAt(Point);
		View.ClearCalls();
		View.RawInput(MakePointer(EArcRmlInputType::MouseMove, Point));
		View.RawInput(MakePointer(EArcRmlInputType::MouseDown, Point));
		View.RawInput(MakePointer(EArcRmlInputType::MouseUp, Point));
		if (!View.Flush())
		{
			return TEXT("flush failed");
		}
		TArray<FString> Ids;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if (Call.FunctionName == TEXT("Clicked") && !Call.Arguments.IsEmpty())
			{
				Ids.Add(Call.Arguments.Last().String);
			}
		}
		return Expect(bGate, *FString::Join(Ids, TEXT(",")));
	}

	bool Matches(const FString& Expected, FVector2f Point, const TCHAR* Reason)
	{
		const FString Actual = Observe(Point);
		return Assert.AreEqual(Expected, Actual, FString::Printf(TEXT("at (%g,%g) got '%s': %s"), Point.X, Point.Y, *Actual, Reason));
	}

	TEST_METHOD(TranslatedButton_HitsWhereDrawnNotWhereLaidOut)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-translate")), Observe(FVector2f(290.0f, 70.0f)), TEXT("drawn x 40+200..140+200 = 240..340, y 40..100: (290,70) is inside")));
		if (!Matches(Expect(false, None), FVector2f(90.0f, 70.0f), TEXT("layout box x 40..140: (90,70) is inside it and outside the drawn 240..340")))
		{
			return;
		}
	}

	TEST_METHOD(RotatedSquare_HitsDiamondNotUnrotatedCorner)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-rotate")), Observe(FVector2f(80.0f, 240.0f)), TEXT("centre (40+40,200+40)")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-rotate")), Observe(FVector2f(130.0f, 240.0f)), TEXT("|dx|+|dy| = 50 < half diagonal 40*sqrt2 = 56.57 near the right corner")));
		if (!Matches(Expect(false, None), FVector2f(44.0f, 204.0f), TEXT("unrotated corner: |dx|+|dy| = 36+36 = 72 > 56.57, inside the old box 40..120 x 200..280")))
		{
			return;
		}
	}

	TEST_METHOD(ScaledAncestor_HitsScaledButton)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-scale")), Observe(FVector2f(500.0f, 200.0f)), TEXT("scale 2 about (550,250): layout 510..540 x 210..240 draws 470..530 x 170..230; (500,200) is its centre")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-scale")), Observe(FVector2f(475.0f, 175.0f)), TEXT("inside drawn 470..530 x 170..230, outside layout 510..540 x 210..240")));
		if (!Matches(Expect(false, None), FVector2f(535.0f, 235.0f), TEXT("inside layout 510..540 x 210..240, outside drawn 470..530 x 170..230")))
		{
			return;
		}
	}

	TEST_METHOD(RotatedClipPanelWithOverflowingButton_HitsOnlyInsidePanel)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-clip-btn")), Observe(FVector2f(868.0f, 112.0f)), TEXT("centre (840,140), offset (28,-28): u=(dx+dy)/sqrt2=0, v=(dy-dx)/sqrt2=-39.6, inside panel |u|,|v|<=80 and button v in -80..0")));
		if (!Matches(Expect(false, None), FVector2f(953.0f, 197.0f), TEXT("offset (113,57): u=120.2 > panel half 80, v=-39.6 is on the button: clipped by the rotated panel")))
		{
			return;
		}
		if (!Matches(Expect(false, None), FVector2f(780.0f, 100.0f), TEXT("inside the layout box 760..1000 x 60..140; offset (-60,-40): v=+14 > 0 is below the button")))
		{
			return;
		}
		if (!Matches(Expect(false, None), FVector2f(910.0f, 190.0f), TEXT("offset (70,50): u=(dx+dy)/sqrt2=84.9 > panel half 80 while v=(dy-dx)/sqrt2=-14.1 is on the button (-80..0): inside the unrotated panel box 760..920 x 60..220, outside the rotated panel")))
		{
			return;
		}
	}

	TEST_METHOD(RotatedFittingPanelWithTranslatedChild_HitsUnclipped)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-fit-btn")), Observe(FVector2f(261.0f, 525.0f)), TEXT("centre (120,440), child local centre (-40,-40)+(200,0)=(160,-40): window offset ((u-v)/sqrt2,(u+v)/sqrt2)=(141.4,84.9), outside the panel half 80")));
		if (!Matches(Expect(false, None), FVector2f(80.0f, 400.0f), TEXT("layout box of the child is 60..100 x 380..420; (80,400) is inside it and not drawn")))
		{
			return;
		}
	}

	TEST_METHOD(RoundedFittingPanelWithTranslatedChild_NeitherHits)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		if (!Matches(Expect(false, None), FVector2f(740.0f, 440.0f), TEXT("child layout 420..460 + 300 = 720..760 x 420..460 lies outside the panel 400..560; the border-radius ancestor masks it")))
		{
			return;
		}
		if (!Matches(Expect(false, None), FVector2f(440.0f, 440.0f), TEXT("old layout box of the child 420..460 x 420..460")))
		{
			return;
		}
	}

	TEST_METHOD(ScrollListRowTranslatedUp_HitsWhereDrawn)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-list-row")), Observe(FVector2f(880.0f, 505.0f)), TEXT("list 380..530; row 3 lays out at 380+150 = 530..580, translateY(-50) draws it at 480..530")));
		if (!Matches(Expect(false, None), FVector2f(880.0f, 555.0f), TEXT("row layout box 530..580, below the list, not drawn there")))
		{
			return;
		}
	}

	TEST_METHOD(InlineLinkWrappedOntoTwoLines_BothLinesHit)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-link")), Observe(FVector2f(1155.0f, 60.0f)), TEXT("container top 40, line-height 40: line one centre y 60, x 1150+5 inside the first glyph")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-link")), Observe(FVector2f(1155.0f, 100.0f)), TEXT("line two centre y 100: two 8-letter words cannot share a 40px container")));
		if (!Matches(Expect(false, None), FVector2f(1155.0f, 140.0f), TEXT("below line two (80..120)")))
		{
			return;
		}
	}

	TEST_METHOD(PerspectiveRotateY30_NearEdgeWidensAsDrawn)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-persp")), Observe(FVector2f(1140.0f, 580.0f)), TEXT("centre (1060+80,480+100)")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-persp")), Observe(FVector2f(1070.0f, 477.0f)), TEXT("left edge x=-80 gets z=+40, scale 600/560 = 1.0714: half height 107.1 so top 472.9 < layout top 480; at x offset -70 top is about 473.3")));
		if (!Matches(Expect(false, None), FVector2f(1215.0f, 580.0f), TEXT("right edge x=+80 gets z=-40, scale 600/640: drawn right edge 1140+64.95 = 1204.95 < 1215 inside layout right 1220")))
		{
			return;
		}
	}

	TEST_METHOD(PerspectiveRotateY89_EdgeOnNeitherHits)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		if (!Matches(Expect(false, None), FVector2f(1150.0f, 330.0f), TEXT("centre of the edge-on button: depth ray 20*cos(89deg) = 0.35 <= 1")))
		{
			return;
		}
		if (!Matches(Expect(false, None), FVector2f(1130.0f, 330.0f), TEXT("inside the layout box 1100..1200")))
		{
			return;
		}
	}

	TEST_METHOD(UntransformedButton_Unchanged)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-plain")), Observe(FVector2f(610.0f, 70.0f)), TEXT("centre of 560..660 x 40..100")));
		if (!Matches(Expect(false, None), FVector2f(550.0f, 70.0f), TEXT("left of 560")))
		{
			return;
		}
	}

	TEST_METHOD(MirroredButton_HitsWhereDrawn)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-mirror")), Observe(FVector2f(600.0f, 630.0f)), TEXT("translateX(150) scaleX(-1) about the centre: layout 400..500 x 600..660 draws mirrored at 550..650")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-mirror")), Observe(FVector2f(555.0f, 605.0f)), TEXT("near the drawn left corner")));
		if (!Matches(Expect(false, None), FVector2f(450.0f, 630.0f), TEXT("inside the layout box 400..500, outside the drawn 550..650")))
		{
			return;
		}
	}

	TEST_METHOD(FractionalEdge_GateAndRmlUiAgreeAtRoundedPoint)
	{
		using namespace ArcRmlInputGateTestsPrivate;
		if (!Matches(Expect(false, None), FVector2f(400.4f, 70.0f), TEXT("left edge 400.25: 400.4 rounds to 400 < 400.25")))
		{
			return;
		}
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-frac")), Observe(FVector2f(400.6f, 70.0f)), TEXT("400.6 rounds to 401 >= 400.25")));
		ASSERT_THAT(AreEqual(Expect(true, TEXT("tg-frac")), Observe(FVector2f(500.4f, 70.0f)), TEXT("right edge 500.25: 500.4 rounds to 500 <= 500.25")));
		if (!Matches(Expect(false, None), FVector2f(500.6f, 70.0f), TEXT("500.6 rounds to 501 > 500.25")))
		{
			return;
		}
	}

	TEST_METHOD(InspectorPick_ReturnsTranslatedButtonAtDrawnPoint)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Button = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tg-translate"));
		ASSERT_THAT(IsNotNull(Button));
		View.SetInspectorActive(true);
		View.ClearInspectorAnswers();
		View.InspectorAt(FVector2f(290.0f, 70.0f), true);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorAnswer* Drawn = View.GetInspectorAnswers().FindByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Request == EArcRmlInspectorRequest::InspectAt;
		});
		ASSERT_THAT(IsNotNull(Drawn));
		ASSERT_THAT(AreEqual(Button->Path, Drawn->Path, TEXT("(290,70) is inside the drawn 240..340")));

		View.ClearInspectorAnswers();
		View.InspectorAt(FVector2f(90.0f, 70.0f), true);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorAnswer* Laid = View.GetInspectorAnswers().FindByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Request == EArcRmlInspectorRequest::InspectAt;
		});
		ASSERT_THAT(IsNotNull(Laid));
		ASSERT_THAT(IsTrue(Laid->Path != Button->Path, TEXT("(90,70) is in the layout box 40..140 only")));

		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("tg-list-row"));
		ASSERT_THAT(IsNotNull(Row));
		View.ClearInspectorAnswers();
		View.InspectorAt(FVector2f(880.0f, 505.0f), true);
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorAnswer* Scrolled = View.GetInspectorAnswers().FindByPredicate([](const FArcRmlInspectorAnswer& Answer) -> bool
		{
			return Answer.Request == EArcRmlInspectorRequest::InspectAt;
		});
		ASSERT_THAT(IsNotNull(Scrolled));
		ASSERT_THAT(AreEqual(Row->Path, Scrolled->Path, TEXT("list 380..530; the row lays out at 530..580 and translateY(-50) draws it at 480..530, so (880,505) is on the drawn row inside the list window")));
	}

	TEST_METHOD(DevClickOnTranslatedButton_ReachesIt)
	{
		View.ClearCalls();
		View.ClickElement(TEXT("#tg-translate"));
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Clicked"), TEXT("tg-translate")), TEXT("the dev click must land on the drawn button at x 240..340")));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num()));
	}
};

namespace ArcRmlHitRegionDirtyTestsPrivate
{
	const FVector2f ActionPoint(80.0f, 70.0f);
	const FVector2f EventPoint(200.0f, 70.0f);
	const FVector2f PointerEventsPoint(320.0f, 70.0f);
	const FVector2f VisibilityPoint(440.0f, 70.0f);
	const FVector2f DisplayPoint(560.0f, 70.0f);
	const FVector2f DomPoint(680.0f, 70.0f);
	const FVector2f TransformLaidPoint(800.0f, 70.0f);
	const FVector2f TransformShiftedPoint(1000.0f, 70.0f);
	const FVector2f ScrollPoint(140.0f, 260.0f);
	constexpr int64 TransformSubtreeElements = 4;
	constexpr int64 ScrollSubtreeElements = 14;
	const FVector2f ShrinkPoint(360.0f, 250.0f);
	const FVector2f BoxWidePoint(580.0f, 230.0f);
	const FVector2f FlowBeforePoint(710.0f, 240.0f);
	const FVector2f FlowAfterPoint(710.0f, 300.0f);
	const FVector2f ClipOverflowPoint(920.0f, 220.0f);
	const FVector2f InheritedPointerEventsPoint(1020.0f, 230.0f);
	const FVector2f OverflowHiddenPoint(360.0f, 360.0f);
}

TEST_CLASS(ArcRmlHitRegionDirtyTests, "ArcRmlUI.InputGate")
{
	FArcRmlTestView View;

	BEFORE_EACH()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Input/HitRegionDirty.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	int64 Count(Rml::ArcPerf::Counter Counter) const
	{
		return View.GetPerfCounter(Counter);
	}

	FString DescribeCounts() const
	{
		return FString::Printf(TEXT("hit region elements walked %lld, hit region lists rebuilt %lld, elements updated %lld, boxes changed %lld, elements rendered %lld"),
			Count(Rml::ArcPerf::Counter::HitRegionElementsWalked), Count(Rml::ArcPerf::Counter::HitRegionListsRebuilt), Count(Rml::ArcPerf::Counter::ElementsUpdated),
			Count(Rml::ArcPerf::Counter::BoxesChanged), Count(Rml::ArcPerf::Counter::ElementsRendered));
	}

	bool WaitForFrameAfter(uint64 FrameBefore)
	{
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= FrameBefore; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > FrameBefore;
	}

	bool MatchesRebuild(const TCHAR* When)
	{
		const FString Mismatch = ArcRmlUITestHelpers::DescribeHitRegionMismatch(View);
		return Assert.IsTrue(Mismatch.IsEmpty(), FString::Printf(TEXT("%s: %s"), When, *Mismatch));
	}

	bool Gate(FVector2f Point, bool bExpected, const TCHAR* When)
	{
		return Assert.AreEqual(bExpected, View.IsInteractiveAt(Point), FString::Printf(TEXT("gate at (%g,%g) %s"), Point.X, Point.Y, When));
	}

	bool ToggleAndCheck(const TCHAR* Field, bool bValue, FVector2f Point, bool bExpected, const TCHAR* When)
	{
		View.SetModelValue(TEXT("hr"), ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		return Assert.IsTrue(View.FlushUntilSettled(), When) && Gate(Point, bExpected, When) && MatchesRebuild(When);
	}

	TEST_METHOD(PublishWithoutChange_WalksNoElementAndReusesTheLists)
	{
		if (!MatchesRebuild(TEXT("after open")))
		{
			return;
		}
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("noop"), true);
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the root class change rendered no frame")));
		TestRunner->AddInfo(FString::Printf(TEXT("%llu frames; %s"), View.GetProbe().FrameSerial - FrameBefore, *DescribeCounts()));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::HitRegionElementsWalked), DescribeCounts()));
		ASSERT_THAT(AreEqual(int64(0), Count(Rml::ArcPerf::Counter::HitRegionListsRebuilt), DescribeCounts()));
		if (!MatchesRebuild(TEXT("after the idle frame")))
		{
			return;
		}
	}

	TEST_METHOD(OneElementBecomesInteractive_WalksOnlyThatElement)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(ActionPoint, false, TEXT("with an empty data-action")))
		{
			return;
		}
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetModelValue(TEXT("hr"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Action")), ArcRmlUITestHelpers::MakeString(TEXT("go")));
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the data-action change rendered no frame")));
		TestRunner->AddInfo(FString::Printf(TEXT("%llu frames; %s"), View.GetProbe().FrameSerial - FrameBefore, *DescribeCounts()));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::HitRegionElementsWalked) <= 1, FString::Printf(TEXT("only #hr-action changed; %s"), *DescribeCounts())));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::HitRegionListsRebuilt) >= 1, FString::Printf(TEXT("the region set changed; %s"), *DescribeCounts())));
		if (!Gate(ActionPoint, true, TEXT("with data-action 'go'")) || !MatchesRebuild(TEXT("after data-action 'go'")))
		{
			return;
		}
		View.SetModelValue(TEXT("hr"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Action")), ArcRmlUITestHelpers::MakeString(FString()));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		if (!Gate(ActionPoint, false, TEXT("with data-action emptied again")) || !MatchesRebuild(TEXT("after data-action emptied")))
		{
			return;
		}
	}

	TEST_METHOD(EventAttributeAddedAndRemoved_GateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(EventPoint, false, TEXT("without data-event-ping")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Armed"), true, EventPoint, true, TEXT("after data-event-ping was added")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Armed"), false, EventPoint, false, TEXT("after data-event-ping was removed")))
		{
			return;
		}
	}

	TEST_METHOD(PointerEventsToggled_GateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(PointerEventsPoint, true, TEXT("with pointer-events auto")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("PeNone"), true, PointerEventsPoint, false, TEXT("after pointer-events none")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("PeNone"), false, PointerEventsPoint, true, TEXT("after pointer-events auto again")))
		{
			return;
		}
	}

	TEST_METHOD(VisibilityToggled_GateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(VisibilityPoint, true, TEXT("visible")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Hidden"), true, VisibilityPoint, false, TEXT("after visibility hidden")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Hidden"), false, VisibilityPoint, true, TEXT("after visibility visible again")))
		{
			return;
		}
	}

	TEST_METHOD(AncestorDisplayToggled_ChildGateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(DisplayPoint, true, TEXT("displayed")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Gone"), true, DisplayPoint, false, TEXT("after the parent got display none")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Gone"), false, DisplayPoint, true, TEXT("after the parent got display block again")))
		{
			return;
		}
	}

	TEST_METHOD(DomRemoveAndInsert_GateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(DomPoint, true, TEXT("inserted")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Show"), false, DomPoint, false, TEXT("after data-if removed it")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Show"), true, DomPoint, true, TEXT("after data-if inserted it again")))
		{
			return;
		}
	}

	TEST_METHOD(AncestorTransformChanged_ChildRegionMovesAndOnlyTheSubtreeIsWalked)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(TransformLaidPoint, true, TEXT("untransformed")) || !Gate(TransformShiftedPoint, false, TEXT("untransformed")))
		{
			return;
		}
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.SetModelValue(TEXT("hr"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Shift")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(WaitForFrameAfter(FrameBefore), TEXT("the transform change rendered no frame")));
		TestRunner->AddInfo(FString::Printf(TEXT("%llu frames; %s"), View.GetProbe().FrameSerial - FrameBefore, *DescribeCounts()));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::HitRegionElementsWalked) <= TransformSubtreeElements,
			FString::Printf(TEXT("only #hr-xf, its child and their 2 whitespace texts changed; %s"), *DescribeCounts())));
		if (!Gate(TransformLaidPoint, false, TEXT("translated 200 px")) || !Gate(TransformShiftedPoint, true, TEXT("translated 200 px"))
			|| !MatchesRebuild(TEXT("after the ancestor transform")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Shift"), false, TransformLaidPoint, true, TEXT("after the transform was removed")))
		{
			return;
		}
	}

	TEST_METHOD(AncestorScrolled_ChildRegionsMoveAndOnlyTheScrollerIsWalked)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		const uint64 FrameBefore = View.GetProbe().FrameSerial;
		View.MouseMove(ScrollPoint);
		View.MouseWheel(ScrollPoint, -1.0f);
		float ScrollY = 0.0f;
		float Previous = -1.0f;
		int32 Stable = 0;
		for (int32 Attempt = 0; Attempt < 200 && Stable < 5; ++Attempt)
		{
			FPlatformProcess::Sleep(0.02f);
			TArray<FArcRmlTestLayoutElement> Elements;
			ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
			const FArcRmlTestLayoutElement* Scroller = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("hr-scroll"));
			ASSERT_THAT(IsNotNull(Scroller));
			ScrollY = Scroller->Scroll.Y;
			Stable = (ScrollY > 0.0f && FMath::IsNearlyEqual(ScrollY, Previous)) ? Stable + 1 : 0;
			Previous = ScrollY;
		}
		ASSERT_THAT(IsTrue(ScrollY > 0.0f, TEXT("the wheel did not scroll #hr-scroll")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		const int64 Frames = int64(View.GetProbe().FrameSerial - FrameBefore);
		TestRunner->AddInfo(FString::Printf(TEXT("scrolled to %g in %lld frames; %s"), ScrollY, Frames, *DescribeCounts()));
		ASSERT_THAT(IsTrue(Count(Rml::ArcPerf::Counter::HitRegionElementsWalked) <= ScrollSubtreeElements * Count(Rml::ArcPerf::Counter::HitRegionListsRebuilt),
			FString::Printf(TEXT("only #hr-scroll, its 6 rows and its 7 whitespace texts moved; %s"), *DescribeCounts())));
		if (!MatchesRebuild(TEXT("after the scroll")))
		{
			return;
		}
	}

	TEST_METHOD(ScrollerContentShrinks_ContainerLeavesTheGate)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(ShrinkPoint, true, TEXT("while the content overflows")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Tall"), false, ShrinkPoint, false, TEXT("after the content shrank and the scrollbar hid")))
		{
			return;
		}
	}

	TEST_METHOD(ScrollerOverflowSetToHidden_ContainerLeavesTheGate)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(OverflowHiddenPoint, true, TEXT("overflow-y auto with overflowing content")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Flat"), true, OverflowHiddenPoint, false, TEXT("after overflow-y hidden hid the scrollbar")))
		{
			return;
		}
	}

	TEST_METHOD(BoxSizeChanged_GateFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(BoxWidePoint, false, TEXT("80 px wide")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Wide"), true, BoxWidePoint, true, TEXT("after the box grew to 160 px")))
		{
			return;
		}
	}

	TEST_METHOD(SiblingGrows_FollowingElementRegionMoves)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(FlowBeforePoint, true, TEXT("button at 220..260")) || !Gate(FlowAfterPoint, false, TEXT("button at 220..260")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Grow"), true, FlowAfterPoint, true, TEXT("after the sibling grew, button at 280..320")) ||
			!Gate(FlowBeforePoint, false, TEXT("after the sibling grew, button at 280..320")))
		{
			return;
		}
	}

	TEST_METHOD(ClippingAncestorOverflowChanged_ChildRegionFollows)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(ClipOverflowPoint, false, TEXT("clipped by overflow hidden")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("Open"), true, ClipOverflowPoint, true, TEXT("after the ancestor got overflow visible")))
		{
			return;
		}
	}

	TEST_METHOD(AncestorClassSetsInheritedPointerEvents_ChildLeavesTheGate)
	{
		using namespace ArcRmlHitRegionDirtyTestsPrivate;
		if (!Gate(InheritedPointerEventsPoint, true, TEXT("pointer-events auto")))
		{
			return;
		}
		if (!ToggleAndCheck(TEXT("PeOff"), true, InheritedPointerEventsPoint, false, TEXT("after the parent class set pointer-events none")))
		{
			return;
		}
	}

	TEST_METHOD(KeyedRowMoved_RegionsMatchAFullRebuild)
	{
		if (!MatchesRebuild(TEXT("before the move")))
		{
			return;
		}
		TArray<FArcRmlListOp> Ops;
		FArcRmlListOp& Move = Ops.AddDefaulted_GetRef();
		Move.Type = EArcRmlListOpType::Move;
		Move.Index = 0;
		Move.ToIndex = 2;
		View.ApplyListOps(TEXT("hr"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Items")), MoveTemp(Ops));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		if (!MatchesRebuild(TEXT("after the first row moved to the end")))
		{
			return;
		}
	}
};
