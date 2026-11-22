// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityCalcTestsPrivate
{
	enum class ECompare
	{
		Rect,
		Horizontal,
		Size
	};

	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const FBoxCase ChromeTextSizeBoxes[] = {
		{TEXT("t-xs"), 0.0f, 0.0f, 1280.0f, 16.0f},
		{TEXT("t-sm"), 0.0f, 16.0f, 1280.0f, 20.0f},
		{TEXT("t-base"), 0.0f, 36.0f, 1280.0f, 24.0f},
		{TEXT("t-lg"), 0.0f, 60.0f, 1280.0f, 28.0f},
		{TEXT("t-xl"), 0.0f, 88.0f, 1280.0f, 28.0f},
		{TEXT("t-2xl"), 0.0f, 116.0f, 1280.0f, 32.0f},
		{TEXT("t-3xl"), 0.0f, 148.0f, 1280.0f, 36.0f},
		{TEXT("t-4xl"), 0.0f, 184.0f, 1280.0f, 40.0f},
		{TEXT("t-5xl"), 0.0f, 224.0f, 1280.0f, 48.0f},
		{TEXT("t-6xl"), 0.0f, 272.0f, 1280.0f, 60.0f},
		{TEXT("t-7xl"), 0.0f, 332.0f, 1280.0f, 72.0f},
		{TEXT("t-8xl"), 0.0f, 404.0f, 1280.0f, 96.0f},
		{TEXT("t-9xl"), 0.0f, 500.0f, 1280.0f, 128.0f},
	};

	const FBoxCase ChromeLeadingBoxes[] = {
		{TEXT("lead6"), 400.0f, 0.0f, 600.0f, 24.0f},
		{TEXT("t-sm6"), 400.0f, 24.0f, 600.0f, 24.0f},
		{TEXT("lead-sm"), 400.0f, 48.0f, 600.0f, 24.0f},
	};

	const FBoxCase ChromeTextSizeAfterLeadingBoxes[] = {
		{TEXT("lead-late"), 400.0f, 308.0f, 600.0f, 24.0f},
	};

	const FBoxCase ChromeInheritedLineHeightBoxes[] = {
		{TEXT("lh-num"), 400.0f, 72.0f, 600.0f, 30.0f},
		{TEXT("lh-num-child"), 400.0f, 72.0f, 600.0f, 30.0f},
		{TEXT("lh-len"), 400.0f, 102.0f, 600.0f, 14.0f},
		{TEXT("lh-len-child"), 400.0f, 102.0f, 600.0f, 14.0f},
		{TEXT("lh-pct"), 400.0f, 116.0f, 600.0f, 16.0f},
	};

	const FBoxCase ChromeLetterSpacingBoxes[] = {
		{TEXT("tr-none"), 400.0f, 132.0f, 126.47f, 30.0f},
		{TEXT("tr-tight"), 400.0f, 162.0f, 121.47f, 30.0f},
		{TEXT("tr-neg"), 400.0f, 192.0f, 131.47f, 30.0f},
		{TEXT("tr-mixed"), 400.0f, 222.0f, 156.47f, 30.0f},
	};

	const FBoxCase ChromeInheritedLetterSpacingBoxes[] = {
		{TEXT("ls-em-child"), 400.0f, 346.0f, 83.23f, 30.0f},
		{TEXT("ls-calc-child"), 400.0f, 380.0f, 93.23f, 30.0f},
		{TEXT("ls-inh-child"), 400.0f, 410.0f, 83.23f, 30.0f},
		{TEXT("ls-unset-child"), 400.0f, 440.0f, 83.23f, 30.0f},
	};

	const FBoxCase ChromeLetterSpacingAfterFontSizeChangeBoxes[] = {
		{TEXT("ls-em-child"), 400.0f, 349.0f, 93.23f, 30.0f},
		{TEXT("ls-calc-child"), 400.0f, 383.0f, 93.23f, 30.0f},
		{TEXT("ls-inh-child"), 400.0f, 413.0f, 83.23f, 30.0f},
	};

	const FBoxCase ChromeDivideBoxes[] = {
		{TEXT("dx-a"), 400.0f, 252.0f, 52.0f, 10.0f},
		{TEXT("dx-b"), 452.0f, 252.0f, 52.0f, 10.0f},
		{TEXT("dx-c"), 504.0f, 252.0f, 50.0f, 10.0f},
	};

	const FBoxCase ChromeBorderWidthBoxes[] = {
		{TEXT("bw-mixed"), 400.0f, 262.0f, 56.0f, 16.0f},
		{TEXT("bw-neg"), 400.0f, 278.0f, 54.0f, 10.0f},
	};

	const FBoxCase ChromeOrderBoxes[] = {
		{TEXT("ord-b"), 400.0f, 288.0f, 50.0f, 10.0f},
		{TEXT("ord-a"), 450.0f, 288.0f, 50.0f, 10.0f},
		{TEXT("ord-c"), 500.0f, 288.0f, 50.0f, 10.0f},
	};

	const FBoxCase ChromeOrderRoundingBoxes[] = {
		{TEXT("ord2-s"), 400.0f, 298.0f, 50.0f, 10.0f},
		{TEXT("ord2-r"), 450.0f, 298.0f, 50.0f, 10.0f},
		{TEXT("ord2-p"), 500.0f, 298.0f, 50.0f, 10.0f},
		{TEXT("ord2-t"), 550.0f, 298.0f, 50.0f, 10.0f},
	};

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	const FChromePixel ChromePixels[] = {
		{TEXT("z-index calc(1.6) above z-index 1"), 1045, 430, FColor(255, 0, 0, 255)},
		{TEXT("z-index calc(10 * -1) below z-index auto"), 1145, 430, FColor(0, 255, 0, 255)},
		{TEXT("gap before outline-offset calc(0.2em + 2px)"), 1041, 620, FColor(255, 255, 255, 255)},
		{TEXT("outline at offset calc(0.2em + 2px)"), 1045, 620, FColor(255, 0, 0, 255)},
		{TEXT("outline at offset calc(2px * -1)"), 1139, 620, FColor(255, 0, 0, 255)},
		{TEXT("outside outline at offset calc(2px * -1)"), 1141, 620, FColor(255, 255, 255, 255)},
		{TEXT("z-index calc(1.4) below an earlier z-index 2"), 1045, 530, FColor(0, 0, 255, 255)},
		{TEXT("z-index calc(1.5) above a later z-index 1"), 1145, 530, FColor(255, 0, 0, 255)},
		{TEXT("divide-x-2 no inline-start border"), 400, 257, FColor(255, 255, 255, 255)},
		{TEXT("divide-x-2 inline-end border"), 450, 257, FColor(0, 0, 0, 255)},
		{TEXT("divide-x-reverse inline-start border"), 400, 337, FColor(0, 0, 0, 255)},
		{TEXT("divide-x-reverse no inline-end border"), 450, 337, FColor(255, 255, 255, 255)},
	};

	const FChromePixel ChromeInvalidZIndexPixels[] = {
		{TEXT("dropped z-index 1.5 paints below the later sibling"), 145, 130, FColor(0, 0, 255, 255)},
		{TEXT("dropped z-index 1.5 box outside the overlap"), 115, 130, FColor(255, 0, 0, 255)},
	};

	constexpr float Tolerance = 1.0f;
	constexpr int32 PixelTolerance = 2;

	FString DescribeMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FBoxCase> Cases, ECompare Compare)
	{
		FString Failures;
		for (const FBoxCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element)
			{
				Failures += FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
				continue;
			}
			const FArcRmlInspectorRect& Rect = Element->Rect;
			const bool bXMatches = Compare == ECompare::Size || FMath::Abs(Rect.Position.X - Case.X) <= Tolerance;
			const bool bYMatches = Compare != ECompare::Rect || FMath::Abs(Rect.Position.Y - Case.Y) <= Tolerance;
			const bool bWidthMatches = FMath::Abs(Rect.Size.X - Case.Width) <= Tolerance;
			const bool bHeightMatches = Compare == ECompare::Horizontal || FMath::Abs(Rect.Size.Y - Case.Height) <= Tolerance;
			if (!bXMatches || !bYMatches || !bWidthMatches || !bHeightMatches)
			{
				Failures += FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, Chrome %.0f,%.0f %.2fx%.0f] "), Case.Id, Rect.Position.X, Rect.Position.Y,
					Rect.Size.X, Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
			}
		}
		return Failures;
	}

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= PixelTolerance && FMath::Abs(Measured.G - Expected.G) <= PixelTolerance
			&& FMath::Abs(Measured.B - Expected.B) <= PixelTolerance && FMath::Abs(Measured.A - Expected.A) <= PixelTolerance;
	}

	FString FixturePath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/CalcProps/CalcPropsTest.rml"));
	}

	FString InvalidZIndexFixturePath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/CalcProps/ZIndexInvalidTest.rml"));
	}

	FString DescribePixelMismatches(FArcRmlTestView& View, TConstArrayView<FChromePixel> Pixels)
	{
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : Pixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		return FString::Join(Mismatches, TEXT(" | "));
	}

	void DropCompiledFixture()
	{
		FArcRmlTestView::DropCompiledDocument(FixturePath());
		FArcRmlTestView::DropCompiledDocument(InvalidZIndexFixturePath());
		IFileManager::Get().Delete(*FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/CalcProps/CalcProps.rcssc")), false, false, true);
		IFileManager::Get().Delete(*FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/CalcProps/ZIndexInvalid.rcssc")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
	}
}

TEST_CLASS(ArcRmlCssParityCalcTests, "ArcRmlUI.CssParity.Calc")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FixturePath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(CalcDeclarations_AreValidValues)
	{
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(TailwindTextSizeLineHeights_SizeTheLinesAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeTextSizeBoxes, ECompare::Rect);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(LeadingAndTextSizeWithLeading_SizeTheLinesAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeLeadingBoxes, ECompare::Rect);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CalcLineHeights_InheritAsNumberOrLengthAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeInheritedLineHeightBoxes, ECompare::Size);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CalcLetterSpacing_WidensTheTextAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeLetterSpacingBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(InheritedEmAndCalcLetterSpacing_KeepTheParentFontSizeLengthAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeInheritedLetterSpacingBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(DeclaringFontSizeChange_RecomputesTheInheritedLetterSpacingAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		View.SetRootClass(TEXT("ls-big"), true);
		TArray<FArcRmlTestLayoutElement> Changed;
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Changed), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Failures = DescribeMismatches(Changed, ChromeLetterSpacingAfterFontSizeChangeBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(DivideX2CalcBorderWidths_PlaceTheItemsAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeDivideBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(MixedUnitAndNegativeCalcBorderWidths_SizeTheBoxAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeBorderWidthBoxes, ECompare::Size);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CalcOrder_IsRoundedAndOrdersTheItemsAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeOrderBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CalcOrder_RoundsHalvesTowardPositiveInfinityAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeOrderRoundingBoxes, ECompare::Horizontal);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(TextSizeRuleAfterLeading_TakesTheLeadingVariableAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeTextSizeAfterLeadingBoxes, ECompare::Size);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};

TEST_CLASS(ArcRmlCssParityCalcInvalidTests, "ArcRmlUI.CssParity.CalcInvalid")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(NonIntegerPlainZIndex_IsDroppedAsInvalidAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, InvalidZIndexFixturePath()) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlDiagnostic>& Diagnostics = View.GetViewDiagnostics().Diagnostics;
		ASSERT_THAT(AreEqual(1, Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Diagnostics[0].Message.Contains(TEXT("'z-index: 1.5;' is dropped: invalid value")), Diagnostics[0].Message));
	}
};

TEST_CLASS(ArcRmlCssParityCalcPixelTests, "ArcRmlUI.Pixels.Calc")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(CalcZIndexAndOutlineOffset_ReadBackAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FixturePath()) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize, View.GetPixelSize().ToString()));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromePixels)));
	}

	TEST_METHOD(DroppedNonIntegerZIndex_PaintsInTreeOrderAsChrome)
	{
		using namespace ArcRmlCssParityCalcTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, InvalidZIndexFixturePath()) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize, View.GetPixelSize().ToString()));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeInvalidZIndexPixels)));
	}
};
