// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CommonInputTypeEnum.h"
#include "CQTest.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"

namespace ArcRmlFocusWithinTestsPrivate
{
	struct FBox
	{
		const TCHAR* DomId;
		float Y;
		float Width;
		float Height;
	};

	const TCHAR* const FocusWithinFixture = TEXT("CssParity/Selectors/FocusWithinTest.rml");
	const TCHAR* const FocusWithinMovedFixture = TEXT("CssParity/Selectors/FocusWithinMovedTest.rml");
	const TCHAR* const RangeNavFixture = TEXT("Focus/RangeNavTest.rml");

	const FBox ChromeFirstFieldFocused[] = {
		{TEXT("outer"), 0.0f, 1280.0f, 28.0f},
		{TEXT("mid"), 4.0f, 1280.0f, 14.0f},
		{TEXT("fi"), 8.0f, 50.0f, 10.0f},
		{TEXT("mark"), 18.0f, 70.0f, 10.0f},
		{TEXT("after"), 28.0f, 40.0f, 20.0f},
		{TEXT("side"), 48.0f, 1280.0f, 10.0f},
		{TEXT("fi2"), 48.0f, 30.0f, 10.0f},
		{TEXT("after2"), 58.0f, 40.0f, 10.0f},
	};

	const FBox ChromeSecondFieldFocused[] = {
		{TEXT("outer"), 0.0f, 1280.0f, 20.0f},
		{TEXT("mid"), 0.0f, 1280.0f, 10.0f},
		{TEXT("fi"), 0.0f, 30.0f, 10.0f},
		{TEXT("mark"), 10.0f, 40.0f, 10.0f},
		{TEXT("after"), 20.0f, 40.0f, 10.0f},
		{TEXT("side"), 30.0f, 1280.0f, 14.0f},
		{TEXT("fi2"), 34.0f, 50.0f, 10.0f},
		{TEXT("after2"), 44.0f, 40.0f, 20.0f},
	};

	const FBox DetachFieldFocused[] = {
		{TEXT("outer"), 0.0f, 1280.0f, 20.0f},
		{TEXT("side"), 30.0f, 1280.0f, 10.0f},
		{TEXT("dt-wrap"), 50.0f, 1280.0f, 18.0f},
		{TEXT("dt-field"), 58.0f, 50.0f, 10.0f},
	};

	const FBox DetachHostRemoved[] = {
		{TEXT("dt-wrap"), 50.0f, 1280.0f, 4.0f},
	};

	const FBox DetachThenSecondFieldFocused[] = {
		{TEXT("side"), 30.0f, 1280.0f, 14.0f},
		{TEXT("fi2"), 34.0f, 50.0f, 10.0f},
		{TEXT("dt-wrap"), 64.0f, 1280.0f, 0.0f},
	};

	constexpr float Tolerance = 0.5f;

	FString DescribeMismatches(const FArcRmlTestView& View, TConstArrayView<FBox> Boxes)
	{
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!View.ParseLayout(Layout))
		{
			return TEXT("[layout not parsed] ");
		}
		FString Mismatches;
		for (const FBox& Box : Boxes)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Box.DomId);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s missing] "), Box.DomId);
				continue;
			}
			const FArcRmlInspectorRect& Rect = Element->Rect;
			if (FMath::Abs(Rect.Position.X) > Tolerance || FMath::Abs(Rect.Position.Y - Box.Y) > Tolerance || FMath::Abs(Rect.Size.X - Box.Width) > Tolerance
				|| FMath::Abs(Rect.Size.Y - Box.Height) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, expected 0,%.2f %.2fx%.2f] "), Box.DomId, Rect.Position.X, Rect.Position.Y,
					Rect.Size.X, Rect.Size.Y, Box.Y, Box.Width, Box.Height);
			}
		}
		return Mismatches;
	}
}

// Expected boxes come from Chrome on FocusWithinTest (autofocus on #fi) and FocusWithinMovedTest (autofocus on #fi2).
TEST_CLASS(ArcRmlCssParityFocusWithinTests, "ArcRmlUI.CssParity.FocusWithin")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Fixture, const TCHAR* DumpFolder)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), DumpFolder)));
		return View.Flush() && View.Flush();
	}

	bool Click(const TCHAR* Selector)
	{
		View.ClickElement(Selector);
		return View.FlushUntilSettled();
	}

	TEST_METHOD(AutofocusedField_GivesItAndEveryAncestorFocusWithinAsChrome)
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(FocusWithinFixture, TEXT("FocusWithin")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, ChromeFirstFieldFocused)));
	}

	TEST_METHOD(AutofocusedFieldInAnotherBranch_GivesOnlyThatBranchFocusWithinAsChrome)
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(FocusWithinMovedFixture, TEXT("FocusWithinMoved")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, ChromeSecondFieldFocused)));
	}

	TEST_METHOD(FocusMovedToAnotherBranch_MovesFocusWithinAndRestylesAsChrome)
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(FocusWithinFixture, TEXT("FocusWithin")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("#fi2"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fi2")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, ChromeSecondFieldFocused)));
	}

	TEST_METHOD(FocusMovedBack_RestoresFocusWithinOnTheFirstBranchAsChrome)
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(FocusWithinFixture, TEXT("FocusWithin")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("#fi2")) && Click(TEXT("#fi"))));
		ASSERT_THAT(AreEqual(FString(TEXT("fi")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, ChromeFirstFieldFocused)));
	}

	// Removing the focused field's host refocuses its parent (Element::RemoveChild), which then matches as the focused element.
	TEST_METHOD(FocusedFieldRemoved_LeavesFocusWithinOnlyOnTheRefocusedParentChain)
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(FocusWithinFixture, TEXT("FocusWithin")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("#dt-field"))));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, DetachFieldFocused)));
		View.SetModelValue(TEXT("fw"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("showDetach")), ArcRmlUITestHelpers::MakeBool(false));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(FString(TEXT("dt-wrap")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, DetachHostRemoved)));
		ASSERT_THAT(IsTrue(Click(TEXT("#fi2"))));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(View, DetachThenSecondFieldFocused)));
	}
};

TEST_CLASS(ArcRmlFocusNavRangeTests, "ArcRmlUI.FocusNav.Range")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlFocusWithinTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(RangeNavFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool Press(const FKey& Key)
	{
		View.PressKey(Key);
		return View.Flush() && View.Flush();
	}

	bool FocusByClick(const TCHAR* Selector)
	{
		View.ClickElement(Selector);
		return View.FlushUntilSettled();
	}

	TEST_METHOD(BareRange_IsReachedWithDownAndLeftWithDownOnThePad)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetInputType(ECommonInputType::Gamepad);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-top")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-range")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-below")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Gamepad_DPad_Up)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-range")), ArcRmlUITestHelpers::GetNavFocusDomId(View)));
	}

	TEST_METHOD(BareRange_StepsOnRightAndKeepsFocusOnTheKeyboard)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusByClick(TEXT("#rn-top"))));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-range")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Press(EKeys::Right)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-range")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("RangeChanged"), FString())));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-below")), ArcRmlUITestHelpers::GetFocusDomId(View)));
	}

	TEST_METHOD(DisabledRange_IsSkippedByNavigationAndNotFocusedByAClick)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusByClick(TEXT("#rn-off"))));
		ASSERT_THAT(AreNotEqual(FString(TEXT("rn-off")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(FocusByClick(TEXT("#rn-range")) && Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-below")), ArcRmlUITestHelpers::GetFocusDomId(View)));
	}

	TEST_METHOD(AuthorNavNone_KeepsFocusOnTheRange)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FocusByClick(TEXT("#rn-b-top")) && Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-nonav")), ArcRmlUITestHelpers::GetFocusDomId(View)));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("rn-nonav")), ArcRmlUITestHelpers::GetFocusDomId(View)));
	}
};
