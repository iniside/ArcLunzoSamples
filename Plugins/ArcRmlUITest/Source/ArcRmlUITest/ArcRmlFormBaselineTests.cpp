// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

namespace ArcRmlFormBaselineTestsPrivate
{
	const TCHAR* const Fixture = TEXT("Form/FormBaseline.rml");
	const TCHAR* const Sheet = TEXT("FormBaseline.rcssc");
	const TCHAR* const ModelName = TEXT("fb");

	constexpr float Tolerance = 0.5f;
	// Ascent of the 16 px body text of the rows, from the fork's dump: the dump gives a line's baseline, Chrome its top.
	constexpr float BodyAscent = 16.0f;

	struct FBoxCase
	{
		const TCHAR* Id;
		float Y;
		float Height;
	};

	struct FRowTextCase
	{
		const TCHAR* RowId;
		const TCHAR* Text;
		float Top;
	};

	// Chrome 156 boxes and line tops of FormBaseline (rml-forms step 5, VENDORED 226): chrome_layout_compare.py chrome --tol 0 on
	// the test's dump, html_sha256 d059e7ec6ded49b8524460e9885ead66ddf59bd4bdec6ce15d03015eacec514f, 2026-11-06 (every rect.y,
	// rect.h and line top equal; only sub-pixel x and widths and the scroll heights differ). Rows are block-level from x 0, so only
	// the vertical axis is asserted; the control widths are step 4's (ArcRmlUI.Form.Sizes). A row is 4 px below the last one.
	const FBoxCase ChromeTextBoxes[] = {
		{TEXT("fb-line-text"), 0.0f, 23.0f},
		{TEXT("fb-text"), 0.0f, 23.0f},
		{TEXT("fb-line-value"), 27.0f, 23.0f},
		{TEXT("fb-value"), 27.0f, 23.0f},
		// The select's 1 px of inner padding sits above the line: the row is 22, the select 1 below its top.
		{TEXT("fb-line-select"), 54.0f, 22.0f},
		{TEXT("fb-select"), 55.0f, 21.0f},
		{TEXT("fb-line-16"), 210.0f, 27.0f},
		{TEXT("fb-text-16"), 210.0f, 27.0f},
		{TEXT("fb-select-16"), 211.0f, 25.0f},
		// Author line-height 30px on the input: Chrome's field is 36 high and its text line centred on the row's baseline.
		{TEXT("fb-line-lh"), 294.0f, 36.0f},
		{TEXT("fb-text-lh"), 294.0f, 36.0f},
	};
	const FRowTextCase ChromeTextLines[] = {
		{TEXT("fb-line-text"), TEXT("ab"), 0.0f},
		{TEXT("fb-line-text"), TEXT("ef"), 0.0f},
		{TEXT("fb-line-value"), TEXT("ab"), 27.0f},
		{TEXT("fb-line-value"), TEXT("ef"), 27.0f},
		{TEXT("fb-line-select"), TEXT("ab"), 54.0f},
		{TEXT("fb-line-select"), TEXT("ef"), 54.0f},
		{TEXT("fb-line-16"), TEXT("ab"), 213.0f},
		{TEXT("fb-line-16"), TEXT("ef"), 213.0f},
		{TEXT("fb-line-lh"), TEXT("ab"), 300.0f},
		{TEXT("fb-line-lh"), TEXT("ef"), 300.0f},
	};
	// appearance: auto puts the border-box bottom on the baseline, so the 3 px bottom margin hangs below it (row 21); the explicit
	// 13 x 13 appearance: none checkbox (.plain) sits on its bottom margin edge (row 24). The radio has no bottom margin.
	const FBoxCase ChromeCheckBoxes[] = {
		{TEXT("fb-line-checkbox"), 185.0f, 21.0f},
		{TEXT("fb-checkbox"), 188.0f, 13.0f},
		{TEXT("fb-line-checkbox-none"), 241.0f, 24.0f},
		{TEXT("fb-checkbox-none"), 244.0f, 13.0f},
		{TEXT("fb-line-radio"), 269.0f, 21.0f},
		{TEXT("fb-radio"), 272.0f, 13.0f},
	};
	const FRowTextCase ChromeCheckLines[] = {
		{TEXT("fb-line-checkbox"), TEXT("ab"), 185.0f},
		{TEXT("fb-line-checkbox"), TEXT("ef"), 185.0f},
		{TEXT("fb-line-checkbox-none"), TEXT("ab"), 244.0f},
		{TEXT("fb-line-checkbox-none"), TEXT("ef"), 244.0f},
		{TEXT("fb-line-radio"), TEXT("ab"), 269.0f},
		{TEXT("fb-line-radio"), TEXT("ef"), 269.0f},
	};
	// Not changed by step 5: button and submit sit on their text's baseline, the textarea on its bottom margin edge.
	const FBoxCase ChromeOtherBoxes[] = {
		{TEXT("fb-line-button"), 80.0f, 23.0f},
		{TEXT("fb-button"), 80.0f, 23.0f},
		{TEXT("fb-line-submit"), 107.0f, 23.0f},
		{TEXT("fb-submit"), 107.0f, 23.0f},
		{TEXT("fb-line-textarea"), 134.0f, 47.0f},
		{TEXT("fb-ta"), 134.0f, 42.0f},
	};
	const FRowTextCase ChromeOtherLines[] = {
		{TEXT("fb-line-button"), TEXT("ab"), 80.0f},
		{TEXT("fb-line-button"), TEXT("ef"), 80.0f},
		{TEXT("fb-line-submit"), TEXT("ab"), 107.0f},
		{TEXT("fb-line-submit"), TEXT("ef"), 107.0f},
		{TEXT("fb-line-textarea"), TEXT("ab"), 160.0f},
		{TEXT("fb-line-textarea"), TEXT("ef"), 160.0f},
	};
	// The row whose bound label changes in ArcRmlUI.LayoutVerify.FormBaseline: a text input, a select and a checkbox on one line
	// before the label span. Chrome measured the loaded label "short"; the changed label stays on that one line after the controls,
	// so their tops and the row height do not move.
	const FBoxCase ChromeLabelBoxes[] = {
		{TEXT("fb-line-label"), 334.0f, 23.0f},
		{TEXT("fb-label-text"), 334.0f, 23.0f},
		{TEXT("fb-label-select"), 335.0f, 21.0f},
		{TEXT("fb-label-checkbox"), 337.0f, 13.0f},
		{TEXT("fb-label"), 334.0f, 21.0f},
	};
	const FRowTextCase ChromeLabelLines[] = {
		{TEXT("fb-line-label"), TEXT("ab"), 334.0f},
	};

	// Radios with a 3 px bottom margin (.mb3), which the user-agent radio margin (3px 3px 0 5px) lacks, so the border-box bottom
	// and the bottom margin edge differ: under appearance: auto Chrome puts the border-box bottom on the baseline and the margin
	// hangs below it (row 21, as the checkbox); the 13 x 13 appearance: none radio (.plain) sits on its bottom margin edge (row 24).
	// Chrome 156, chrome_layout_compare.py chrome --tol 0 on the test's dump, html_sha256
	// 748c23502083ea1053e9d0bad0c384d25e8ef387a1fcb1af4151f0f68009e74c, 2026-11-06.
	const FBoxCase ChromeRadioBoxes[] = {
		{TEXT("fb-line-radio-mb"), 361.0f, 21.0f},
		{TEXT("fb-radio-mb"), 364.0f, 13.0f},
		{TEXT("fb-line-radio-none"), 386.0f, 24.0f},
		{TEXT("fb-radio-none"), 389.0f, 13.0f},
	};
	const FRowTextCase ChromeRadioLines[] = {
		{TEXT("fb-line-radio-mb"), TEXT("ab"), 361.0f},
		{TEXT("fb-line-radio-mb"), TEXT("ef"), 361.0f},
		{TEXT("fb-line-radio-none"), TEXT("ab"), 389.0f},
		{TEXT("fb-line-radio-none"), TEXT("ef"), 389.0f},
	};
	// The row whose bound tail changes in the reuse tests: a 300 px wide inline-block holding "cd <input> ef", then the tail span.
	// The inline-block sits on the row by the baseline of its line, which the input's text baseline sets, so the baseline it
	// keeps on its element (Element::GetBaseline, which a reused box does not rewrite) is what places it on the line. Loaded: same dump and hash as ChromeRadioBoxes. Changed tail ("a longer
	// tail after the change"): chrome --tol 0 on the FormBaselineChanged dump, html_sha256
	// 8c9ce4e4a1b798f895fab7419e8a1a28a23bc79854bf8b3ef7bca26431022cfb, 2026-11-06, every y, height and line top as loaded.
	const FBoxCase ChromeReuseBoxes[] = {
		{TEXT("fb-line-reuse"), 414.0f, 23.0f},
		{TEXT("fb-reuse-box"), 414.0f, 23.0f},
		{TEXT("fb-reuse-text"), 414.0f, 23.0f},
		{TEXT("fb-tail"), 414.0f, 21.0f},
	};
	const FRowTextCase ChromeReuseLines[] = {
		{TEXT("fb-line-reuse"), TEXT("ab"), 414.0f},
		{TEXT("fb-reuse-box"), TEXT("cd"), 414.0f},
		{TEXT("fb-reuse-box"), TEXT("ef"), 414.0f},
	};
	const TCHAR* const ChangedTail = TEXT("a longer tail after the change");
	// Ranges sit on the line by their border-box bottom under appearance: auto and none, so the bottom margin hangs below the
	// baseline: the bare range (user-agent margin 2px), one with margin 6px, one 30 px high, and the appearance: none .slider
	// (240 x 6, thumb 16 px) bare and with margin 6px. Chrome 156, chrome_layout_compare.py chrome --tol 0 on the test's dump,
	// html_sha256 7e84276b8ecacdc3310a13362e3423287b4e01e02bc7b5ae3836ed6764225c24, 2026-11-06.
	const FBoxCase ChromeRangeBoxes[] = {
		{TEXT("fb-line-range"), 441.0f, 23.0f},
		{TEXT("fb-range"), 443.0f, 16.0f},
		{TEXT("fb-line-range-margin"), 468.0f, 28.0f},
		{TEXT("fb-range-margin"), 474.0f, 16.0f},
		{TEXT("fb-line-range-tall"), 500.0f, 37.0f},
		{TEXT("fb-range-tall"), 502.0f, 30.0f},
		{TEXT("fb-line-range-none"), 541.0f, 21.0f},
		{TEXT("fb-range-none"), 551.0f, 6.0f},
		{TEXT("fb-line-range-none-margin"), 566.0f, 22.0f},
		{TEXT("fb-range-none-margin"), 576.0f, 6.0f},
	};
	const FRowTextCase ChromeRangeLines[] = {
		{TEXT("fb-line-range"), TEXT("ab"), 443.0f},
		{TEXT("fb-line-range"), TEXT("ef"), 443.0f},
		{TEXT("fb-line-range-margin"), TEXT("ab"), 474.0f},
		{TEXT("fb-line-range-margin"), TEXT("ef"), 474.0f},
		{TEXT("fb-line-range-tall"), TEXT("ab"), 516.0f},
		{TEXT("fb-line-range-tall"), TEXT("ef"), 516.0f},
		{TEXT("fb-line-range-none"), TEXT("ab"), 541.0f},
		{TEXT("fb-line-range-none"), TEXT("ef"), 541.0f},
		{TEXT("fb-line-range-none-margin"), TEXT("ab"), 566.0f},
		{TEXT("fb-line-range-none-margin"), TEXT("ef"), 566.0f},
	};

	FString DescribeBoxes(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FBoxCase> Cases)
	{
		FString Mismatches;
		for (const FBoxCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
			}
			else if (FMath::Abs(Element->Rect.Position.Y - Case.Y) > Tolerance || FMath::Abs(Element->Rect.Size.Y - Case.Height) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s y %.3f h %.3f, Chrome y %g h %g] "), Case.Id, Element->Rect.Position.Y, Element->Rect.Size.Y, Case.Y, Case.Height);
			}
		}
		return Mismatches;
	}

	// The row's own text ("ab", "ef"): the top of its line, its baseline less the body ascent.
	FString DescribeRowTexts(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FRowTextCase> Cases)
	{
		FString Mismatches;
		for (const FRowTextCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Row = ArcRmlUITestHelpers::FindByDomId(Layout, Case.RowId);
			if (!Row)
			{
				Mismatches += FString::Printf(TEXT("[%s is not in the layout] "), Case.RowId);
				continue;
			}
			const FArcRmlTestTextLine* Found = nullptr;
			for (const FArcRmlTestLayoutElement& Element : Layout)
			{
				if (Element.Kind != TEXT("text") || Element.Parent != Row->Path)
				{
					continue;
				}
				for (const FArcRmlTestTextLine& Line : Element.Lines)
				{
					if (Line.Text.TrimStartAndEnd() == Case.Text)
					{
						Found = &Line;
						break;
					}
				}
				if (Found)
				{
					break;
				}
			}
			if (!Found)
			{
				Mismatches += FString::Printf(TEXT("[%s has no line '%s'] "), Case.RowId, Case.Text);
			}
			else if (FMath::Abs(Found->Baseline - BodyAscent - Case.Top) > Tolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s '%s' top %.3f, Chrome %g] "), Case.RowId, Case.Text, Found->Baseline - BodyAscent, Case.Top);
			}
		}
		return Mismatches;
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

// Form controls on the text baseline (rml-forms step 5, VENDORED 226): each FormBaseline row's height, its control's top and the
// row's "ab" / "ef" line tops against Chrome's.
TEST_CLASS(ArcRmlFormBaselineTests, "ArcRmlUI.Form.Baseline")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument))
		{
			return false;
		}
		// The dump is the input of chrome_layout_compare.py chrome --tol 0, which gave the expected values.
		View.DumpLayout(GetDumpDir(TEXT("FormBaseline")));
		return View.Flush() && View.Flush() && View.ParseLayout(Layout);
	}

	TEST_METHOD(TextInputAndSelect_SitOnTheirTextBaselineAsChrome)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Mismatches = DescribeBoxes(Layout, ChromeTextBoxes) + DescribeRowTexts(Layout, ChromeTextLines);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("text input and select rows are Chrome's height with the control and the row text at Chrome's tops")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(Checkbox_SitsOnItsBorderBoxBottomUnderAutoAndItsMarginEdgeUnderNone)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Mismatches = DescribeBoxes(Layout, ChromeCheckBoxes) + DescribeRowTexts(Layout, ChromeCheckLines);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("checkbox and radio rows are Chrome's height with the control and the row text at Chrome's tops")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(Radio_SitsOnItsBorderBoxBottomUnderAutoAndItsMarginEdgeUnderNone)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Mismatches = DescribeBoxes(Layout, ChromeRadioBoxes) + DescribeRowTexts(Layout, ChromeRadioLines);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("radio rows with a bottom margin are Chrome's height with the radio and the row text at Chrome's tops")) + TEXT(": ") + Mismatches));
	}

	// The tail change re-lays out the row and reuses the inline-block's recorded layout; the changed state is dumped too
	// (FormBaselineChanged), the input of the Chrome check of that state.
	TEST_METHOD(InlineBlockAroundAnInput_IsReusedAndStaysOnTheBaselineAsChrome)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeReuseBoxes) + DescribeRowTexts(Layout, ChromeReuseLines), TEXT("after open")));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("tail")), ArcRmlUITestHelpers::MakeString(ChangedTail));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		const int64 Reused = View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused);
		View.DumpLayout(GetDumpDir(TEXT("FormBaselineChanged")));
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(ChangedTail), ArcRmlUITestHelpers::GetText(Layout, TEXT("fb-tail")), TEXT("the tail changed")));
		ASSERT_THAT(IsTrue(Reused >= 1, TEXT("fb-reuse-box did not keep its layout")));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeReuseBoxes) + DescribeRowTexts(Layout, ChromeReuseLines), TEXT("after the tail change")));
	}

	TEST_METHOD(Range_SitsOnItsBorderBoxBottomUnderAutoAndNone)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Mismatches = DescribeBoxes(Layout, ChromeRangeBoxes) + DescribeRowTexts(Layout, ChromeRangeLines);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("range rows are Chrome's height with the range and the row text at Chrome's tops")) + TEXT(": ") + Mismatches));
	}

	TEST_METHOD(ButtonSubmitAndTextArea_KeepTheirBaselineAsChrome)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Mismatches = DescribeBoxes(Layout, ChromeOtherBoxes) + DescribeRowTexts(Layout, ChromeOtherLines);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), FString(TEXT("button, submit and textarea rows are Chrome's height with the control and the row text at Chrome's tops")) + TEXT(": ") + Mismatches));
	}
};

// Layout reuse and the layout verify reference keep the controls' baselines when a sibling text on their line changes (rml-forms
// step 5, VENDORED 226).
TEST_CLASS(ArcRmlFormBaselineReuseTests, "ArcRmlUI.LayoutVerify.FormBaseline")
{
	FArcRmlTestView View;
	ArcRmlFormBaselineTestsPrivate::FLayoutVerifyLogCapture Capture;
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

	TEST_METHOD(SiblingTextChange_VerifiesAndKeepsTheControlsOnTheBaseline)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeLabelBoxes) + DescribeRowTexts(Layout, ChromeLabelLines) + DescribeBoxes(Layout, ChromeTextBoxes), TEXT("after open")));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		const FString Changed(TEXT("a longer label after the change"));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("label")), ArcRmlUITestHelpers::MakeString(Changed));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(Changed, ArcRmlUITestHelpers::GetText(Layout, TEXT("fb-label")), TEXT("the label changed")));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > CleanBefore, TEXT("no layout verify pass ran after the label change")));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeLabelBoxes) + DescribeRowTexts(Layout, ChromeLabelLines) + DescribeBoxes(Layout, ChromeTextBoxes), TEXT("after the label change")));
	}

	// The tail change outside the inline-block reuses it; the reference layout formats it fresh, so a reused baseline that
	// differs from a fresh one shows as a mismatch, and the reused box must still sit at Chrome's tops.
	TEST_METHOD(SiblingTextChange_ReusesTheInlineBlockAroundAnInputAndVerifies)
	{
		using namespace ArcRmlFormBaselineTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), Sheet), false, true, true);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeReuseBoxes) + DescribeRowTexts(Layout, ChromeReuseLines), TEXT("after open")));
		GLog->Flush();
		const int32 CleanBefore = Capture.NumCleanPasses();
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		View.SetModelValue(ModelName, ArcRmlUITestHelpers::MakeFieldPath(TEXT("tail")), ArcRmlUITestHelpers::MakeString(ChangedTail));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(ChangedTail), ArcRmlUITestHelpers::GetText(Layout, TEXT("fb-tail")), TEXT("the tail changed")));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, Capture.Num(), FString::Printf(TEXT("layout verify reported %s"), *Capture.Describe())));
		ASSERT_THAT(IsTrue(Capture.NumCleanPasses() > CleanBefore, TEXT("no layout verify pass ran after the tail change")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::LayoutReused) >= 1, TEXT("fb-reuse-box did not keep its layout")));
		ASSERT_THAT(AreEqual(FString(), DescribeBoxes(Layout, ChromeReuseBoxes) + DescribeRowTexts(Layout, ChromeReuseLines), TEXT("after the tail change")));
	}
};
