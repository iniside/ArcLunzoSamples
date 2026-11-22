// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlRangeTestsPrivate
{
	const TCHAR* const EventsFixture = TEXT("Form/RangeEventsTest.rml");
	const TCHAR* const EventsSheet = TEXT("RangeEvents.rcssc");
	const TCHAR* const StyledLayoutFixture = TEXT("Form/RangeStyledLayoutTest.rml");
	const TCHAR* const StyledLayoutSheet = TEXT("RangeStyledLayout.rcssc");
	const TCHAR* const DefaultPixelFixture = TEXT("Form/RangeDefaultPixelTest.rml");
	const TCHAR* const DefaultPixelSheet = TEXT("RangeDefaultPixel.rcssc");
	const TCHAR* const ModelName = TEXT("re");

	// RangeEvents.rcss: a 216 px wide input whose 16 px thumb travels 200 px, so value v of 0..100 puts the thumb's left edge at
	// 2v and its centre at 8 + 2v from the input's left edge.
	constexpr float ThumbHalf = 8.0f;
	constexpr float Travel = 200.0f;
	constexpr float BoxTolerance = 0.5f;
	constexpr int32 PixelTolerance = 2;

	float ToFloat(const FArcRmlValue& Value)
	{
		return Value.Kind == EArcRmlValueKind::Float ? static_cast<float>(Value.Float)
			: Value.Kind == EArcRmlValueKind::Int ? static_cast<float>(Value.Int) : -1000.0f;
	}

	struct FRangeBoxes
	{
		const FArcRmlTestLayoutElement* Input = nullptr;
		const FArcRmlTestLayoutElement* Track = nullptr;
		const FArcRmlTestLayoutElement* Thumb = nullptr;
	};

	FRangeBoxes FindBoxes(const TArray<FArcRmlTestLayoutElement>& Layout, const TCHAR* DomId)
	{
		FRangeBoxes Boxes;
		Boxes.Input = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Boxes.Input)
		{
			return Boxes;
		}
		const FString& InputPath = Boxes.Input->Path;
		Boxes.Track = Layout.FindByPredicate([&InputPath](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == InputPath && Element.Tag == TEXT("slidertrack");
		});
		Boxes.Thumb = Layout.FindByPredicate([&InputPath](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == InputPath && Element.Tag == TEXT("sliderbar");
		});
		return Boxes;
	}

	FString FormatNumber(const FArcRmlValue& Value)
	{
		if (Value.Kind == EArcRmlValueKind::Float)
		{
			return FString::Printf(TEXT("%.1f"), Value.Float);
		}
		if (Value.Kind == EArcRmlValueKind::Int)
		{
			return FString::Printf(TEXT("%lld.0"), Value.Int);
		}
		return FString::Printf(TEXT("<kind %d '%s'>"), static_cast<int32>(Value.Kind), *Value.String);
	}

	// The input and change calls the fixture's handlers made for one input, in order: "In 25.0, Ch 25.0".
	FString DescribeEvents(const FArcRmlTestView& View, const TCHAR* Tag)
	{
		TArray<FString> Parts;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if ((Call.FunctionName != TEXT("In") && Call.FunctionName != TEXT("Ch")) || Call.Arguments.Num() == 0 || Call.Arguments[0].String != Tag)
			{
				continue;
			}
			Parts.Add(FString::Printf(TEXT("%s %s"), *Call.FunctionName, Call.Arguments.Num() == 2 ? *FormatNumber(Call.Arguments[1]) : TEXT("<no value>")));
		}
		return FString::Join(Parts, TEXT(", "));
	}
}

TEST_CLASS(ArcRmlRangeEventTests, "ArcRmlUI.Range.Events")
{
	FArcRmlTestView View;
	FString Mismatches;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlRangeTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(EventsFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), EventsSheet), false, true, true);
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool Settle()
	{
		return View.Flush() && View.Flush();
	}

	ArcRmlRangeTestsPrivate::FRangeBoxes Boxes(const TCHAR* DomId, TArray<FArcRmlTestLayoutElement>& Layout)
	{
		Layout.Reset();
		View.ParseLayout(Layout);
		return ArcRmlRangeTestsPrivate::FindBoxes(Layout, DomId);
	}

	// A point on the input at the x where value v of 0..100 centres the thumb; OnThumb picks the thumb's row, else the input's
	// top row, above the track and the thumb.
	FVector2f PointFor(const TCHAR* DomId, float Value, bool bOnThumb)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(DomId, Layout);
		check(Found.Input);
		const float X = Found.Input->Rect.Position.X + ThumbHalf + Travel * Value / 100.0f;
		const float Y = Found.Input->Rect.Position.Y + (bOnThumb ? Found.Input->Rect.Size.Y * 0.5f : 1.0f);
		return FVector2f(X, Y);
	}

	void ExpectEvents(const TCHAR* Step, const TCHAR* Tag, const TCHAR* Expected)
	{
		const FString Measured = ArcRmlRangeTestsPrivate::DescribeEvents(View, Tag);
		if (Measured != Expected)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s fired '%s', expected '%s'] "), Step, Tag, *Measured, Expected);
		}
	}

	// Value v of Min..Max puts the thumb's left edge at Travel * (v - Min) / (Max - Min), rounded to whole pixels.
	void ExpectValue(const TCHAR* Step, const TCHAR* DomId, float Value, float Min = 0.0f, float Max = 100.0f)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(DomId, Layout);
		if (!Found.Input || !Found.Thumb)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s has no thumb] "), Step, DomId);
			return;
		}
		const float Expected = FMath::RoundToFloat(Travel * (Value - Min) / (Max - Min));
		const float Measured = Found.Thumb->Rect.Position.X - Found.Input->Rect.Position.X;
		if (FMath::Abs(Measured - Expected) > BoxTolerance)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s thumb at %.2f, expected %.2f for value %.1f] "), Step, DomId, Measured, Expected, Value);
		}
	}

	void ExpectFocus(const TCHAR* Step, const TCHAR* DomId)
	{
		const FString Focus = ArcRmlUITestHelpers::GetFocusDomId(View);
		if (Focus != DomId)
		{
			Mismatches += FString::Printf(TEXT("[%s: focus on '%s' (path '%s'), expected '%s'] "), Step, *Focus, *View.GetProbe().FocusPath, DomId);
		}
	}

	bool Press(const FKey& Key)
	{
		View.ClearCalls();
		View.PressKey(Key);
		return Settle();
	}

	// A press on the thumb's centre keeps the value and focuses the input.
	bool FocusByThumb(const TCHAR* DomId, float Value)
	{
		const FVector2f Point = PointFor(DomId, Value, true);
		View.MouseMove(Point);
		View.MouseDown(Point);
		View.MouseUp(Point);
		return View.FlushUntilSettled() && Settle();
	}

	// A point on the track's centre row at the x where value v of 0..100 centres the thumb.
	FVector2f TrackPointFor(const TCHAR* DomId, float Value)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(DomId, Layout);
		check(Found.Input && Found.Track);
		const float X = Found.Input->Rect.Position.X + ThumbHalf + Travel * Value / 100.0f;
		const float Y = Found.Track->Rect.Position.Y + Found.Track->Rect.Size.Y * 0.5f;
		return FVector2f(X, Y);
	}

	bool PressAndRelease(const FVector2f& Point)
	{
		View.ClearCalls();
		View.MouseMove(Point);
		View.MouseDown(Point);
		View.MouseUp(Point);
		return View.FlushUntilSettled() && Settle();
	}

	// RangeEvents.rcss: re-main is 20 px high, 21 px with :focus and 22 px with :focus-visible; re-wrap is 240 px wide, 250 px
	// with :focus-within.
	void ExpectFocusStyles(const TCHAR* Step, float InputHeight, float WrapWidth)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(TEXT("re-main"), Layout);
		const FArcRmlTestLayoutElement* Wrap = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("re-wrap"));
		if (!Found.Input || !Wrap)
		{
			Mismatches += FString::Printf(TEXT("[%s: re-main or re-wrap missing] "), Step);
			return;
		}
		if (FMath::Abs(Found.Input->Rect.Size.Y - InputHeight) > BoxTolerance)
		{
			Mismatches += FString::Printf(TEXT("[%s: re-main %.2f px high, expected %.0f] "), Step, Found.Input->Rect.Size.Y, InputHeight);
		}
		if (FMath::Abs(Wrap->Rect.Size.X - WrapWidth) > BoxTolerance)
		{
			Mismatches += FString::Printf(TEXT("[%s: re-wrap %.2f px wide, expected %.0f] "), Step, Wrap->Rect.Size.X, WrapWidth);
		}
	}

	// After a pointer press on re-main: the input itself holds focus, matches :focus but not :focus-visible, re-wrap matches
	// :focus-within, and one click reached the input's handler with the input as its target (ev.offset_x and ev.offset_y from the
	// input's padding box, not from the track's or the thumb's).
	void ExpectPointerFocus(const TCHAR* Step, const FVector2f& ClickOffset)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(TEXT("re-main"), Layout);
		if (!Found.Input)
		{
			Mismatches += FString::Printf(TEXT("[%s: re-main missing] "), Step);
			return;
		}
		const FArcRmlTestProbe& Probe = View.GetProbe();
		if (Probe.FocusPath != Found.Input->Path)
		{
			Mismatches += FString::Printf(TEXT("[%s: focus path '%s', expected re-main's '%s'] "), Step, *Probe.FocusPath, *Found.Input->Path);
		}
		if (Probe.bFocusVisible)
		{
			Mismatches += FString::Printf(TEXT("[%s: the focused element has :focus-visible] "), Step);
		}
		ExpectFocusStyles(Step, 21.0f, 250.0f);
		TArray<FVector2f> Offsets;
		for (const FArcRmlViewModelCall& Call : View.GetCalls())
		{
			if (Call.FunctionName == TEXT("Cl") && Call.Arguments.Num() == 3 && Call.Arguments[0].String == TEXT("main"))
			{
				Offsets.Add(FVector2f(ToFloat(Call.Arguments[1]), ToFloat(Call.Arguments[2])));
			}
		}
		if (Offsets.Num() != 1 || FMath::Abs(Offsets[0].X - ClickOffset.X) > 1.0f || FMath::Abs(Offsets[0].Y - ClickOffset.Y) > 1.0f)
		{
			TArray<FString> Described;
			for (const FVector2f& Offset : Offsets)
			{
				Described.Add(FString::Printf(TEXT("%.1f,%.1f"), Offset.X, Offset.Y));
			}
			Mismatches += FString::Printf(TEXT("[%s: click offsets on re-main [%s], expected one at %.1f,%.1f] "), Step,
				*FString::Join(Described, TEXT(", ")), ClickOffset.X, ClickOffset.Y);
		}
	}

	// A press on a disabled range focuses neither the input nor one of its parts.
	void ExpectNoFocusIn(const TCHAR* Step, const TCHAR* DomId)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(DomId, Layout);
		const FString& FocusPath = View.GetProbe().FocusPath;
		if (!Found.Input)
		{
			Mismatches += FString::Printf(TEXT("[%s: %s missing] "), Step, DomId);
			return;
		}
		if (FocusPath == Found.Input->Path || FocusPath.StartsWith(Found.Input->Path + TEXT(".")) || FocusPath.StartsWith(Found.Input->Path + TEXT("~")))
		{
			Mismatches += FString::Printf(TEXT("[%s: focus path '%s' is in %s ('%s')] "), Step, *FocusPath, DomId, *Found.Input->Path);
		}
	}

	TEST_METHOD(Open_FiresNothingAndABareRangeSitsAtTheMidpoint)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Settle()));
		for (const TCHAR* Tag : {TEXT("main"), TEXT("bound"), TEXT("derive"), TEXT("bare"), TEXT("off")})
		{
			ExpectEvents(TEXT("open"), Tag, TEXT(""));
		}
		ExpectValue(TEXT("bare range"), TEXT("re-bare"), 50.0f);
		ExpectValue(TEXT("bound value"), TEXT("re-bound"), 50.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(PressOutsideTheTrack_MovesTheValueToThePointerAndChangesAtRelease)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Point = PointFor(TEXT("re-main"), 25.0f, false);
		View.ClearCalls();
		View.MouseMove(Point);
		View.MouseDown(Point);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("press above the track"), TEXT("main"), TEXT("In 25.0"));
		ExpectValue(TEXT("press above the track"), TEXT("re-main"), 25.0f);
		View.MouseUp(Point);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("release"), TEXT("main"), TEXT("In 25.0, Ch 25.0"));
		ExpectValue(TEXT("release"), TEXT("re-main"), 25.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(PressOnTheThumb_KeepsTheValue)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Point = PointFor(TEXT("re-main"), 50.0f, true) + FVector2f(5.0f, 0.0f);
		View.ClearCalls();
		View.MouseMove(Point);
		View.MouseDown(Point);
		View.MouseUp(Point);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("press 5 px right of the thumb's centre"), TEXT("main"), TEXT(""));
		ExpectValue(TEXT("press 5 px right of the thumb's centre"), TEXT("re-main"), 50.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(PressOnTheThumb_FocusesTheInputWithoutFocusVisibleAndTheNextKeySteps)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectFocusStyles(TEXT("before the press"), 20.0f, 240.0f);
		ASSERT_THAT(IsTrue(PressAndRelease(PointFor(TEXT("re-main"), 50.0f, true))));
		ExpectEvents(TEXT("press on the thumb's centre"), TEXT("main"), TEXT(""));
		ExpectPointerFocus(TEXT("press on the thumb's centre"), FVector2f(108.0f, 10.0f));
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ExpectEvents(TEXT("Right after the thumb press"), TEXT("main"), TEXT("In 51.0, Ch 51.0"));
		ExpectValue(TEXT("Right after the thumb press"), TEXT("re-main"), 51.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// At value 50 the thumb spans x 100..116 of the input; value 25 (x 58) lies on the progress left of it.
	TEST_METHOD(PressOnTheProgress_FocusesTheInputWithoutFocusVisibleAndTheNextKeySteps)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectFocusStyles(TEXT("before the press"), 20.0f, 240.0f);
		ASSERT_THAT(IsTrue(PressAndRelease(TrackPointFor(TEXT("re-main"), 25.0f))));
		ExpectEvents(TEXT("press on the progress at 25"), TEXT("main"), TEXT("In 25.0, Ch 25.0"));
		ExpectValue(TEXT("press on the progress at 25"), TEXT("re-main"), 25.0f);
		ExpectPointerFocus(TEXT("press on the progress at 25"), FVector2f(58.0f, 10.0f));
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ExpectEvents(TEXT("Right after the progress press"), TEXT("main"), TEXT("In 26.0, Ch 26.0"));
		ExpectValue(TEXT("Right after the progress press"), TEXT("re-main"), 26.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// Value 75 (x 158) lies on the bare track right of the thumb at value 50, outside the progress.
	TEST_METHOD(PressOnTheBareTrack_FocusesTheInputWithoutFocusVisibleAndTheNextKeySteps)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectFocusStyles(TEXT("before the press"), 20.0f, 240.0f);
		ASSERT_THAT(IsTrue(PressAndRelease(TrackPointFor(TEXT("re-main"), 75.0f))));
		ExpectEvents(TEXT("press on the track at 75"), TEXT("main"), TEXT("In 75.0, Ch 75.0"));
		ExpectValue(TEXT("press on the track at 75"), TEXT("re-main"), 75.0f);
		ExpectPointerFocus(TEXT("press on the track at 75"), FVector2f(158.0f, 10.0f));
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ExpectEvents(TEXT("Right after the track press"), TEXT("main"), TEXT("In 76.0, Ch 76.0"));
		ExpectValue(TEXT("Right after the track press"), TEXT("re-main"), 76.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(Drag_FiresInputPerValueAndChangeOnceAtRelease)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Start = PointFor(TEXT("re-main"), 25.0f, true);
		const FVector2f Middle = PointFor(TEXT("re-main"), 40.0f, true);
		const FVector2f End = PointFor(TEXT("re-main"), 60.0f, true);
		View.ClearCalls();
		View.MouseMove(Start);
		View.MouseDown(Start);
		View.MouseMove(Middle);
		View.MouseMove(End);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("drag 25 to 60"), TEXT("main"), TEXT("In 25.0, In 40.0, In 60.0"));
		ExpectValue(TEXT("drag 25 to 60"), TEXT("re-main"), 60.0f);
		View.MouseUp(End);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("release at 60"), TEXT("main"), TEXT("In 25.0, In 40.0, In 60.0, Ch 60.0"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// The press renders a frame before the first move, as in a game: the move after the settled press still drags and records a
	// new frame.
	TEST_METHOD(MoveAfterASettledPress_DragsAndRecordsAFrame)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Start = PointFor(TEXT("re-main"), 25.0f, true);
		const FVector2f End = PointFor(TEXT("re-main"), 60.0f, true);
		View.ClearCalls();
		View.MouseMove(Start);
		View.MouseDown(Start);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("settled press at 25"), TEXT("main"), TEXT("In 25.0"));
		const uint64 PressSerial = View.GetProbe().FrameSerial;
		View.MouseMove(End);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("move to 60 after the settled press"), TEXT("main"), TEXT("In 25.0, In 60.0"));
		ExpectValue(TEXT("move to 60 after the settled press"), TEXT("re-main"), 60.0f);
		const uint64 MoveSerial = View.GetProbe().FrameSerial;
		if (MoveSerial <= PressSerial)
		{
			Mismatches += FString::Printf(TEXT("[move to 60 after the settled press: frame serial %llu did not advance past the press's %llu] "), MoveSerial,
				PressSerial);
		}
		View.MouseUp(End);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("release at 60"), TEXT("main"), TEXT("In 25.0, In 60.0, Ch 60.0"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(DragThereAndBack_FiresTwoInputsAndNoChange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Start = PointFor(TEXT("re-main"), 50.0f, true);
		const FVector2f Away = PointFor(TEXT("re-main"), 60.0f, true);
		View.ClearCalls();
		View.MouseMove(Start);
		View.MouseDown(Start);
		View.MouseMove(Away);
		View.MouseMove(Start);
		View.MouseUp(Start);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("drag 50 to 60 to 50"), TEXT("main"), TEXT("In 60.0, In 50.0"));
		ExpectValue(TEXT("drag 50 to 60 to 50"), TEXT("re-main"), 50.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(Keys_StepJumpToTheEndsAndDoNothingPastThem)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusByThumb(TEXT("re-main"), 50.0f)));
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ExpectEvents(TEXT("Right"), TEXT("main"), TEXT("In 51.0, Ch 51.0"));
		ExpectValue(TEXT("Right"), TEXT("re-main"), 51.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Left)));
		ExpectEvents(TEXT("Left"), TEXT("main"), TEXT("In 50.0, Ch 50.0"));
		ExpectValue(TEXT("Left"), TEXT("re-main"), 50.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Home)));
		ExpectEvents(TEXT("Home"), TEXT("main"), TEXT("In 0.0, Ch 0.0"));
		ExpectValue(TEXT("Home"), TEXT("re-main"), 0.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Left)));
		ExpectEvents(TEXT("Left at the minimum"), TEXT("main"), TEXT(""));
		ExpectValue(TEXT("Left at the minimum"), TEXT("re-main"), 0.0f);
		ExpectFocus(TEXT("Left at the minimum"), TEXT("re-main"));
		ASSERT_THAT(IsTrue(Press(EKeys::End)));
		ExpectEvents(TEXT("End"), TEXT("main"), TEXT("In 100.0, Ch 100.0"));
		ExpectValue(TEXT("End"), TEXT("re-main"), 100.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ExpectEvents(TEXT("Right at the maximum"), TEXT("main"), TEXT(""));
		ExpectValue(TEXT("Right at the maximum"), TEXT("re-main"), 100.0f);
		ExpectFocus(TEXT("Right at the maximum"), TEXT("re-main"));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ExpectEvents(TEXT("Down"), TEXT("main"), TEXT(""));
		ExpectValue(TEXT("Down"), TEXT("re-main"), 100.0f);
		ExpectFocus(TEXT("Down"), TEXT("re-after"));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("re-bound")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Up)));
		ExpectEvents(TEXT("Up"), TEXT("bound"), TEXT(""));
		ExpectValue(TEXT("Up"), TEXT("re-bound"), 50.0f);
		ExpectFocus(TEXT("Up"), TEXT("re-after"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(PadLeftRight_StepAndPadDownLeaves)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(Settle()));
		ASSERT_THAT(AreEqual(FString(TEXT("re-main")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Right)));
		ExpectEvents(TEXT("pad Right"), TEXT("main"), TEXT("In 51.0, Ch 51.0"));
		ExpectValue(TEXT("pad Right"), TEXT("re-main"), 51.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Left)));
		ExpectEvents(TEXT("pad Left"), TEXT("main"), TEXT("In 50.0, Ch 50.0"));
		ExpectValue(TEXT("pad Left"), TEXT("re-main"), 50.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		ExpectEvents(TEXT("pad Down"), TEXT("main"), TEXT(""));
		ExpectValue(TEXT("pad Down"), TEXT("re-main"), 50.0f);
		const FString NavFocus = ArcRmlUITestHelpers::GetNavFocusDomId(View);
		if (NavFocus != TEXT("re-after"))
		{
			Mismatches += FString::Printf(TEXT("[pad Down: nav focus on '%s', expected 're-after'] "), *NavFocus);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ValueWrittenThroughABinding_IsClampedAndFiresNothing)
	{
		using namespace ArcRmlRangeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("v")), ArcRmlUITestHelpers::MakeInt(30));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectEvents(TEXT("value 30"), TEXT("bound"), TEXT(""));
		ExpectValue(TEXT("value 30"), TEXT("re-bound"), 30.0f);
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("v")), ArcRmlUITestHelpers::MakeInt(150));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectEvents(TEXT("value 150"), TEXT("bound"), TEXT(""));
		ExpectValue(TEXT("value 150"), TEXT("re-bound"), 100.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// re-derive keeps value="50"; each min, max or step write re-derives the value from that attribute.
	TEST_METHOD(MinMaxStepWrittenThroughBindings_ReDeriveFromTheValueAttributeAndFireNothing)
	{
		using namespace ArcRmlRangeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("mx")), ArcRmlUITestHelpers::MakeInt(40));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectEvents(TEXT("max 40"), TEXT("derive"), TEXT(""));
		ExpectValue(TEXT("max 40"), TEXT("re-derive"), 40.0f, 0.0f, 40.0f);
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("mx")), ArcRmlUITestHelpers::MakeInt(100));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectEvents(TEXT("max back to 100"), TEXT("derive"), TEXT(""));
		ExpectValue(TEXT("max back to 100"), TEXT("re-derive"), 50.0f);
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("mn")), ArcRmlUITestHelpers::MakeInt(5));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("st")), ArcRmlUITestHelpers::MakeInt(20));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectEvents(TEXT("min 5 step 20"), TEXT("derive"), TEXT(""));
		ExpectValue(TEXT("min 5 step 20: 5 + 2 * 20"), TEXT("re-derive"), 45.0f, 5.0f, 100.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(ValueWrittenDuringADrag_AppliesAtReleaseOnlyWhenNoLaterDragMoved)
	{
		using namespace ArcRmlRangeTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f At50 = PointFor(TEXT("re-bound"), 50.0f, true);
		const FVector2f At60 = PointFor(TEXT("re-bound"), 60.0f, true);
		View.ClearCalls();
		View.MouseMove(At50);
		View.MouseDown(At50);
		View.MouseMove(At60);
		ASSERT_THAT(IsTrue(Settle()));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("v")), ArcRmlUITestHelpers::MakeInt(10));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectValue(TEXT("value 10 written during the drag"), TEXT("re-bound"), 60.0f);
		View.MouseUp(At60);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("release after the write"), TEXT("bound"), TEXT("In 60.0, Ch 60.0"));
		ExpectValue(TEXT("release after the write"), TEXT("re-bound"), 10.0f);

		const FVector2f At10 = PointFor(TEXT("re-bound"), 10.0f, true);
		const FVector2f At40 = PointFor(TEXT("re-bound"), 40.0f, true);
		const FVector2f At70 = PointFor(TEXT("re-bound"), 70.0f, true);
		View.ClearCalls();
		View.MouseMove(At10);
		View.MouseDown(At10);
		View.MouseMove(At40);
		ASSERT_THAT(IsTrue(Settle()));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("v")), ArcRmlUITestHelpers::MakeInt(35));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectValue(TEXT("stale value 35 written during the drag"), TEXT("re-bound"), 40.0f);
		View.MouseMove(At70);
		View.MouseUp(At70);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("drag on past the stale write"), TEXT("bound"), TEXT("In 40.0, In 70.0, Ch 70.0"));
		ExpectValue(TEXT("drag on past the stale write"), TEXT("re-bound"), 70.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// A disabled range cannot take focus, so no key reaches it; only the pointer is checked.
	TEST_METHOD(DisabledRange_FiresNothingAndKeepsItsValue)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f Start = PointFor(TEXT("re-off"), 25.0f, false);
		const FVector2f End = PointFor(TEXT("re-off"), 80.0f, false);
		View.ClearCalls();
		View.MouseMove(Start);
		View.MouseDown(Start);
		View.MouseMove(End);
		View.MouseUp(End);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("press and drag"), TEXT("off"), TEXT(""));
		ExpectValue(TEXT("press and drag"), TEXT("re-off"), 50.0f);
		ExpectNoFocusIn(TEXT("press and drag"), TEXT("re-off"));
		ASSERT_THAT(IsTrue(PressAndRelease(PointFor(TEXT("re-off"), 50.0f, true))));
		ExpectNoFocusIn(TEXT("press on the thumb"), TEXT("re-off"));
		ASSERT_THAT(IsTrue(PressAndRelease(TrackPointFor(TEXT("re-off"), 25.0f))));
		ExpectNoFocusIn(TEXT("press on the track"), TEXT("re-off"));
		ExpectEvents(TEXT("presses on the thumb and the track"), TEXT("off"), TEXT(""));
		ExpectValue(TEXT("presses on the thumb and the track"), TEXT("re-off"), 50.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(FocusLeavingTheRange_ClearsFocusWithinOnItsAncestor)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusByThumb(TEXT("re-main"), 50.0f)));
		ExpectFocusStyles(TEXT("range focused"), 21.0f, 250.0f);
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ExpectFocus(TEXT("Down out of the range"), TEXT("re-after"));
		ExpectFocusStyles(TEXT("Down out of the range"), 20.0f, 240.0f);
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	// re-scaled-wrap is scale(2) about its centre O, so a layout point p shows on screen at O + 2 * (p - O).
	FVector2f ScaledPointFor(float Value)
	{
		using namespace ArcRmlRangeTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		const FRangeBoxes Found = Boxes(TEXT("re-scaled"), Layout);
		const FArcRmlTestLayoutElement* Wrap = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("re-scaled-wrap"));
		check(Found.Input && Wrap);
		const FVector2f Origin = Wrap->Rect.Position + Wrap->Rect.Size * 0.5f;
		const FVector2f Local(Found.Input->Rect.Position.X + ThumbHalf + Travel * Value / 100.0f, Found.Input->Rect.Position.Y + Found.Input->Rect.Size.Y * 0.5f);
		return Origin + (Local - Origin) * 2.0f;
	}

	void ExpectScaledThumbUnder(const TCHAR* Step, const FVector2f& Pointer)
	{
		TArray<FArcRmlTestLayoutElement> Layout;
		const ArcRmlRangeTestsPrivate::FRangeBoxes Found = Boxes(TEXT("re-scaled"), Layout);
		if (!Found.Thumb)
		{
			Mismatches += FString::Printf(TEXT("[%s: re-scaled has no thumb] "), Step);
			return;
		}
		const float Centre = Found.Thumb->TransformedRect.Position.X + Found.Thumb->TransformedRect.Size.X * 0.5f;
		if (FMath::Abs(Centre - Pointer.X) > 1.0f)
		{
			Mismatches += FString::Printf(TEXT("[%s: thumb's transformed centre at x %.2f, pointer at %.2f] "), Step, Centre, Pointer.X);
		}
	}

	TEST_METHOD(PressAndDragUnderAScaledParent_FollowThePointerInTransformedSpace)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FVector2f At25 = ScaledPointFor(25.0f);
		const FVector2f At75 = ScaledPointFor(75.0f);
		View.ClearCalls();
		View.MouseMove(At25);
		View.MouseDown(At25);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("press at 25 under scale(2)"), TEXT("scaled"), TEXT("In 25.0"));
		ExpectValue(TEXT("press at 25 under scale(2)"), TEXT("re-scaled"), 25.0f);
		ExpectScaledThumbUnder(TEXT("press at 25 under scale(2)"), At25);
		View.MouseMove(At75);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("drag to 75 under scale(2)"), TEXT("scaled"), TEXT("In 25.0, In 75.0"));
		ExpectValue(TEXT("drag to 75 under scale(2)"), TEXT("re-scaled"), 75.0f);
		ExpectScaledThumbUnder(TEXT("drag to 75 under scale(2)"), At75);
		View.MouseUp(At75);
		ASSERT_THAT(IsTrue(Settle()));
		ExpectEvents(TEXT("release at 75 under scale(2)"), TEXT("scaled"), TEXT("In 25.0, In 75.0, Ch 75.0"));
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};

// Expected boxes are Chrome 156's ink of RangeStyledLayoutTest (Saved/ArcRml/RangeStep3Chrome/post/RangeStyledLayoutTest.ink.txt):
// blue is the track, green the thumb; the thumb covers the track's ink at either end, so the track box spans both.
TEST_CLASS(ArcRmlRangeLayoutTests, "ArcRmlUI.CssParity.RangeLayout")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	struct FInk
	{
		const TCHAR* DomId;
		FVector4f Track;
		FVector4f Thumb;
	};

	static FString DescribeBox(const TCHAR* DomId, const TCHAR* Part, const FArcRmlTestLayoutElement* Element, const FVector4f& Expected)
	{
		using namespace ArcRmlRangeTestsPrivate;
		if (!Element)
		{
			return FString::Printf(TEXT("[%s %s missing] "), DomId, Part);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Expected.X) <= BoxTolerance && FMath::Abs(Rect.Position.Y - Expected.Y) <= BoxTolerance
			&& FMath::Abs(Rect.Size.X - Expected.Z) <= BoxTolerance && FMath::Abs(Rect.Size.Y - Expected.W) <= BoxTolerance)
		{
			return FString();
		}
		return FString::Printf(TEXT("[%s %s %.2f,%.2f %.2fx%.2f, Chrome %.0f,%.0f %.0fx%.0f] "), DomId, Part, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
			Rect.Size.Y, Expected.X, Expected.Y, Expected.Z, Expected.W);
	}

	TEST_METHOD(StyledParts_MatchChromeBoxes)
	{
		using namespace ArcRmlRangeTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(StyledLayoutFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), StyledLayoutSheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("RangeLayout"))));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));

		const FInk Chrome[] = {
			{TEXT("rs-mid"), FVector4f(2.0f, 72.0f, 240.0f, 6.0f), FVector4f(114.0f, 67.0f, 16.0f, 16.0f)},
			{TEXT("rs-min"), FVector4f(18.0f, 132.0f, 224.0f, 6.0f), FVector4f(2.0f, 127.0f, 16.0f, 16.0f)},
			{TEXT("rs-max"), FVector4f(2.0f, 192.0f, 224.0f, 6.0f), FVector4f(226.0f, 187.0f, 16.0f, 16.0f)},
			{TEXT("rs-tall"), FVector4f(2.0f, 264.0f, 240.0f, 6.0f), FVector4f(69.0f, 259.0f, 16.0f, 16.0f)},
			{TEXT("rs-pad"), FVector4f(14.0f, 318.0f, 240.0f, 6.0f), FVector4f(171.0f, 313.0f, 16.0f, 16.0f)},
		};
		FString Mismatches;
		for (const FInk& Ink : Chrome)
		{
			const FRangeBoxes Found = FindBoxes(Layout, Ink.DomId);
			const float Left = FMath::Min(Ink.Track.X, Ink.Thumb.X);
			const float Right = FMath::Max(Ink.Track.X + Ink.Track.Z, Ink.Thumb.X + Ink.Thumb.Z);
			Mismatches += DescribeBox(Ink.DomId, TEXT("track"), Found.Track, FVector4f(Left, Ink.Track.Y, Right - Left, Ink.Track.W));
			Mismatches += DescribeBox(Ink.DomId, TEXT("thumb"), Found.Thumb, Ink.Thumb);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};

// Expected colours are read from Chrome 156's screenshot of RangeDefaultPixelTest (Saved/ArcRml/RangeStep3Chrome/post/RangeDefaultPixelTest.png),
// at points at least 2 px inside the track, the filled part and the thumb in both renderings; the "thumb above the track" points are
// 6 px from the thumb's centre, about 1.4 px inside Chrome's circle of radius about 7.4.
TEST_CLASS(ArcRmlRangePixelTests, "ArcRmlUI.Pixels.RangeDefault")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	TEST_METHOD(DefaultLook_BareAndWithAccentColor_MatchesChromePixels)
	{
		using namespace ArcRmlRangeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(DefaultPixelFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), DefaultPixelSheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));

		const FColor Track(239, 239, 239, 255);
		const FColor Blue(0, 117, 255, 255);
		const FColor Red(220, 30, 60, 255);
		const FChromePixel Expected[] = {
			{TEXT("bare track"), 100, 10, Track},
			{TEXT("bare fill"), 30, 10, Blue},
			{TEXT("bare thumb centre"), 67, 10, Blue},
			{TEXT("bare thumb above the track"), 67, 4, Blue},
			{TEXT("accent track"), 100, 50, Track},
			{TEXT("accent fill"), 30, 50, Red},
			{TEXT("accent thumb centre"), 67, 50, Red},
			{TEXT("accent thumb above the track"), 67, 44, Red},
			{TEXT("quarter fill"), 15, 90, Blue},
			{TEXT("quarter thumb centre"), 38, 90, Blue},
			{TEXT("quarter thumb above the track"), 38, 84, Blue},
			{TEXT("quarter track past the thumb"), 60, 90, Track},
			{TEXT("wide fill"), 30, 130, Red},
			{TEXT("wide thumb centre"), 237, 130, Red},
			{TEXT("wide thumb above the track"), 237, 124, Red},
			{TEXT("wide track"), 280, 130, Track},
			{TEXT("tall thumb centre"), 10, 177, Blue},
			{TEXT("tall thumb above the track"), 10, 171, Blue},
			{TEXT("tall track"), 60, 177, Track},
		};
		FString Mismatches;
		for (const FChromePixel& Pixel : Expected)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (FMath::Abs(Measured.R - Pixel.Chrome.R) > PixelTolerance || FMath::Abs(Measured.G - Pixel.Chrome.G) > PixelTolerance
				|| FMath::Abs(Measured.B - Pixel.Chrome.B) > PixelTolerance || FMath::Abs(Measured.A - Pixel.Chrome.A) > PixelTolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s (%d,%d): %d,%d,%d,%d, Chrome %d,%d,%d,%d] "), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G,
					Measured.B, Measured.A, Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B, Pixel.Chrome.A);
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}
};

// Draw cost of frames that replay content (never the idle composite): the first draw after the open and the draws of frames
// recorded while a slider is dragged. Each draw's counted GPU work and its geometry share are written to the test log; the
// geometry must stay merged into the measured draw calls (all five default sliders in one, the styled page in two).
TEST_CLASS(ArcRmlRangeDrawTests, "ArcRmlUI.Pixels.RangeDraws")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Fixture, const TCHAR* Sheet)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		View.SetDestination(FIntRect(0, 0, 1280, 720));
		return true;
	}

	bool RecordNewFrame(TFunctionRef<void()> Change)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		Change();
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
	}

	bool DrawContent(const TCHAR* Fixture, const TCHAR* Step, int32 GeometryDrawCalls)
	{
		if (!View.DrawFrame())
		{
			TestRunner->AddError(FString::Printf(TEXT("%s %s: the frame was not drawn"), Fixture, Step));
			return false;
		}
		TestRunner->AddInfo(FString::Printf(TEXT("RangeDraws %s %s: %d draw calls, %d RHI commands (geometry %d draw calls, %d RHI commands)"), Fixture, Step,
			View.GetDrawGpuDrawCalls(), View.GetDrawGpuRhiCommands(), View.GetReplayDrawCalls(), View.GetReplayRhiCommands()));
		if (View.GetReplayDrawCalls() != GeometryDrawCalls)
		{
			TestRunner->AddError(FString::Printf(TEXT("%s %s: the frame's geometry took %d draw calls, expected %d"), Fixture, Step, View.GetReplayDrawCalls(),
				GeometryDrawCalls));
			return false;
		}
		return true;
	}

	void MeasureDrag(const TCHAR* Fixture, const TCHAR* DomId, int32 GeometryDrawCalls)
	{
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		const FArcRmlTestLayoutElement* Input = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		ASSERT_THAT(IsNotNull(Input));
		const float Y = Input->Rect.Position.Y + Input->Rect.Size.Y * 0.5f;
		const FVector2f Start(Input->Rect.Position.X + Input->Rect.Size.X * 0.2f, Y);
		const FVector2f Middle(Input->Rect.Position.X + Input->Rect.Size.X * 0.5f, Y);
		const FVector2f End(Input->Rect.Position.X + Input->Rect.Size.X * 0.8f, Y);
		ASSERT_THAT(IsTrue(RecordNewFrame([this, Start, Middle]()
		{
			View.MouseMove(Start);
			View.MouseDown(Start);
			View.MouseMove(Middle);
		}), TEXT("the press and drag recorded no frame")));
		ASSERT_THAT(IsTrue(DrawContent(Fixture, TEXT("drag"), GeometryDrawCalls)));
		ASSERT_THAT(IsTrue(RecordNewFrame([this, End]()
		{
			View.MouseMove(End);
			View.MouseUp(End);
		}), FString::Printf(TEXT("the drag on and release recorded no frame (serial %llu)"), View.GetProbe().FrameSerial)));
		ASSERT_THAT(IsTrue(DrawContent(Fixture, TEXT("release"), GeometryDrawCalls)));
	}

	TEST_METHOD(DefaultLook_FirstDrawAndDrag)
	{
		using namespace ArcRmlRangeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(DefaultPixelFixture, DefaultPixelSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(DrawContent(TEXT("RangeDefaultPixel"), TEXT("first draw"), 1)));
		MeasureDrag(TEXT("RangeDefaultPixel"), TEXT("rd-bare"), 1);
	}

	TEST_METHOD(StyledLayout_FirstDrawAndDrag)
	{
		using namespace ArcRmlRangeTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenFixture(StyledLayoutFixture, StyledLayoutSheet), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(DrawContent(TEXT("RangeStyledLayout"), TEXT("first draw"), 2)));
		MeasureDrag(TEXT("RangeStyledLayout"), TEXT("rs-mid"), 2);
	}
};
