// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlButtonTestsPrivate
{
	const TCHAR* const LayoutFixture = TEXT("Form/ButtonLayout.rml");
	const TCHAR* const LayoutSheet = TEXT("ButtonLayout.rcssc");
	const TCHAR* const BehaviourFixture = TEXT("Form/ButtonBehaviour.rml");
	const TCHAR* const BehaviourSheet = TEXT("ButtonBehaviour.rcssc");
	const TCHAR* const ModelName = TEXT("bb");

	constexpr float Tolerance = 0.5f;
	// Ascent of the 13.3333 px configured sans-serif: the dump gives a line's baseline, Chrome its top. Chrome's line tops and
	// 17 px line heights equal the fork's at tolerance 0 on this fixture, so top + ascent is Chrome's baseline.
	constexpr float ControlAscent = 13.0f;

	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	// Chrome 156 border boxes of ButtonLayout (rml-forms step 3, VENDORED 224): chrome_layout_compare.py chrome --tol 0 on the
	// 1920x1080 dump with html_sha256 6e4401708f59..., 2026-11-05. Rows are block-level from x 0, so the test view size does not
	// move them.
	const FBoxCase ChromeBoxes[] = {
		{TEXT("bl-bare"), 0.0f, 0.0f, 46.296875f, 23.0f},
		{TEXT("bl-box"), 0.0f, 27.0f, 160.0f, 60.0f},
		{TEXT("bl-ibutton"), 0.0f, 91.0f, 160.0f, 60.0f},
		{TEXT("bl-submit"), 0.0f, 155.0f, 160.0f, 60.0f},
		{TEXT("bl-reset"), 0.0f, 219.0f, 160.0f, 60.0f},
		{TEXT("bl-reset-bare"), 0.0f, 283.0f, 48.40625f, 23.0f},
		{TEXT("bl-svg"), 0.0f, 310.0f, 160.0f, 60.0f},
		{TEXT("bl-overflow"), 0.0f, 374.0f, 52.578125f, 30.0f},
		{TEXT("bl-block"), 0.0f, 433.0f, 160.0f, 60.0f},
		{TEXT("bl-min"), 0.0f, 497.0f, 38.96875f, 50.0f},
		{TEXT("bl-disabled"), 0.0f, 551.0f, 160.0f, 60.0f},
	};

	// Each text line under a button: its text, x and top below the button's border-box top, as Chrome lays them out (same run).
	// The value text of a button input is dom:false in the dump, so Chrome's x for it is the centring of Chrome's own text width:
	// "Save" 30.296875 (the button's), "Send" 32.83 (input type=submit 48.83 wide in FormDefaultsProbe less 16), "Clear" 32.40625
	// (the bare reset 48.40625 less 16).
	struct FLineCase
	{
		const TCHAR* Id;
		int32 Line;
		const TCHAR* Text;
		float X;
		float TopInBox;
	};

	const FLineCase ChromeLines[] = {
		{TEXT("bl-bare"), 0, TEXT("Save"), 8.0f, 3.0f},
		{TEXT("bl-box"), 0, TEXT("Save"), 64.84375f, 21.5f},
		{TEXT("bl-ibutton"), 0, TEXT("Save"), 64.84375f, 21.5f},
		{TEXT("bl-submit"), 0, TEXT("Send"), 63.585f, 21.5f},
		{TEXT("bl-reset"), 0, TEXT("Clear"), 63.796875f, 21.5f},
		{TEXT("bl-reset-bare"), 0, TEXT("Clear"), 8.0f, 3.0f},
		{TEXT("bl-svg"), 0, TEXT(" Go"), 77.765625f, 23.0f},
		{TEXT("bl-overflow"), 0, TEXT("One"), 13.796875f, 3.0f},
		{TEXT("bl-overflow"), 1, TEXT("Two"), 13.484375f, 20.0f},
		{TEXT("bl-overflow"), 2, TEXT("Three"), 8.0f, 37.0f},
		{TEXT("bl-block"), 0, TEXT("Block"), 62.265625f, 21.5f},
		{TEXT("bl-min"), 0, TEXT("Min"), 8.0f, 16.5f},
		{TEXT("bl-disabled"), 0, TEXT("Off"), 70.453125f, 21.5f},
	};

	// Chrome's 16 px inline svg in the 160 x 60 button: 20 below the top, beside " Go".
	const FBoxCase ChromeSvg = {TEXT("bl-svg"), 61.765625f, 330.0f, 16.0f, 16.0f};

	const TCHAR* const InlineFixture = TEXT("Form/ButtonInline.rml");
	const TCHAR* const InlineSheet = TEXT("ButtonInline.rcssc");
	// Ascent of the 16 px body text around the inline buttons, from the fork's dump; the line tops below are Chrome's.
	constexpr float BodyAscent = 16.0f;

	// Chrome 156 border boxes of ButtonInline (review fix of rml-forms step 3, VENDORED 224): chrome_layout_compare.py chrome --tol 0
	// on the 1920x1080 dump with html_sha256 f370a27e6fe1..., 2026-11-06. An inline button is shrink-to-fit (a bare button's 46.30,
	// the submit input's 48.83) and sits on the row's line by the baseline of its last line. Rows start at x 0, so the test view
	// size does not move them. bi-paint is left out: Chrome puts a button without a line 3 px lower (its content-box bottom as
	// baseline), the fork on its bottom margin edge.
	const FBoxCase ChromeInlineBoxes[] = {
		{TEXT("bi-style"), 23.625f, 0.0f, 46.296875f, 23.0f},
		{TEXT("bi-class"), 23.625f, 35.0f, 46.296875f, 23.0f},
		{TEXT("bi-submit"), 23.625f, 70.0f, 48.828125f, 23.0f},
		{TEXT("bi-tall"), 23.625f, 105.0f, 46.296875f, 40.0f},
		{TEXT("bi-outline"), 23.625f, 157.0f, 46.296875f, 23.0f},
	};

	// The row's own text around each inline button: Chrome's x and line top (same run). "ab " and " ef" share one line, so " ef"
	// follows the shrink-to-fit button and the line's baseline is the button's last-line baseline.
	struct FRowTextCase
	{
		const TCHAR* RowId;
		const TCHAR* Text;
		float X;
		float Top;
	};

	const FRowTextCase ChromeInlineRowTexts[] = {
		{TEXT("bi-r-style"), TEXT("ab"), 0.0f, 0.0f},
		{TEXT("bi-r-style"), TEXT("ef"), 69.921875f, 0.0f},
		{TEXT("bi-r-class"), TEXT("ab"), 0.0f, 35.0f},
		{TEXT("bi-r-class"), TEXT("ef"), 69.921875f, 35.0f},
		{TEXT("bi-r-submit"), TEXT("ab"), 0.0f, 70.0f},
		{TEXT("bi-r-submit"), TEXT("ef"), 72.453125f, 70.0f},
		{TEXT("bi-r-tall"), TEXT("ab"), 0.0f, 113.5f},
		{TEXT("bi-r-tall"), TEXT("ef"), 69.921875f, 113.5f},
		{TEXT("bi-r-outline"), TEXT("ab"), 0.0f, 157.0f},
		{TEXT("bi-r-outline"), TEXT("ef"), 69.921875f, 157.0f},
	};

	// The text in each inline button, below its border-box top. "Send" is dom:false in the dump; Chrome's x is the button x plus
	// its 2 px border and 6 px padding, as the button is shrink-to-fit around it. The 40 px button centres its line 11.5 below the
	// top, and that line's baseline (116.5 + 13) is the row's baseline (113.5 + 16).
	const FLineCase ChromeInlineLines[] = {
		{TEXT("bi-style"), 0, TEXT("Save"), 31.625f, 3.0f},
		{TEXT("bi-class"), 0, TEXT("Save"), 31.625f, 3.0f},
		{TEXT("bi-submit"), 0, TEXT("Send"), 31.625f, 3.0f},
		{TEXT("bi-tall"), 0, TEXT("Save"), 31.625f, 11.5f},
		{TEXT("bi-outline"), 0, TEXT("Save"), 31.625f, 3.0f},
	};

	// The two axes are asserted apart: the shrink-to-fit width moves boxes and text along the line, the baseline moves them down.
	enum class EInlineAxis : uint8
	{
		Horizontal,
		Vertical,
	};

	FString DescribeInlineBox(const TArray<FArcRmlTestLayoutElement>& Layout, const FBoxCase& Case, EInlineAxis Axis)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		const bool bMatches = Axis == EInlineAxis::Horizontal
			? FMath::Abs(Rect.Position.X - Case.X) <= Tolerance && FMath::Abs(Rect.Size.X - Case.Width) <= Tolerance
			: FMath::Abs(Rect.Position.Y - Case.Y) <= Tolerance && FMath::Abs(Rect.Size.Y - Case.Height) <= Tolerance;
		if (!bMatches)
		{
			return FString::Printf(TEXT("[%s is %.3f,%.3f %.3fx%.3f, Chrome %.3f,%.3f %.3fx%.3f] "), Case.Id, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
				Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
		}
		return FString();
	}

	FString DescribeRowText(const TArray<FArcRmlTestLayoutElement>& Layout, const FRowTextCase& Case, EInlineAxis Axis)
	{
		const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Layout, Case.RowId);
		if (!Row)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.RowId);
		}
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Kind != TEXT("text") || Element.Parent != Row->Path)
			{
				continue;
			}
			for (const FArcRmlTestTextLine& Line : Element.Lines)
			{
				if (Line.Text.TrimStartAndEnd() != Case.Text)
				{
					continue;
				}
				const float Top = Line.Baseline - BodyAscent;
				const bool bMatches = Axis == EInlineAxis::Horizontal ? FMath::Abs(Line.X - Case.X) <= Tolerance : FMath::Abs(Top - Case.Top) <= Tolerance;
				if (!bMatches)
				{
					return FString::Printf(TEXT("[%s '%s' at x %.3f top %.3f; Chrome x %.3f top %.3f] "), Case.RowId, Case.Text, Line.X, Top, Case.X, Case.Top);
				}
				return FString();
			}
		}
		return FString::Printf(TEXT("[%s has no line '%s'] "), Case.RowId, Case.Text);
	}

	TArray<FArcRmlTestTextLine> GetDescendantLines(const TArray<FArcRmlTestLayoutElement>& Layout, const FString& DomId)
	{
		TArray<FArcRmlTestTextLine> Lines;
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Owner)
		{
			return Lines;
		}
		const FString Prefix = Owner->Path + TEXT(".");
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Kind == TEXT("text") && Element.Path.StartsWith(Prefix))
			{
				Lines.Append(Element.Lines);
			}
		}
		return Lines;
	}

	FString DescribeBox(const TArray<FArcRmlTestLayoutElement>& Layout, const FBoxCase& Case)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Case.X) > Tolerance || FMath::Abs(Rect.Position.Y - Case.Y) > Tolerance
			|| FMath::Abs(Rect.Size.X - Case.Width) > Tolerance || FMath::Abs(Rect.Size.Y - Case.Height) > Tolerance)
		{
			return FString::Printf(TEXT("[%s is %.3f,%.3f %.3fx%.3f, Chrome %.3f,%.3f %.3fx%.3f] "), Case.Id, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
				Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
		}
		return FString();
	}

	FString DescribeLine(const TArray<FArcRmlTestLayoutElement>& Layout, const FLineCase& Case)
	{
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		const TArray<FArcRmlTestTextLine> Lines = GetDescendantLines(Layout, Case.Id);
		if (!Owner || !Lines.IsValidIndex(Case.Line))
		{
			return FString::Printf(TEXT("[%s has no line %d] "), Case.Id, Case.Line);
		}
		const FArcRmlTestTextLine& Line = Lines[Case.Line];
		const float TopInBox = Line.Baseline - ControlAscent - Owner->Rect.Position.Y;
		if (Line.Text.TrimStartAndEnd() != FString(Case.Text).TrimStartAndEnd() || FMath::Abs(Line.X - Case.X) > Tolerance
			|| FMath::Abs(TopInBox - Case.TopInBox) > Tolerance)
		{
			return FString::Printf(TEXT("[%s line %d '%s' at x %.3f, %.3f below the top; Chrome '%s' at x %.3f, %.3f below] "), Case.Id, Case.Line, *Line.Text,
				Line.X, TopInBox, Case.Text, Case.X, Case.TopInBox);
		}
		return FString();
	}

	FString GetDumpDir(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), Name));
	}

	class FLayoutVerifyLogCapture final : public FOutputDevice
	{
	public:
		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category != FName(TEXT("LogArcRmlUI")) || !FCString::Strstr(Text, TEXT("layout verify")))
			{
				return;
			}
			FScopeLock Lock(&Mutex);
			if (!FCString::Strstr(Text, TEXT("mismatches 0,")))
			{
				Lines.Add(Text);
			}
			else if (FCString::Strstr(Text, TEXT("layout verify '")))
			{
				CleanPasses++;
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		FString Describe()
		{
			FScopeLock Lock(&Mutex);
			return FString::Join(Lines, TEXT(" | "));
		}

		int32 Num()
		{
			FScopeLock Lock(&Mutex);
			return Lines.Num();
		}

		int32 NumCleanPasses()
		{
			FScopeLock Lock(&Mutex);
			return CleanPasses;
		}

	private:
		FCriticalSection Mutex;
		TArray<FString> Lines;
		int32 CleanPasses = 0;
	};
}

// Button layout against Chrome (rml-forms step 3, VENDORED 224): the boxes and the centred content of ButtonLayout.
TEST_CLASS(ArcRmlButtonLayoutTests, "ArcRmlUI.Form.ButtonLayout")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlButtonTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(LayoutFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), LayoutSheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		// The dump is the input of chrome_layout_compare.py chrome --tol 0.5, which gave the expected values.
		View.DumpLayout(GetDumpDir(TEXT("ButtonLayout")));
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	TEST_METHOD(EveryButtonBox_IsChromes)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FBoxCase& Case : ChromeBoxes)
		{
			Mismatches += DescribeBox(Layout, Case);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("each button and button input border box is Chrome's (bare button 46.30 x 23, bare reset 48.41 x 23)")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(TextInATallButtonOrButtonInput_IsCentredAsChrome)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FLineCase& Case : ChromeLines)
		{
			if (FCString::Strcmp(Case.Id, TEXT("bl-overflow")) != 0 && FCString::Strcmp(Case.Id, TEXT("bl-svg")) != 0)
			{
				Mismatches += DescribeLine(Layout, Case);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("a button, input type=button|submit|reset, display:block and min-height button centre their one line ('Save' at x 64.84, 21.5 below the top of a 160 x 60 box)")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(InlineSvgAndTextInATallButton_AreCentredAsChrome)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Button = ArcRmlUITestHelpers::FindByDomId(Layout, ChromeSvg.Id);
		ASSERT_THAT(IsNotNull(Button, TEXT("bl-svg is not in the layout")));
		const FArcRmlTestLayoutElement* Svg = Layout.FindByPredicate([Button](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == Button->Path && Element.Tag == TEXT("svg");
		});
		ASSERT_THAT(IsNotNull(Svg, TEXT("bl-svg has no svg child")));
		FString Mismatches;
		if (FMath::Abs(Svg->Rect.Position.X - ChromeSvg.X) > Tolerance || FMath::Abs(Svg->Rect.Position.Y - ChromeSvg.Y) > Tolerance
			|| FMath::Abs(Svg->Rect.Size.X - ChromeSvg.Width) > Tolerance || FMath::Abs(Svg->Rect.Size.Y - ChromeSvg.Height) > Tolerance)
		{
			Mismatches += FString::Printf(TEXT("[svg is %.3f,%.3f %.3fx%.3f, Chrome %.3f,%.3f 16x16] "), Svg->Rect.Position.X, Svg->Rect.Position.Y, Svg->Rect.Size.X,
				Svg->Rect.Size.Y, ChromeSvg.X, ChromeSvg.Y);
		}
		for (const FLineCase& Case : ChromeLines)
		{
			if (FCString::Strcmp(Case.Id, TEXT("bl-svg")) == 0)
			{
				Mismatches += DescribeLine(Layout, Case);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("the 16 px inline svg sits 20 below the button top and ' Go' beside it, as Chrome")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(OverflowingButtonContent_StartsAtTheTopAsChrome)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FLineCase& Case : ChromeLines)
		{
			if (FCString::Strcmp(Case.Id, TEXT("bl-overflow")) == 0)
			{
				Mismatches += DescribeLine(Layout, Case);
			}
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("three lines in a 30 px button start 3 below its top (border and padding), not shifted up")) + TEXT(": ") + Mismatches));
	}
};

// Button behaviour (rml-forms step 3, VENDORED 224): a button is a form control (focusable, :disabled, skipped when disabled) and
// keeps its handlers.
TEST_CLASS(ArcRmlButtonBehaviourTests, "ArcRmlUI.Form.Button")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlButtonTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(BehaviourFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), BehaviourSheet), false, true, true);
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout);
	}

	bool Click(const TCHAR* DomId)
	{
		View.ParseLayout(Layout);
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		if (!Element)
		{
			return false;
		}
		View.MouseClick(Element->Rect.Position + Element->Rect.Size * 0.5f);
		return View.FlushUntilSettled() && View.Flush();
	}

	bool Press(const FKey& Key)
	{
		View.PressKey(Key);
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	FString GetColour(const TCHAR* DomId)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, DomId);
		return Element ? Element->Color : FString(TEXT("<missing>"));
	}

	TEST_METHOD(BareButton_IsFocusedByAClickAndTab)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("bb-first"))));
		ASSERT_THAT(AreEqual(FString(TEXT("bb-first")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("a click focuses a bare button")));
		ASSERT_THAT(IsTrue(Press(EKeys::Tab)));
		ASSERT_THAT(AreEqual(FString(TEXT("bb-last")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("Tab moves to the next enabled button, past the disabled one")));
	}

	TEST_METHOD(BareButton_IsANavStopAndTheDisabledOneIsSkipped)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("bb-first"))));
		ASSERT_THAT(IsTrue(Press(EKeys::Down)));
		ASSERT_THAT(AreEqual(FString(TEXT("bb-last")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("Down from a button reaches the button below the disabled one")));
		ASSERT_THAT(IsTrue(Press(EKeys::Up)));
		ASSERT_THAT(AreEqual(FString(TEXT("bb-first")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("Up skips the disabled button back to the first")));
	}

	TEST_METHOD(DisabledButton_MatchesDisabledAndIsNotFocusedByAClick)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("#ff0000ff")), GetColour(TEXT("bb-off")), TEXT("button:disabled matches the disabled button")));
		ASSERT_THAT(AreEqual(FString(TEXT("#000000ff")), GetColour(TEXT("bb-first")), TEXT("button:disabled does not match an enabled button")));
		ASSERT_THAT(IsTrue(Click(TEXT("bb-off"))));
		ASSERT_THAT(AreNotEqual(FString(TEXT("bb-off")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("a click does not focus a disabled button")));
	}

	TEST_METHOD(FocusedButtonDisabledByABinding_LosesFocusAndMatchesDisabled)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Click(TEXT("bb-last"))));
		ASSERT_THAT(AreEqual(FString(TEXT("bb-last")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("a click focuses the enabled bb-last")));
		ASSERT_THAT(AreEqual(FString(TEXT("#000000ff")), GetColour(TEXT("bb-last"))));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("lastOff")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.Flush() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreNotEqual(FString(TEXT("bb-last")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("disabling the focused button blurs it")));
		ASSERT_THAT(AreEqual(FString(TEXT("#ff0000ff")), GetColour(TEXT("bb-last")), TEXT("the bound disabled attribute sets :disabled")));
	}

	TEST_METHOD(ClickOnAButton_RunsItsDataEventClick)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearCalls();
		ASSERT_THAT(IsTrue(Click(TEXT("bb-first"))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountCalls(View, TEXT("Pressed"), TEXT("bb-first")),
			FString::Printf(TEXT("%d calls"), View.GetCalls().Num())));
	}
};

// Layout reuse and the layout verify reference keep a button's centring offset (rml-forms step 3, VENDORED 224).
TEST_CLASS(ArcRmlButtonLayoutReuseTests, "ArcRmlUI.LayoutVerify.Button")
{
	FArcRmlTestView View;
	ArcRmlButtonTestsPrivate::FLayoutVerifyLogCapture Capture;
	TArray<FArcRmlTestLayoutElement> Layout;
	int32 SavedVerify = 0;
	bool bCaptureAttached = false;

	BEFORE_EACH()
	{
		IConsoleVariable* Verify = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"));
		ASSERT_THAT(IsNotNull(Verify));
		SavedVerify = Verify->GetInt();
		Verify->Set(1, ECVF_SetByCode);
		GLog->AddOutputDevice(&Capture);
		bCaptureAttached = true;
	}

	AFTER_EACH()
	{
		View.Close();
		if (bCaptureAttached)
		{
			GLog->RemoveOutputDevice(&Capture);
			bCaptureAttached = false;
		}
		IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.LayoutVerify"))->Set(SavedVerify, ECVF_SetByCode);
	}

	// The text of bb-last ("Last") in its 160 x 60 box: x centred on its own width, top 21.5 below the box top as Chrome's 'Save'.
	FString DescribeCentred()
	{
		using namespace ArcRmlButtonTestsPrivate;
		const FArcRmlTestLayoutElement* Button = ArcRmlUITestHelpers::FindByDomId(Layout, TEXT("bb-last"));
		const TArray<FArcRmlTestTextLine> Lines = GetDescendantLines(Layout, TEXT("bb-last"));
		if (!Button || Lines.IsEmpty())
		{
			return TEXT("[bb-last or its line is missing] ");
		}
		const float X = Lines[0].X - Button->Rect.Position.X;
		const float Top = Lines[0].Baseline - ControlAscent - Button->Rect.Position.Y;
		const float ExpectedX = (160.0f - Lines[0].Width) * 0.5f;
		if (FMath::Abs(X - ExpectedX) > Tolerance || FMath::Abs(Top - 21.5f) > Tolerance)
		{
			return FString::Printf(TEXT("[bb-last 'Last' at x %.3f, %.3f below the top; expected x %.3f, 21.5 below] "), X, Top, ExpectedX);
		}
		return FString();
	}

	TEST_METHOD(SiblingTextChange_ReusesTheButtonAndKeepsItsCentredText)
	{
		using namespace ArcRmlButtonTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(BehaviourFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), BehaviourSheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeCentred(), TEXT("after open")));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("label")), ArcRmlUITestHelpers::MakeString(TEXT("a longer label after the change")));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(TEXT("a longer label after the change")), ArcRmlUITestHelpers::GetText(Layout, TEXT("bb-label")), TEXT("the label changed")));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > CleanBefore, TEXT("no layout verify pass ran after the label change")));
		// bb-last runs the centring format and keeps its layout; the auto-width bb-first and bb-off may reuse too, as their
		// shrink-to-fit widths are kept across passes.
		const int64 Reused = View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused);
		ASSERT_THAT(IsTrue(Reused >= 1, TEXT("bb-last did not keep its layout")));
		ASSERT_THAT(AreEqual(FString(), DescribeCentred(), TEXT("after the re-layout")));
	}
};

// An inline button against Chrome (review fix of rml-forms step 3, VENDORED 224): a button or button input with display: inline is
// an atomic inline, shrink-to-fit on the row's line and aligned by the baseline of its last line.
TEST_CLASS(ArcRmlButtonInlineTests, "ArcRmlUI.Form.ButtonInline")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlButtonTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(InlineFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), InlineSheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		// The dump is the input of chrome_layout_compare.py chrome --tol 0, which gave the expected values.
		View.DumpLayout(GetDumpDir(TEXT("ButtonInline")));
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	TEST_METHOD(InlineButton_IsShrinkToFitAndTheRowTextFollowsItAsChrome)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FBoxCase& Case : ChromeInlineBoxes)
		{
			Mismatches += DescribeInlineBox(Layout, Case, EInlineAxis::Horizontal);
		}
		for (const FRowTextCase& Case : ChromeInlineRowTexts)
		{
			Mismatches += DescribeRowText(Layout, Case, EInlineAxis::Horizontal);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("an inline button is 46.30 wide (the submit 48.83) at x 23.63 and ' ef' follows it at x 69.92, as Chrome")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(InlineButton_SitsOnTheLineByItsLastLineBaselineAsChrome)
	{
		using namespace ArcRmlButtonTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FString Mismatches;
		for (const FBoxCase& Case : ChromeInlineBoxes)
		{
			Mismatches += DescribeInlineBox(Layout, Case, EInlineAxis::Vertical);
		}
		for (const FRowTextCase& Case : ChromeInlineRowTexts)
		{
			Mismatches += DescribeRowText(Layout, Case, EInlineAxis::Vertical);
		}
		for (const FLineCase& Case : ChromeInlineLines)
		{
			Mismatches += DescribeLine(Layout, Case);
		}
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("each inline button's top and the row text's line top are Chrome's (the row text at the row top, the 40 px button's centred text on the row baseline), the button text 3 below its top (11.5 in the 40 px one)")) + TEXT(": ") + Mismatches));
	}
};

// Inline button paint against Chrome's screenshot of ButtonInline (same run, chrome --screenshot --pixel): the outline goes round
// the whole shrink-to-fit box, and the block child of an inline button paints over the button's background, because the button is
// painted as an atomic unit and not in the inline order that hoists the block child under its parent's inline content.
TEST_CLASS(ArcRmlButtonInlinePixelTests, "ArcRmlUI.Pixels.ButtonInline")
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

	TEST_METHOD(InlineButtonOutlineAndBlockChild_MatchChromePixels)
	{
		using namespace ArcRmlButtonTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(InlineFixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), InlineSheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));

		const FColor Ring(224, 0, 224, 255);
		const FColor Inner(32, 64, 255, 255);
		const FColor Background(0, 192, 0, 255);
		// Points at least 1 px inside each paint in both renderings: the 3 px outline of bi-outline (Chrome ink 21,154 52x29), the
		// 20 x 10 block child of bi-paint and the button's background left of it (bi-paint sits 3 px higher in the fork, see above).
		const FChromePixel Expected[] = {
			{TEXT("outline left"), 21, 168, Ring},
			{TEXT("outline right"), 71, 168, Ring},
			{TEXT("outline top"), 45, 155, Ring},
			{TEXT("outline bottom"), 45, 181, Ring},
			{TEXT("block child"), 41, 201, Inner},
			{TEXT("button background"), 28, 201, Background},
		};
		constexpr int32 PixelTolerance = 2;
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
