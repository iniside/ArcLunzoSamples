// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlPixelTestsPrivate
{
	constexpr int32 Tolerance = 2;

	struct FExpectedPixel
	{
		const TCHAR* Name = TEXT("");
		int32 X = 0;
		int32 Y = 0;
		FColor Chrome;
	};

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= Tolerance && FMath::Abs(Measured.G - Expected.G) <= Tolerance
			&& FMath::Abs(Measured.B - Expected.B) <= Tolerance && FMath::Abs(Measured.A - Expected.A) <= Tolerance;
	}

	FString DescribePixels(const FArcRmlTestView& View, TConstArrayView<FExpectedPixel> Expected)
	{
		TArray<FString> Parts;
		for (const FExpectedPixel& Pixel : Expected)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			Parts.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d,%d chrome %d,%d,%d,%d%s"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
				Measured.A, Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B, Pixel.Chrome.A, IsNear(Measured, Pixel.Chrome) ? TEXT("") : TEXT(" MISMATCH")));
		}
		return FString::Join(Parts, TEXT(" | "));
	}

	int32 CountMismatches(const FArcRmlTestView& View, TConstArrayView<FExpectedPixel> Expected)
	{
		int32 Mismatches = 0;
		for (const FExpectedPixel& Pixel : Expected)
		{
			Mismatches += IsNear(View.GetPixel(Pixel.X, Pixel.Y), Pixel.Chrome) ? 0 : 1;
		}
		return Mismatches;
	}

	constexpr double CoverageToleranceFraction = 0.005;

	constexpr int32 InkTolerance = 1;

	// A pixel counts as ink when its green channel is below the midpoint between the fixture's #c03020 text and the white backdrop; the Chrome
	// screenshot was read with the same rule.
	bool HasInk(const FColor& Pixel)
	{
		return Pixel.G < 150;
	}

	TArray<int32> FindInkRunStarts(const FArcRmlTestView& View, const FIntRect& Rect)
	{
		TArray<int32> Starts;
		bool bInRun = false;
		for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
		{
			bool bColumnHasInk = false;
			for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y && !bColumnHasInk; ++Y)
			{
				bColumnHasInk = HasInk(View.GetPixel(X, Y));
			}
			if (bColumnHasInk && !bInRun)
			{
				Starts.Add(X);
			}
			bInRun = bColumnHasInk;
		}
		return Starts;
	}

	bool StartsMatch(const TArray<int32>& Measured, TConstArrayView<int32> Chrome)
	{
		if (Measured.Num() != Chrome.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Chrome.Num(); ++Index)
		{
			if (FMath::Abs(Measured[Index] - Chrome[Index]) > InkTolerance)
			{
				return false;
			}
		}
		return true;
	}

	FString DescribeStarts(const TArray<int32>& Measured)
	{
		TArray<FString> Parts;
		for (const int32 Start : Measured)
		{
			Parts.Add(FString::FromInt(Start));
		}
		return FString::Join(Parts, TEXT(" "));
	}
}

TEST_CLASS(ArcRmlPixelTests, "ArcRmlUI.Pixels")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool RenderFixture(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		FArcRmlTestView::DropCompiledDocument(Path);
		return Assert.IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View))
			&& Assert.AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)) && Assert.IsTrue(View.RenderPixels())
			&& Assert.IsTrue(View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize, View.GetPixelSize().ToString());
	}

	TEST_METHOD(SmokeFixture_BoxGradientAndRoundedCorner_MatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Pixels/PixelSmokeTest.rml"))));

		const FExpectedPixel Expected[] = {
			{TEXT("backdrop"), 5, 5, FColor(255, 255, 255, 255)},
			{TEXT("box"), 70, 50, FColor(51, 102, 204, 255)},
			{TEXT("box left outside"), 19, 50, FColor(255, 255, 255, 255)},
			{TEXT("box left inside"), 20, 50, FColor(51, 102, 204, 255)},
			{TEXT("box right inside"), 119, 50, FColor(51, 102, 204, 255)},
			{TEXT("box right outside"), 120, 50, FColor(255, 255, 255, 255)},
			{TEXT("box top outside"), 70, 19, FColor(255, 255, 255, 255)},
			{TEXT("box top inside"), 70, 20, FColor(51, 102, 204, 255)},
			{TEXT("box bottom inside"), 70, 79, FColor(51, 102, 204, 255)},
			{TEXT("box bottom outside"), 70, 80, FColor(255, 255, 255, 255)},
			{TEXT("gradient left outside"), 139, 50, FColor(255, 255, 255, 255)},
			{TEXT("gradient left inside"), 140, 50, FColor(254, 0, 0, 255)},
			{TEXT("gradient start"), 141, 50, FColor(254, 0, 2, 255)},
			{TEXT("gradient quarter"), 204, 50, FColor(190, 0, 64, 255)},
			{TEXT("gradient middle"), 268, 50, FColor(127, 0, 128, 255)},
			{TEXT("gradient three quarters"), 332, 50, FColor(63, 0, 191, 255)},
			{TEXT("gradient end"), 394, 50, FColor(1, 0, 253, 255)},
			{TEXT("corner outside"), 422, 22, FColor(255, 255, 255, 255)},
			{TEXT("corner outside the arc"), 428, 28, FColor(255, 255, 255, 255)},
			{TEXT("corner inside the arc"), 440, 40, FColor(34, 136, 68, 255)},
			{TEXT("rounded box"), 470, 70, FColor(34, 136, 68, 255)},
			{TEXT("translucent left outside"), 539, 50, FColor(255, 255, 255, 255)},
			{TEXT("translucent left inside"), 540, 50, FColor(153, 178, 229, 255)},
			{TEXT("translucent middle"), 590, 50, FColor(153, 178, 229, 255)},
			{TEXT("translucent right outside"), 640, 50, FColor(255, 255, 255, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));

		constexpr double ChromeRoundedCoverage = 8595.042;
		const double Measured = View.SumCoverage(FIntRect(420, 20, 520, 120), FColor(34, 136, 68, 255));
		ASSERT_THAT(IsTrue(FMath::Abs(Measured - ChromeRoundedCoverage) <= ChromeRoundedCoverage * CoverageToleranceFraction,
			FString::Printf(TEXT("rounded box coverage measured %.3f chrome %.3f"), Measured, ChromeRoundedCoverage)));
	}

	TEST_METHOD(OutlineFixture_EllipticalRadii_MatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Outline/OutlineTest.rml"))));

		const FExpectedPixel Expected[] = {
			{TEXT("ellipse top left inside the ring"), 1023, 424, FColor(136, 0, 204, 255)},
			{TEXT("ellipse top left outside the ring"), 1016, 424, FColor(51, 51, 51, 255)},
			{TEXT("ellipse bottom left inside the ring"), 1023, 476, FColor(136, 0, 204, 255)},
			{TEXT("ellipse left slope outside the ring, upper"), 1012, 432, FColor(51, 51, 51, 255)},
			{TEXT("ellipse left slope outside the ring, upper again"), 1012, 433, FColor(51, 51, 51, 255)},
			{TEXT("ellipse left slope outside the ring, lower"), 1012, 467, FColor(51, 51, 51, 255)},
			{TEXT("ellipse top right inside the ring"), 1176, 424, FColor(136, 0, 204, 255)},
			{TEXT("ellipse top right outside the ring"), 1183, 424, FColor(51, 51, 51, 255)},
			{TEXT("ellipse middle"), 1100, 425, FColor(136, 0, 204, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(OutlineFixture_DashedTopEdge_DashesAndGapsMatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Outline/OutlineTest.rml"))));

		const FColor Blue(0, 0, 255, 255);
		const FColor Black(0, 0, 0, 255);
		const FExpectedPixel Expected[] = {
			{TEXT("corner dash"), 300, 18, Blue},
			{TEXT("first gap"), 304, 18, Black},
			{TEXT("dash 1"), 309, 18, Blue},
			{TEXT("gap 1"), 313, 18, Black},
			{TEXT("dash 2"), 318, 18, Blue},
			{TEXT("gap 2"), 322, 18, Black},
			{TEXT("dash 3"), 327, 18, Blue},
			{TEXT("gap row above the dash"), 314, 17, Black},
			{TEXT("gap row below the dash"), 314, 19, Black},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(OutlineFixture_DottedTopEdge_DotsAndGapsMatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Outline/OutlineTest.rml"))));

		const FColor Ink(255, 51, 153, 255);
		const FColor Black(0, 0, 0, 255);
		const FExpectedPixel Expected[] = {
			{TEXT("dot 0"), 297, 517, Ink},
			{TEXT("dot 0 lower"), 298, 518, Ink},
			{TEXT("gap 0"), 301, 517, Black},
			{TEXT("dot 1"), 305, 517, Ink},
			{TEXT("dot 1 lower"), 306, 518, Ink},
			{TEXT("gap 1"), 309, 517, Black},
			{TEXT("dot 2"), 313, 517, Ink},
			{TEXT("gap 2"), 317, 517, Black},
			{TEXT("dot 3"), 321, 517, Ink},
			{TEXT("gap 3"), 325, 518, Black},
			{TEXT("dot 4"), 329, 517, Ink},
			{TEXT("row above the dots"), 305, 515, Black},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(OutlineFixture_RoundedDashedRing_DashesAndGapsMatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Outline/OutlineTest.rml"))));

		const FColor Ink(51, 153, 255, 255);
		const FColor Black(0, 0, 0, 255);
		const FExpectedPixel Expected[] = {
			{TEXT("top dash 1"), 484, 517, Ink},
			{TEXT("top gap 1"), 489, 517, Black},
			{TEXT("top dash 2"), 495, 517, Ink},
			{TEXT("top gap 2"), 501, 517, Black},
			{TEXT("top dash 3"), 507, 517, Ink},
			{TEXT("top gap above the dash"), 489, 516, Black},
			{TEXT("arc dash a"), 474, 517, Ink},
			{TEXT("arc dash b"), 472, 518, Ink},
			{TEXT("arc dash c"), 471, 519, Ink},
			{TEXT("arc gap"), 466, 522, Black},
			{TEXT("arc dash d"), 462, 526, Ink},
			{TEXT("arc dash e"), 461, 528, Ink},
			{TEXT("arc gap end"), 458, 531, Black},
			{TEXT("left dash"), 458, 536, Ink},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(PaintOrderFixture_StrokeAndFillOrder_MatchChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Text/PaintOrderTest.rml"))));

		const FExpectedPixel Expected[] = {
			{TEXT("normal outside stroke half"), 40, 62, FColor(212, 52, 42, 255)},
			{TEXT("normal inside the glyph edge shows the stroke"), 42, 62, FColor(212, 52, 42, 255)},
			{TEXT("stroke fill outside stroke half"), 40, 142, FColor(42, 212, 140, 255)},
			{TEXT("stroke fill inside the glyph edge shows the fill"), 42, 142, FColor(212, 176, 42, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(GradientHardStopFixture_HardStops_AreSharpAsInChromePixels)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Pixels/GradientHardStopTest.rml"))));

		const FColor Orange(240, 160, 32, 255);
		const FColor Blue(32, 96, 208, 255);
		const FColor White(255, 255, 255, 255);
		const FColor Black(0, 0, 0, 255);
		const FColor DarkStripe(180, 181, 183, 255);
		const FExpectedPixel Expected[] = {
			{TEXT("split start"), 25, 50, Orange},
			{TEXT("split two before the stop"), 118, 50, Orange},
			{TEXT("split last before the stop"), 119, 50, Orange},
			{TEXT("split first after the stop"), 120, 50, Blue},
			{TEXT("split two after the stop"), 121, 50, Blue},
			{TEXT("split end"), 214, 50, Blue},
			{TEXT("stripe first"), 241, 40, White},
			{TEXT("gap first after the stripe"), 242, 40, Black},
			{TEXT("gap second"), 243, 40, Black},
			{TEXT("gap last before the wrap"), 254, 40, Black},
			{TEXT("stripe first after the wrap"), 255, 40, White},
			{TEXT("stripe second after the wrap"), 256, 40, White},
			{TEXT("gap first in the second period"), 257, 40, Black},
			{TEXT("gap first in the tenth period"), 377, 40, Black},
			{TEXT("diagonal dark, first period"), 421, 21, DarkStripe},
			{TEXT("diagonal light, first period"), 423, 24, FColor(212, 213, 214, 255)},
			{TEXT("diagonal dark, second period"), 426, 27, DarkStripe},
			{TEXT("diagonal light, second period"), 429, 29, FColor(212, 212, 213, 255)},
			{TEXT("diagonal dark, middle"), 466, 66, DarkStripe},
			{TEXT("diagonal light, middle"), 470, 69, FColor(212, 212, 214, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}

	TEST_METHOD(FontFeatureFixture_TabularNums_InkColumnsMatchChrome)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Text/FontFeaturePixelTest.rml"))));

		const int32 ChromeDigitStarts[] = {42, 59, 76};
		const TArray<int32> Damage111 = FindInkRunStarts(View, FIntRect(20, 30, 300, 100));
		const TArray<int32> Damage808 = FindInkRunStarts(View, FIntRect(20, 95, 300, 170));
		ASSERT_THAT(IsTrue(StartsMatch(Damage111, ChromeDigitStarts), FString::Printf(TEXT("111 digit ink starts at %s, Chrome 42 59 76"), *DescribeStarts(Damage111))));
		ASSERT_THAT(IsTrue(StartsMatch(Damage808, ChromeDigitStarts), FString::Printf(TEXT("808 digit ink starts at %s, Chrome 42 59 76"), *DescribeStarts(Damage808))));
	}

	TEST_METHOD(FontFeatureFixture_SlashedZero_SlashHasInkAndPlainZeroIsEmptyAsInChrome)
	{
		using namespace ArcRmlPixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("CssParity/Text/FontFeaturePixelTest.rml"))));

		const FExpectedPixel Expected[] = {
			{TEXT("slash inside the slashed zero"), 69, 260, FColor(192, 48, 32, 255)},
			{TEXT("slash left neighbour"), 68, 260, FColor(192, 48, 32, 255)},
			{TEXT("slash right neighbour"), 70, 260, FColor(192, 48, 32, 255)},
			{TEXT("same pixel in the plain zero"), 269, 260, FColor(255, 255, 255, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Expected), DescribePixels(View, Expected)));
	}
};
