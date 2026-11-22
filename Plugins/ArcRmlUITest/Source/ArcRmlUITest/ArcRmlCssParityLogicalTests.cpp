// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityLogicalTestsPrivate
{
	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const FBoxCase ChromePaddingBoxes[] = {
		{TEXT("px"), 0.0f, 0.0f, 132.0f, 10.0f},
		{TEXT("px-in"), 16.0f, 0.0f, 100.0f, 10.0f},
		{TEXT("py"), 0.0f, 10.0f, 100.0f, 26.0f},
		{TEXT("py-in"), 0.0f, 18.0f, 100.0f, 10.0f},
		{TEXT("ps"), 0.0f, 36.0f, 116.0f, 10.0f},
		{TEXT("ps-in"), 12.0f, 36.0f, 100.0f, 10.0f},
		{TEXT("c2"), 400.0f, 208.0f, 100.0f, 21.0f},
		{TEXT("c2-in"), 400.0f, 213.0f, 100.0f, 10.0f},
	};

	const FBoxCase ChromeMarginBoxes[] = {
		{TEXT("mx"), 540.0f, 46.0f, 200.0f, 10.0f},
		{TEXT("ms"), 24.0f, 56.0f, 100.0f, 10.0f},
		{TEXT("me"), 0.0f, 66.0f, 1256.0f, 10.0f},
		{TEXT("my"), 0.0f, 81.0f, 1280.0f, 10.0f},
		{TEXT("bx"), 0.0f, 98.0f, 106.0f, 10.0f},
		{TEXT("c1"), 400.0f, 194.0f, 100.0f, 10.0f},
		{TEXT("c2"), 400.0f, 208.0f, 100.0f, 21.0f},
	};

	const FBoxCase ChromeBorderBoxes[] = {
		{TEXT("bx"), 0.0f, 98.0f, 106.0f, 10.0f},
		{TEXT("by"), 0.0f, 108.0f, 100.0f, 14.0f},
		{TEXT("bs"), 0.0f, 122.0f, 104.0f, 11.0f},
		{TEXT("c4"), 400.0f, 269.0f, 105.0f, 10.0f},
		{TEXT("c4b"), 400.0f, 279.0f, 107.0f, 10.0f},
		{TEXT("c5"), 400.0f, 289.0f, 100.0f, 13.0f},
		{TEXT("c5b"), 400.0f, 302.0f, 100.0f, 12.0f},
		{TEXT("c6"), 400.0f, 314.0f, 100.0f, 13.0f},
		{TEXT("c7"), 400.0f, 327.0f, 104.0f, 10.0f},
	};

	const FBoxCase ChromeSizeAndInsetBoxes[] = {
		{TEXT("size"), 0.0f, 133.0f, 50.0f, 20.0f},
		{TEXT("minmax"), 0.0f, 153.0f, 60.0f, 15.0f},
		{TEXT("inset"), 0.0f, 168.0f, 1280.0f, 40.0f},
		{TEXT("inset-in"), 10.0f, 173.0f, 1250.0f, 10.0f},
		{TEXT("c3"), 400.0f, 229.0f, 800.0f, 40.0f},
		{TEXT("c3-a"), 1120.0f, 233.0f, 50.0f, 30.0f},
		{TEXT("c3-b"), 400.0f, 262.0f, 5.0f, 5.0f},
		{TEXT("c8"), 400.0f, 337.0f, 300.0f, 10.0f},
		{TEXT("c9"), 400.0f, 347.0f, 800.0f, 5.0f},
	};

	const FBoxCase ChromeOverrideBoxes[] = {
		{TEXT("o1"), 0.0f, 248.0f, 124.0f, 10.0f},
		{TEXT("o1-in"), 4.0f, 248.0f, 100.0f, 10.0f},
		{TEXT("o2"), 0.0f, 258.0f, 140.0f, 10.0f},
		{TEXT("o2-in"), 20.0f, 258.0f, 100.0f, 10.0f},
		{TEXT("o3"), 0.0f, 268.0f, 124.0f, 10.0f},
		{TEXT("o3-in"), 4.0f, 268.0f, 100.0f, 10.0f},
		{TEXT("o4"), 0.0f, 278.0f, 140.0f, 10.0f},
		{TEXT("o4-in"), 20.0f, 278.0f, 100.0f, 10.0f},
		{TEXT("o5"), 6.0f, 288.0f, 100.0f, 10.0f},
		{TEXT("o6"), 30.0f, 298.0f, 100.0f, 10.0f},
	};

	const FBoxCase ChromeVarShorthandBoxes[] = {
		{TEXT("v1"), 400.0f, 0.0f, 116.0f, 42.0f},
		{TEXT("v1-in"), 408.0f, 16.0f, 100.0f, 10.0f},
		{TEXT("v2"), 400.0f, 42.0f, 132.0f, 42.0f},
		{TEXT("v2-in"), 416.0f, 58.0f, 100.0f, 10.0f},
		{TEXT("v3"), 400.0f, 84.0f, 116.0f, 42.0f},
		{TEXT("v3-in"), 408.0f, 100.0f, 100.0f, 10.0f},
		{TEXT("v4"), 400.0f, 126.0f, 132.0f, 42.0f},
		{TEXT("v4-in"), 416.0f, 142.0f, 100.0f, 10.0f},
		{TEXT("v5"), 424.0f, 176.0f, 100.0f, 10.0f},
	};

	const FBoxCase ChromeFloatAndClearBoxes[] = {
		{TEXT("fl"), 400.0f, 352.0f, 300.0f, 20.0f},
		{TEXT("fl-s"), 400.0f, 352.0f, 40.0f, 20.0f},
		{TEXT("fl-e"), 650.0f, 352.0f, 50.0f, 20.0f},
		{TEXT("fl-n"), 400.0f, 352.0f, 300.0f, 5.0f},
		{TEXT("cl"), 400.0f, 372.0f, 300.0f, 30.0f},
		{TEXT("cl-s"), 400.0f, 392.0f, 300.0f, 5.0f},
		{TEXT("ce"), 400.0f, 402.0f, 300.0f, 35.0f},
		{TEXT("ce-e"), 400.0f, 432.0f, 300.0f, 5.0f},
	};

	const FBoxCase ChromeTextAlignX[] = {
		{TEXT("ta-start-ib"), 0.0f, 0.0f, 50.0f, 10.0f},
		{TEXT("ta-end-ib"), 1230.0f, 0.0f, 50.0f, 10.0f},
	};

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	const FChromePixel ChromePixels[] = {
		{TEXT("rounded start-start corner"), 20, 600, FColor(255, 255, 255, 255)},
		{TEXT("square start-end corner"), 119, 600, FColor(255, 0, 0, 255)},
		{TEXT("rounded end-start corner"), 20, 639, FColor(255, 255, 255, 255)},
		{TEXT("square end-end corner"), 119, 639, FColor(255, 0, 0, 255)},
		{TEXT("start edge middle"), 20, 620, FColor(255, 0, 0, 255)},
		{TEXT("border-inline start colour"), 1, 103, FColor(255, 0, 0, 255)},
		{TEXT("border-inline end colour"), 104, 103, FColor(255, 0, 0, 255)},
		{TEXT("inside the border-inline box"), 50, 103, FColor(255, 255, 255, 255)},
		{TEXT("border-block start colour"), 50, 108, FColor(0, 0, 255, 255)},
		{TEXT("border-block end colour"), 50, 121, FColor(0, 0, 255, 255)},
		{TEXT("border-inline-start colour"), 1, 127, FColor(0, 255, 0, 255)},
		{TEXT("square start-start corner"), 200, 600, FColor(0, 170, 0, 255)},
		{TEXT("rounded start-end corner"), 299, 600, FColor(255, 255, 255, 255)},
		{TEXT("square end-start corner"), 200, 639, FColor(0, 170, 0, 255)},
		{TEXT("rounded end-end corner"), 299, 639, FColor(255, 255, 255, 255)},
		{TEXT("border-inline-end colour"), 502, 274, FColor(255, 0, 255, 255)},
		{TEXT("border-inline-end-color"), 503, 284, FColor(0, 255, 255, 255)},
		{TEXT("border-block-start colour"), 450, 290, FColor(255, 128, 0, 255)},
		{TEXT("border-block-start-color"), 450, 302, FColor(128, 0, 255, 255)},
		{TEXT("border-block-color start"), 450, 314, FColor(18, 52, 86, 255)},
		{TEXT("border-block-color end"), 450, 326, FColor(18, 52, 86, 255)},
		{TEXT("border-inline start side"), 401, 332, FColor(101, 67, 33, 255)},
		{TEXT("border-inline end side"), 502, 332, FColor(101, 67, 33, 255)},
	};

	constexpr float Tolerance = 1.0f;
	constexpr int32 PixelTolerance = 2;

	FString DescribeMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FBoxCase> Cases, bool bOnlyX)
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
			const bool bXMatches = FMath::Abs(Rect.Position.X - Case.X) <= Tolerance && FMath::Abs(Rect.Size.X - Case.Width) <= Tolerance;
			const bool bYMatches = bOnlyX || (FMath::Abs(Rect.Position.Y - Case.Y) <= Tolerance && FMath::Abs(Rect.Size.Y - Case.Height) <= Tolerance);
			if (!bXMatches || !bYMatches)
			{
				Failures += FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, Chrome %.0f,%.0f %.0fx%.0f] "), Case.Id, Rect.Position.X, Rect.Position.Y,
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
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Logical/LogicalTest.rml"));
	}

	void DropCompiledFixture()
	{
		FArcRmlTestView::DropCompiledDocument(FixturePath());
		IFileManager::Get().Delete(*FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/Logical/Logical.rcssc")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
	}
}

TEST_CLASS(ArcRmlCssParityLogicalTests, "ArcRmlUI.CssParity.Logical")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FixturePath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(LogicalDeclarations_AreKnownProperties)
	{
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(PaddingInlineBlockStartAndEnd_PadThePhysicalSidesAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromePaddingBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(MarginInlineAutoStartEndAndBlock_PlaceTheBoxAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeMarginBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(BorderInlineBlockAndStartWidths_SizeTheBorderBoxAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeBorderBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(LogicalSizesAndInsets_SizeAndPlaceTheBoxAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeSizeAndInsetBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(TextAlignStartAndEnd_AlignLeftAndRightAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeTextAlignX, true);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(FloatAndClearInlineStartAndEnd_PlaceTheBoxesAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeFloatAndClearBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(OverlappingVarShorthands_FollowCascadeOrderAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeVarShorthandBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(LogicalAndPhysicalDeclarations_FollowDeclarationOrderAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		const FString Failures = DescribeMismatches(Layout, ChromeOverrideBoxes, false);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};

TEST_CLASS(ArcRmlCssParityLogicalPixelTests, "ArcRmlUI.Pixels.Logical")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(RoundedStartCornersAndLogicalBorderColours_ReadBackAsChrome)
	{
		using namespace ArcRmlCssParityLogicalTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		DropCompiledFixture();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FixturePath()) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize, View.GetPixelSize().ToString()));
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : ChromePixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		ASSERT_THAT(AreEqual(FString(), FString::Join(Mismatches, TEXT(" | "))));
	}
};
