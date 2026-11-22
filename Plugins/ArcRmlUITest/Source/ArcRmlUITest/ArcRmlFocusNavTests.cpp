// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "InputCoreTypes.h"

namespace ArcRmlFocusNavTestsPrivate
{
	FVector2f GetCenter(const FArcRmlTestLayoutElement& Element)
	{
		return Element.Rect.Position + Element.Rect.Size * 0.5f;
	}

	const FArcRmlTestLayoutElement* FindList(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& ListName)
	{
		return Elements.FindByPredicate([&ListName](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Kind == TEXT("element") && (Element.DomId == ListName || Element.Classes == ListName);
		});
	}

	FString FindFirstVisibleRow(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& ListName, const FString& RowPrefix, int32 RowCount)
	{
		const FArcRmlTestLayoutElement* List = FindList(Elements, ListName);
		if (!List)
		{
			return FString();
		}
		const float ClipTop = List->Rect.Position.Y + 1.0f;
		const float ClipBottom = List->Rect.Position.Y + List->Rect.Size.Y - 1.0f;
		for (int32 Index = 0; Index < RowCount; ++Index)
		{
			const FString RowId = FString::Printf(TEXT("%s%d"), *RowPrefix, Index);
			const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, RowId);
			if (!Row)
			{
				continue;
			}
			const float Top = Row->Rect.Position.Y;
			const float Bottom = Top + Row->Rect.Size.Y;
			if (FMath::Min(Bottom, ClipBottom) - FMath::Max(Top, ClipTop) >= 1.0f)
			{
				return RowId;
			}
		}
		return FString();
	}

	float VisibleExtent(const FArcRmlInspectorRect& Box, TConstArrayView<FArcRmlInspectorRect> Clips)
	{
		float Extent = TNumericLimits<float>::Max();
		for (const FArcRmlInspectorRect& Clip : Clips)
		{
			const float Width = FMath::Min(Box.Position.X + Box.Size.X, Clip.Position.X + Clip.Size.X) - FMath::Max(Box.Position.X, Clip.Position.X);
			const float Height = FMath::Min(Box.Position.Y + Box.Size.Y, Clip.Position.Y + Clip.Size.Y) - FMath::Max(Box.Position.Y, Clip.Position.Y);
			Extent = FMath::Min(Extent, FMath::Min(Width, Height));
		}
		return Extent;
	}

	class FDroppedEntryWarningCapture final : public FOutputDevice
	{
	public:
		FDroppedEntryWarningCapture()
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FDroppedEntryWarningCapture() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != TEXT("LogArcRmlUI") || (Verbosity & ELogVerbosity::VerbosityMask) != ELogVerbosity::Warning)
			{
				return;
			}
			if (FCString::Strstr(Text, TEXT("entering it is dropped")) != nullptr)
			{
				FScopeLock Lock(&CountLock);
				++Count;
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		int32 GetCount()
		{
			GLog->Flush();
			FScopeLock Lock(&CountLock);
			return Count;
		}

	private:
		FCriticalSection CountLock;
		int32 Count = 0;
	};

	FString FindNearestRow(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& RowPrefix, int32 RowCount, FVector2f Center,
		TConstArrayView<FArcRmlInspectorRect> Clips, float MinExtent)
	{
		FString Best;
		float BestDistance = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index < RowCount; ++Index)
		{
			const FString RowId = FString::Printf(TEXT("%s%d"), *RowPrefix, Index);
			const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, RowId);
			if (!Row || VisibleExtent(Row->Rect, Clips) < MinExtent)
			{
				continue;
			}
			const float Distance = FVector2f::DistSquared(GetCenter(*Row), Center);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = RowId;
			}
		}
		return Best;
	}

	TArray<FArcRmlInspectorRect> GetClips(const TArray<FArcRmlTestLayoutElement>& Elements)
	{
		TArray<FArcRmlInspectorRect> Clips;
		for (const TCHAR* const Name : {TEXT("nrt-outer"), TEXT("nrt-list")})
		{
			if (const FArcRmlTestLayoutElement* Clip = FindList(Elements, Name))
			{
				Clips.Add(Clip->Rect);
			}
		}
		return Clips;
	}

	FString DescribeRows(const TArray<FArcRmlTestLayoutElement>& Elements, const FString& ListName, const FString& RowPrefix, int32 RowCount)
	{
		TArray<FString> Parts;
		const FArcRmlTestLayoutElement* List = FindList(Elements, ListName);
		Parts.Add(List ? FString::Printf(TEXT("%s y %.1f h %.1f"), *ListName, List->Rect.Position.Y, List->Rect.Size.Y) : FString::Printf(TEXT("%s missing"), *ListName));
		TArray<FString> Ids;
		for (int32 Index = 0; Index < RowCount; ++Index)
		{
			Ids.Add(FString::Printf(TEXT("%s%d"), *RowPrefix, Index));
		}
		for (const FString& Id : Ids)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
			Parts.Add(Element ? FString::Printf(TEXT("%s y %.1f h %.1f"), *Id, Element->Rect.Position.Y, Element->Rect.Size.Y) : FString::Printf(TEXT("%s missing"), *Id));
		}
		return FString::Join(Parts, TEXT(", "));
	}
}

TEST_CLASS(ArcRmlFocusNavTests, "ArcRmlUI.FocusNav")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndFlush(const FString& RelativePath)
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) && View.Flush() && View.GetProbe().bHasDocument;
	}

	bool Press(const FKey& Key)
	{
		View.PressKey(Key);
		return View.Flush();
	}

	bool OpenVariantAndFlush(const FString& RelativePath, const FString& Variant)
	{
		return View.Open(ArcRmlUITestHelpers::MakePath(RelativePath), ArcRmlUITestHelpers::ViewSize, 1.0f, Variant) && View.Flush() && View.GetProbe().bHasDocument;
	}

	bool PressUntilNavFocus(const FKey& Key, const FString& DomId, int32 MaxPresses)
	{
		for (int32 Step = 0; Step < MaxPresses && ArcRmlUITestHelpers::GetNavFocusDomId(View) != DomId; ++Step)
		{
			if (!Press(Key))
			{
				return false;
			}
		}
		return ArcRmlUITestHelpers::GetNavFocusDomId(View) == DomId;
	}

	bool TakeOverWithGamepad()
	{
		View.SetInputType(ECommonInputType::Gamepad);
		return View.Flush() && View.Flush();
	}

	FString DescribeCalls() const
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			Parts.Add(FString::Printf(TEXT("%s(%s)"), *Call.FunctionName, Call.Arguments.IsEmpty() ? TEXT("") : *Call.Arguments.Last().String));
		}
		return FString::Printf(TEXT("%d calls: %s; focus '%s', nav focus '%s'"), Parts.Num(), *FString::Join(Parts, TEXT(", ")), *ArcRmlUITestHelpers::GetFocusDomId(View),
			*ArcRmlUITestHelpers::GetNavFocusDomId(View));
	}

	bool ClickRowWithMouse(const FString& RowId)
	{
		if (!View.ParseLayout(Elements))
		{
			return false;
		}
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, RowId);
		if (!Row)
		{
			return false;
		}
		View.MouseClick(ArcRmlFocusNavTestsPrivate::GetCenter(*Row));
		return View.Flush();
	}

	TEST_METHOD(MouseClickThenPadTakeover_RunsEachFocusListenerOnce)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavRecoverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickRowWithMouse(TEXT("nrt-r0")), TEXT("mouse click on nrt-r0 failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-r0")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(IsFalse(View.GetProbe().bFocusVisible, DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusAny"), TEXT("nrt-r0")), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusVisible"), FString()), DescribeCalls()));

		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-r0")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(IsTrue(View.GetProbe().bFocusVisible, DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusAny"), TEXT("nrt-r0")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusVisible"), TEXT("nrt-r0")), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("BlurSeen"), FString()), DescribeCalls()));
		ASSERT_THAT(AreEqual(2, View.GetCalls().Num(), DescribeCalls()));
	}

	TEST_METHOD(PadMove_RunsEachFocusListenerOnce)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavRecoverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickRowWithMouse(TEXT("nrt-r0")), TEXT("mouse click on nrt-r0 failed")));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-r0")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeCalls()));
		View.ClearCalls();

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-r1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusAny"), TEXT("nrt-r1")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("FocusVisible"), TEXT("nrt-r1")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("BlurSeen"), TEXT("nrt-r0")), DescribeCalls()));
		ASSERT_THAT(AreEqual(3, View.GetCalls().Num(), DescribeCalls()));
	}

	TEST_METHOD(EscapeWalksParentAreasThenClosesWithLeave)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/AreaBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetInputBindings().Back.Keys.Contains(EKeys::Escape),
			TEXT("the back keys gathered from the document's input mapping do not contain Escape")));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("leaf")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-leaf-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("first escape flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("mid")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-mid-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("second escape flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("top")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-top-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("third escape flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests()));
		ASSERT_THAT(IsTrue(View.GetProbe().bLeaving, TEXT("the close did not start the leave transition")));
		ASSERT_THAT(IsFalse(View.GetProbe().bRemoved, TEXT("the view was removed before its leave transition played")));

		TestCommandBuilder
			.Until(TEXT("the leave transition removes the view"), [this]() -> bool
			{
				View.Flush();
				return View.GetProbe().bRemoved;
			}, FTimespan::FromSeconds(5))
			.Then(TEXT("the view is removed by its root transition end, not by the leave timeout"), [this]()
			{
				ASSERT_THAT(IsTrue(View.GetProbe().bRemoved));
				ASSERT_THAT(IsFalse(View.GetProbe().bLeaving));
				ASSERT_THAT(IsFalse(View.GetProbe().bLeaveTimedOut, TEXT("the leave finished by its timeout instead of the root transition end")));
				ASSERT_THAT(IsFalse(View.IsOpen()));
			});
	}

	TEST_METHOD(BodyBackHandler_GetsBackWithNothingFocused)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BodyBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("the fixture has nothing focusable")));

		View.Back();
		ASSERT_THAT(IsTrue(View.Flush()));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("BodyBack")), View.GetCalls()[0].FunctionName, DescribeCalls()));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("the body's data-event-back stopped the back, yet the view was asked to close")));
	}

	bool FocusAndBack(const FString& DomId)
	{
		View.ClickElement(TEXT("#") + DomId);
		if (!View.Flush() || ArcRmlUITestHelpers::GetFocusDomId(View) != DomId)
		{
			return false;
		}
		View.ClearCalls();
		View.Back();
		return View.Flush();
	}

	TEST_METHOD(BackInChildArea_ReturnsToParentAlthoughBodyHandlesBack)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("bot-child-1")), TEXT("focus on bot-child-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(0, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("bot-home-1")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("home")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
	}

	TEST_METHOD(BackInTopLevelArea_RunsTheBodyHandler)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("bot-home-1")), TEXT("focus on bot-home-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("BodyBack")), View.GetCalls()[0].FunctionName, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("bot-home-1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
	}

	TEST_METHOD(BackWithMissingEmptyOrSelfParent_RunsTheBodyHandler)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const TCHAR* DomId : {TEXT("bot-orphan-1"), TEXT("bot-lonely-1"), TEXT("bot-self-1")})
		{
			ASSERT_THAT(IsTrue(FocusAndBack(DomId), FString::Printf(TEXT("focus on %s or the back flush failed"), DomId)));
			ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
			ASSERT_THAT(AreEqual(FString(TEXT("BodyBack")), View.GetCalls()[0].FunctionName, DescribeCalls()));
			ASSERT_THAT(AreEqual(FString(DomId), ArcRmlUITestHelpers::GetFocusDomId(View)));
			ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
		}
	}

	TEST_METHOD(AreaHandlerStopsBack_NothingElseRuns)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("bot-outer-1")), TEXT("focus on bot-outer-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("OuterBack")), View.GetCalls()[0].FunctionName, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("bot-outer-1")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("the stopped back still returned to the parent area")));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
	}

	TEST_METHOD(NestedAreas_ChildReturnsAndParentlessChildBubblesToTheOuterArea)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("bot-inner-1")), TEXT("focus on bot-inner-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(0, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("bot-outer-1")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("outer")), View.GetProbe().ActiveArea));

		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("bot-loose-1")), TEXT("focus on bot-loose-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("OuterBack")), View.GetCalls()[0].FunctionName, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("bot-loose-1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests()));
	}

	TEST_METHOD(BackWithNoHandlerAndMissingParent_Closes)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/BackOrderCloseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusAndBack(TEXT("boc-orphan-1")), TEXT("focus on boc-orphan-1 or the back flush failed")));
		ASSERT_THAT(AreEqual(1, View.GetCloseRequests(), TEXT("an unhandled back did not close the view")));
	}

	// ScrollBorderTest: sb-box is a 200 x 100 overflow: hidden box with a 10 px border; sb-target (40 x 20, tab-index auto) sits
	// 500 px right and down in its 1000 x 1000 content. Tab focuses it and scrolls it into view with ScrollIntoView(Adaptive):
	// vertically Adaptive centres it between the start and end alignments, so its centre lands on the padding box's centre;
	// horizontally the default Nearest takes the end alignment, so its right edge lands on the padding box's right edge
	// (Element::ScrollIntoView counted the border twice, 20 px off on each axis; VENDORED 231 review fixes). The scroll stays
	// inside the scroll range on both axes, so no clamp hides an offset.
	TEST_METHOD(TabIntoBorderedScrollContainer_LandsTheTargetOnThePaddingBox)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/ScrollBorderTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Tab) && View.Flush(), TEXT("Tab flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("sb-target")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Box = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sb-box"));
		const FArcRmlTestLayoutElement* Target = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sb-target"));
		ASSERT_THAT(IsTrue(Box && Target));
		constexpr float Border = 10.0f;
		const FVector2f PaddingCentre = Box->Rect.Position + FVector2f(Border, Border) + (Box->Rect.Size - FVector2f(2.0f * Border, 2.0f * Border)) * 0.5f;
		const FVector2f TargetCentre = ArcRmlFocusNavTestsPrivate::GetCenter(*Target);
		const float PaddingRight = Box->Rect.Position.X + Box->Rect.Size.X - Border;
		const float TargetRight = Target->Rect.Position.X + Target->Rect.Size.X;
		const FVector2f MaxScroll(1000.0f - (Box->Rect.Size.X - 2.0f * Border), 1000.0f - (Box->Rect.Size.Y - 2.0f * Border));
		const bool bInsideRange = Box->Scroll.X > 1.0f && Box->Scroll.Y > 1.0f && Box->Scroll.X < MaxScroll.X - 1.0f && Box->Scroll.Y < MaxScroll.Y - 1.0f;
		ASSERT_THAT(IsTrue(bInsideRange && FMath::IsNearlyEqual(TargetCentre.Y, PaddingCentre.Y, 0.75f) && FMath::IsNearlyEqual(TargetRight, PaddingRight, 0.75f),
			*FString::Printf(TEXT("sb-target centre y %.1f right %.1f, sb-box padding centre y %.1f right %.1f, scroll (%.1f,%.1f) of (%.1f,%.1f)"), TargetCentre.Y,
				TargetRight, PaddingCentre.Y, PaddingRight, Box->Scroll.X, Box->Scroll.Y, MaxScroll.X, MaxScroll.Y)));
	}

	TEST_METHOD(NavCandidatesClippedByScrollContainer_AreSkipped)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavClipTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nc-left")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right), TEXT("pad right flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nc-r0")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		for (int32 Step = 0; Step < 9; ++Step)
		{
			ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		}
		ASSERT_THAT(AreEqual(FString(TEXT("nc-r9")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Left), TEXT("pad left flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nc-left")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FString Expected = ArcRmlFocusNavTestsPrivate::FindFirstVisibleRow(Elements, TEXT("nc-list"), TEXT("nc-r"), 10);
		ASSERT_THAT(IsFalse(Expected.IsEmpty(), FString::Printf(TEXT("no row of nc-list is visible: %s"), *ArcRmlFocusNavTestsPrivate::DescribeRows(Elements, TEXT("nc-list"), TEXT("nc-r"), 10))));
		ASSERT_THAT(AreNotEqual(FString(TEXT("nc-r0")), Expected, TEXT("the list must be scrolled so that nc-r0 is clipped")));

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right), TEXT("pad right flush failed")));
		ASSERT_THAT(AreEqual(Expected, ArcRmlUITestHelpers::GetNavFocusDomId(View)));
	}

	void CheckRecoveryToNearestVisibleRow(bool bExpectNearestBelowFirstVisible)
	{
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right), TEXT("pad right flush failed")));
		ASSERT_THAT(IsTrue(PressUntilNavFocus(EKeys::Gamepad_DPad_Down, TEXT("nrt-r9"), 12), FString::Printf(TEXT("pad down did not reach nrt-r9, nav focus '%s'"),
			*ArcRmlUITestHelpers::GetNavFocusDomId(View))));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Left), TEXT("pad left flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Removed = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("nrt-b"));
		ASSERT_THAT(IsNotNull(Removed));
		const FVector2f RemovedCenter = ArcRmlFocusNavTestsPrivate::GetCenter(*Removed);
		const TArray<FArcRmlInspectorRect> Clips = ArcRmlFocusNavTestsPrivate::GetClips(Elements);
		const FString Expected = ArcRmlFocusNavTestsPrivate::FindNearestRow(Elements, TEXT("nrt-r"), 10, RemovedCenter, Clips, 1.0f);
		const FString FirstVisible = ArcRmlFocusNavTestsPrivate::FindFirstVisibleRow(Elements, TEXT("nrt-list"), TEXT("nrt-r"), 10);
		const FString Rows = ArcRmlFocusNavTestsPrivate::DescribeRows(Elements, TEXT("nrt-list"), TEXT("nrt-r"), 10);
		ASSERT_THAT(IsFalse(Expected.IsEmpty(), FString::Printf(TEXT("no row of nrt-list is visible: %s"), *Rows)));
		ASSERT_THAT(AreNotEqual(FString(TEXT("nrt-r0")), FirstVisible, TEXT("the list must be scrolled so that nrt-r0 is clipped")));
		if (bExpectNearestBelowFirstVisible)
		{
			ASSERT_THAT(AreNotEqual(FirstVisible, Expected, FString::Printf(TEXT("the nearest visible row must differ from the first visible one: %s"), *Rows)));
		}
		const float ExpectedTop = ArcRmlUITestHelpers::FindByDomId(Elements, Expected)->Rect.Position.Y;

		View.SetModelValue(TEXT("nrt"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showB")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after removing nrt-b failed")));
		ASSERT_THAT(AreEqual(Expected, ArcRmlUITestHelpers::GetNavFocusDomId(View), Rows));

		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, Expected);
		ASSERT_THAT(IsNotNull(Row));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(ExpectedTop, Row->Rect.Position.Y, 0.5f),
			FString::Printf(TEXT("%s moved from y %.1f to %.1f, the list scroll changed"), *Expected, ExpectedTop, Row->Rect.Position.Y)));
	}

	TEST_METHOD(RemovedFocus_RecoversToNearestVisibleRowKeepingScroll)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavRecoverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		CheckRecoveryToNearestVisibleRow(false);
	}

	TEST_METHOD(RemovedFocusBesideLowerRows_RecoversToNearestNotFirstVisible)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavRecoverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetModelValue(TEXT("nrt"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("bTop")), ArcRmlUITestHelpers::MakeString(TEXT("180px")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after moving nrt-b failed")));
		CheckRecoveryToNearestVisibleRow(true);
	}

	TEST_METHOD(RowVisibleUnderOnePixelInNestedClip_IsNotChosenByMove)
	{
		ASSERT_THAT(IsTrue(OpenVariantAndFlush(TEXT("Focus/NavRecoverTest.rml"), TEXT("nestedPrep")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const TArray<FArcRmlInspectorRect> Clips = ArcRmlFocusNavTestsPrivate::GetClips(Elements);
		const FVector2f Center = ArcRmlFocusNavTestsPrivate::GetCenter(*ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("nrt-b")));
		const FString NearestAny = ArcRmlFocusNavTestsPrivate::FindNearestRow(Elements, TEXT("nrt-r"), 10, Center, Clips, UE_KINDA_SMALL_NUMBER);
		const FString Rows = ArcRmlFocusNavTestsPrivate::DescribeRows(Elements, TEXT("nrt-list"), TEXT("nrt-r"), 10);
		ASSERT_THAT(IsFalse(NearestAny.IsEmpty(), Rows));
		const float NearestExtent = ArcRmlFocusNavTestsPrivate::VisibleExtent(ArcRmlUITestHelpers::FindByDomId(Elements, NearestAny)->Rect, Clips);
		ASSERT_THAT(IsTrue(NearestExtent > 0.0f && NearestExtent <= 1.0f,
			FString::Printf(TEXT("the nearest row %s must show 1px or less under the 80.4px container, it shows %.2f: %s"), *NearestAny, NearestExtent, *Rows)));

		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right), TEXT("pad right flush failed")));
		const FString Target = ArcRmlUITestHelpers::GetNavFocusDomId(View);
		ASSERT_THAT(IsTrue(Target.StartsWith(TEXT("nrt-r")), FString::Printf(TEXT("pad right left the list, nav focus '%s'"), *Target)));
		ASSERT_THAT(AreNotEqual(NearestAny, Target, Rows));
		ASSERT_THAT(IsTrue(ArcRmlFocusNavTestsPrivate::VisibleExtent(ArcRmlUITestHelpers::FindByDomId(Elements, Target)->Rect, Clips) >= 1.0f, Rows));
	}

	TEST_METHOD(RowVisibleUnderOnePixelInNestedClip_IsNotChosenByRecovery)
	{
		ASSERT_THAT(IsTrue(OpenVariantAndFlush(TEXT("Focus/NavRecoverTest.rml"), TEXT("nestedPrep")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const TArray<FArcRmlInspectorRect> Clips = ArcRmlFocusNavTestsPrivate::GetClips(Elements);
		const FVector2f Center = ArcRmlFocusNavTestsPrivate::GetCenter(*ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("nrt-b")));
		const FString NearestAny = ArcRmlFocusNavTestsPrivate::FindNearestRow(Elements, TEXT("nrt-r"), 10, Center, Clips, UE_KINDA_SMALL_NUMBER);
		const FString Expected = ArcRmlFocusNavTestsPrivate::FindNearestRow(Elements, TEXT("nrt-r"), 10, Center, Clips, 1.0f);
		const FString Rows = ArcRmlFocusNavTestsPrivate::DescribeRows(Elements, TEXT("nrt-list"), TEXT("nrt-r"), 10);
		ASSERT_THAT(IsFalse(Expected.IsEmpty(), Rows));
		ASSERT_THAT(AreNotEqual(NearestAny, Expected, FString::Printf(TEXT("the nearest row must be one that shows 1px or less: %s"), *Rows)));

		View.SetModelValue(TEXT("nrt"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showB")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after removing nrt-b failed")));
		ASSERT_THAT(AreEqual(Expected, ArcRmlUITestHelpers::GetNavFocusDomId(View), Rows));
	}

	TEST_METHOD(AbsoluteButtonInNonPositionedScrollList_IsReachable)
	{
		ASSERT_THAT(IsTrue(OpenVariantAndFlush(TEXT("Focus/NavRecoverTest.rml"), TEXT("abs")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-abs")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
	}

	TEST_METHOD(TranslatedDialog_DownMovesBetweenItsButtons)
	{
		ASSERT_THAT(IsTrue(OpenVariantAndFlush(TEXT("Focus/NavRecoverTest.rml"), TEXT("dialog")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (int32 Step = 0; Step < 15 && ArcRmlUITestHelpers::GetFocusDomId(View) != TEXT("nrt-d1"); ++Step)
		{
			ASSERT_THAT(IsTrue(Press(EKeys::Tab), TEXT("tab flush failed")));
		}
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-d1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-d1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down), TEXT("pad down flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-d2")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
	}

	TEST_METHOD(ConfirmWhileEnteringAnArea_ActsOnTheEnteredElement)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/EnterConfirmTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("ect-bar-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		View.ClearCalls();

		View.PressKey(EKeys::Gamepad_DPad_Down);
		View.PressKey(EKeys::Enter);
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after down and confirm failed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Clicked"), TEXT("ect-list-1")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, View.GetCalls().Num(), DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("ect-list-1")), ArcRmlUITestHelpers::GetNavFocusDomId(View), DescribeCalls()));
	}

	TEST_METHOD(DevClickDuringKeyboardHold_EndsTheHoldLikeAMouseClick)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/AreaBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Down), TEXT("arrow flush failed")));
		ASSERT_THAT(IsTrue(View.GetProbe().bNavFocusHeld, TEXT("the arrow key did not start the navigation hold")));

		View.ClickElement(TEXT("#abt-leaf-2"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the dev click failed")));
		ASSERT_THAT(IsFalse(View.GetProbe().bNavFocusHeld, TEXT("ArcRml.Click kept the navigation hold a mouse click ends")));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-leaf-2")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
	}

	TEST_METHOD(DevClickDuringGamepadHold_SwitchesToMouseAndEndsTheHold)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/AreaBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(IsTrue(View.GetProbe().bNavFocusHeld, TEXT("the gamepad takeover did not start the navigation hold")));

		View.ClickElement(TEXT("#abt-leaf-2"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the dev click failed")));
		ASSERT_THAT(IsFalse(View.GetProbe().bNavFocusHeld, TEXT("ArcRml.Click under gamepad input kept the hold a real click ends")));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-leaf-2")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
	}

	TEST_METHOD(BackAfterMouseClickWithoutHold_ReturnsToParentArea)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/AreaBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(ClickRowWithMouse(TEXT("abt-leaf-1")), TEXT("mouse click on abt-leaf-1 failed")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-leaf-1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("leaf")), View.GetProbe().ActiveArea));
		ASSERT_THAT(IsFalse(View.GetProbe().bNavFocusHeld, TEXT("a mouse click must not hold navigation focus")));

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("escape flush failed")));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("back closed the view instead of returning to the parent area")));
		ASSERT_THAT(AreEqual(FString(TEXT("mid")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-mid-1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsFalse(View.GetProbe().bNavFocusHeld));
	}

	TEST_METHOD(BackInTheFrameOfAClickIntoAnArea_ReturnsToParentArea)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/AreaBackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().ActiveArea.IsEmpty(), TEXT("the document must start outside any area")));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("abt-leaf-1"));
		ASSERT_THAT(IsNotNull(Row));

		View.MouseClick(ArcRmlFocusNavTestsPrivate::GetCenter(*Row));
		View.PressKey(EKeys::Escape);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the click and the back failed")));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("back in the frame of the click closed the view from a stale snapshot")));
		ASSERT_THAT(AreEqual(FString(TEXT("mid")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("abt-mid-1")), ArcRmlUITestHelpers::GetFocusDomId(View)));
	}

	FVector2f CenterOf(const TCHAR* DomId)
	{
		if (!View.ParseLayout(Elements))
		{
			return FVector2f::ZeroVector;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		return Element ? ArcRmlFocusNavTestsPrivate::GetCenter(*Element) : FVector2f::ZeroVector;
	}

	bool OpenContainAndHoldOnSecondOption()
	{
		return OpenAndFlush(TEXT("Focus/ContainPointerTest.rml")) && View.Flush() && Press(EKeys::Down)
			&& ArcRmlUITestHelpers::GetFocusDomId(View) == TEXT("cpt-opt-1") && View.GetProbe().bNavFocusHeld;
	}

	// While focus is inside a data-focus-contain area (an open popup), the end of navigation focus keeps focus there, as on an open
	// select: a move over the area ends only the visible focus, and a click on an option focuses it and clicks with no blur on the area.
	TEST_METHOD(PointerMoveAndClickInsideContainArea_KeepFocusAndClick)
	{
		ASSERT_THAT(IsTrue(OpenContainAndHoldOnSecondOption(), DescribeCalls()));
		View.ClearCalls();

		View.MouseMove(CenterOf(TEXT("cpt-opt-0")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the mouse move failed")));
		ASSERT_THAT(IsFalse(View.GetProbe().bNavFocusHeld, TEXT("the mouse move did not end the navigation hold")));
		ASSERT_THAT(AreEqual(FString(TEXT("cpt-opt-1")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(IsFalse(View.GetProbe().bFocusVisible, DescribeCalls()));
		ASSERT_THAT(IsTrue(View.GetSnapshot().bFocusInContainArea, DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), FString()), DescribeCalls()));

		View.MouseClick(CenterOf(TEXT("cpt-opt-0")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("cpt-opt-0")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Picked"), TEXT("0")), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), FString()), DescribeCalls()));
	}

	// cpt-panel is no pointer region, so its presses go to the game, except while focus is inside a data-focus-contain area: then the
	// press reaches the document, focus leaves the area and the area gets its blur, also after the pointer moved there first.
	TEST_METHOD(PressOutsideContainArea_BlursTheArea)
	{
		ASSERT_THAT(IsTrue(OpenContainAndHoldOnSecondOption(), DescribeCalls()));
		const FVector2f Panel = CenterOf(TEXT("cpt-panel"));
		ASSERT_THAT(IsFalse(View.IsInteractiveAt(Panel), TEXT("cpt-panel is a pointer region")));
		View.ClearCalls();

		View.MouseMove(Panel);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the mouse move failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("cpt-opt-1")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), FString()), DescribeCalls()));

		View.MouseClick(Panel);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the press failed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), TEXT("out")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), FString()), DescribeCalls()));
		ASSERT_THAT(IsFalse(View.GetSnapshot().bFocusInContainArea, DescribeCalls()));
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(View);
		ASSERT_THAT(IsFalse(Focus == TEXT("cpt-opt-0") || Focus == TEXT("cpt-opt-1"), DescribeCalls()));
	}

	// cpt-owner (data-focus-owner) holds the focusable cpt-trigger beside the area: a press on the trigger moves focus within the owner,
	// so the area's blur carries ev.related_in_owner true (Chrome: owner.contains(relatedTarget)); a press on cpt-panel carries false.
	TEST_METHOD(PressOnTriggerInTheOwner_BlurCarriesRelatedInOwner)
	{
		ASSERT_THAT(IsTrue(OpenContainAndHoldOnSecondOption(), DescribeCalls()));
		View.ClearCalls();

		View.MouseClick(CenterOf(TEXT("cpt-trigger")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the press failed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), TEXT("in")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("PopBlur"), FString()), DescribeCalls()));
	}

	// cpt-bare has no data-focus-owner: a press on its sibling trigger blurs the area with ev.related_in_owner false.
	TEST_METHOD(PressOnTriggerWithNoOwner_BlurCarriesFalse)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/ContainPointerTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.MouseClick(CenterOf(TEXT("cpt-bare-opt")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the first press failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("cpt-bare-opt")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		View.ClearCalls();

		View.MouseClick(CenterOf(TEXT("cpt-bare-trigger")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the trigger press failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("cpt-bare-trigger")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("BareBlur"), TEXT("out")), DescribeCalls()));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("BareBlur"), FString()), DescribeCalls()));
	}

	TEST_METHOD(RemovedFocusWithEveryRowClipped_FallsBackToEntry)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/NavRecoverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(TakeOverWithGamepad(), TEXT("gamepad takeover flush failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-b")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));

		View.SetModelValue(TEXT("nrt"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showB")), ArcRmlUITestHelpers::MakeBool(false));
		View.SetModelValue(TEXT("nrt"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("listH")), ArcRmlUITestHelpers::MakeString(TEXT("0px")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after removing nrt-b failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("nrt-r0")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
	}

	// feo-label is no focus candidate, and feo-near's centre is nearer to it than feo-trigger's: Back must return to the trigger, not the nearest row.
	TEST_METHOD(ClickOnNonFocusableChildOfTrigger_BackReturnsToTrigger)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/FocusEnterOriginTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.MouseClick(CenterOf(TEXT("feo-label")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(AreEqual(FString(TEXT("pop")), View.GetProbe().ActiveArea, DescribeCalls()));
		ASSERT_THAT(AreEqual(FString(TEXT("feo-pop-1")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));

		ASSERT_THAT(IsTrue(Press(EKeys::Escape), TEXT("escape flush failed")));
		ASSERT_THAT(AreEqual(0, View.GetCloseRequests(), TEXT("back closed the view instead of returning to the parent area")));
		ASSERT_THAT(AreEqual(FString(TEXT("rows")), View.GetProbe().ActiveArea));
		ASSERT_THAT(AreEqual(FString(TEXT("feo-trigger")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
	}

	// The trigger's call removes the area it enters (a dropdown box that closes its list): the entry is dropped with no warning.
	TEST_METHOD(TriggerClickWhoseCallRemovesTheArea_LogsNoWarning)
	{
		ASSERT_THAT(IsTrue(OpenAndFlush(TEXT("Focus/FocusEnterOriginTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Trigger = CenterOf(TEXT("feo-close"));
		ArcRmlFocusNavTestsPrivate::FDroppedEntryWarningCapture Warnings;
		View.MouseClick(Trigger);
		View.SetModelValue(TEXT("feo"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("open")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.Flush(), TEXT("flush after the click failed")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Toggle"), FString()), DescribeCalls()));
		ASSERT_THAT(IsTrue(View.GetProbe().PendingFocusEnterArea.IsEmpty(), TEXT("entering the removed area is still pending")));
		ASSERT_THAT(AreEqual(FString(TEXT("feo-close")), ArcRmlUITestHelpers::GetFocusDomId(View), DescribeCalls()));
		ASSERT_THAT(AreEqual(0, Warnings.GetCount(), TEXT("a click whose call removed its area logged the dropped-entry warning")));
	}
};
