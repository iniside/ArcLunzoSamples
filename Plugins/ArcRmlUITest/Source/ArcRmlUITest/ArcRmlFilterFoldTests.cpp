// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"

namespace ArcRmlFilterFoldTestsPrivate
{
	constexpr int32 Tolerance = 1;

	constexpr int32 BlurTolerance = 7;

	struct FExpectedPixel
	{
		const TCHAR* Name = TEXT("");
		int32 X = 0;
		int32 Y = 0;
		FColor Chrome;
		int32 PixelTolerance = Tolerance;
	};

	struct FClobberScope
	{
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 0;

		FClobberScope()
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RDG.ClobberResources"));
			if (Variable != nullptr)
			{
				Previous = Variable->GetInt();
				Variable->Set(1, ECVF_SetByCode);
			}
		}

		bool IsOn() const
		{
			return Variable != nullptr && Variable->GetInt() == 1;
		}

		~FClobberScope()
		{
			if (Variable != nullptr)
			{
				Variable->Set(Previous, ECVF_SetByCode);
			}
		}
	};

	bool IsNear(const FColor& Measured, const FColor& Expected, int32 Allowed = Tolerance)
	{
		return FMath::Abs(Measured.R - Expected.R) <= Allowed && FMath::Abs(Measured.G - Expected.G) <= Allowed
			&& FMath::Abs(Measured.B - Expected.B) <= Allowed && FMath::Abs(Measured.A - Expected.A) <= Allowed;
	}

	FString DescribeMismatches(const FArcRmlTestView& View, TConstArrayView<FExpectedPixel> Expected)
	{
		TArray<FString> Parts;
		for (const FExpectedPixel& Pixel : Expected)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome, Pixel.PixelTolerance))
			{
				Parts.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d,%d chrome %d,%d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G,
					Measured.B, Measured.A, Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B, Pixel.Chrome.A));
			}
		}
		return Parts.IsEmpty() ? FString(TEXT("all near")) : FString::Join(Parts, TEXT(" | "));
	}

	int32 CountMismatches(const FArcRmlTestView& View, TConstArrayView<FExpectedPixel> Expected)
	{
		int32 Mismatches = 0;
		for (const FExpectedPixel& Pixel : Expected)
		{
			Mismatches += IsNear(View.GetPixel(Pixel.X, Pixel.Y), Pixel.Chrome, Pixel.PixelTolerance) ? 0 : 1;
		}
		return Mismatches;
	}

	FString DescribeRectMismatches(const FArcRmlTestView& View, const FIntRect& Rect, const FColor& Expected, int32& OutMismatches)
	{
		OutMismatches = 0;
		FString First;
		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
		{
			for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
			{
				const FColor Measured = View.GetPixel(X, Y);
				if (!IsNear(Measured, Expected))
				{
					First = OutMismatches == 0 ? FString::Printf(TEXT("first at %d,%d: measured %d,%d,%d,%d expected %d,%d,%d,%d"), X, Y, Measured.R, Measured.G,
												 Measured.B, Measured.A, Expected.R, Expected.G, Expected.B, Expected.A)
												   : First;
					++OutMismatches;
				}
			}
		}
		return First;
	}

	const FExpectedPixel FilterChainPixels[] = {
		{TEXT("contrast blur invert: left interior"), 40, 65, FColor(139, 193, 220, 255)},
		{TEXT("contrast blur invert: right interior"), 130, 65, FColor(207, 168, 94, 255)},
		{TEXT("contrast blur invert: seam left"), 86, 65, FColor(153, 188, 194, 255), BlurTolerance},
		{TEXT("contrast blur invert: seam right"), 94, 65, FColor(197, 171, 113, 255), BlurTolerance},
		{TEXT("contrast blur invert: near left edge"), 23, 65, FColor(164, 206, 227, 255), BlurTolerance},
		{TEXT("contrast blur invert: outside left"), 19, 65, FColor(202, 227, 239, 255), BlurTolerance},
		{TEXT("contrast blur invert: near right edge"), 157, 65, FColor(221, 193, 140, 255), BlurTolerance},
		{TEXT("contrast blur invert: near top edge"), 50, 21, FColor(182, 216, 233, 255), BlurTolerance},
		{TEXT("contrast blur invert: above top"), 50, 18, FColor(213, 232, 242, 255), BlurTolerance},
		{TEXT("blur saturate brightness: left interior"), 220, 65, FColor(218, 146, 110, 255)},
		{TEXT("blur saturate brightness: right interior"), 310, 65, FColor(36, 89, 187, 255)},
		{TEXT("blur saturate brightness: seam left"), 266, 65, FColor(168, 131, 131, 255), BlurTolerance},
		{TEXT("blur saturate brightness: seam right"), 274, 65, FColor(75, 101, 171, 255), BlurTolerance},
		{TEXT("blur saturate brightness: near left edge"), 203, 65, FColor(227, 175, 149, 255), BlurTolerance},
		{TEXT("blur saturate brightness: outside left"), 199, 65, FColor(238, 204, 188, 255), BlurTolerance},
		{TEXT("blur saturate brightness: near right edge"), 337, 65, FColor(109, 144, 210, 255), BlurTolerance},
		{TEXT("blur saturate brightness: near top edge"), 230, 21, FColor(232, 189, 168, 255), BlurTolerance},
		{TEXT("blur saturate brightness: above top"), 230, 18, FColor(240, 212, 197, 255), BlurTolerance},
		{TEXT("brightness contrast opacity: left interior"), 400, 65, FColor(237, 207, 192, 255)},
		{TEXT("brightness contrast opacity: right interior"), 490, 65, FColor(155, 177, 219, 255)},
		{TEXT("brightness contrast opacity: seam left"), 446, 65, FColor(237, 207, 192, 255)},
		{TEXT("brightness contrast opacity: seam right"), 454, 65, FColor(155, 177, 219, 255)},
		{TEXT("brightness contrast opacity: near left edge"), 383, 65, FColor(237, 207, 192, 255)},
		{TEXT("brightness contrast opacity: outside left"), 379, 65, FColor(255, 255, 255, 255)},
		{TEXT("brightness contrast opacity: near right edge"), 517, 65, FColor(155, 177, 219, 255)},
		{TEXT("brightness contrast opacity: near top edge"), 410, 21, FColor(237, 207, 192, 255)},
		{TEXT("brightness contrast opacity: above top"), 410, 18, FColor(255, 255, 255, 255)},
		{TEXT("hue-rotate sepia: left interior"), 580, 65, FColor(181, 180, 146, 255)},
		{TEXT("hue-rotate sepia: right interior"), 670, 65, FColor(149, 116, 129, 255)},
		{TEXT("hue-rotate sepia: seam left"), 626, 65, FColor(181, 180, 146, 255)},
		{TEXT("hue-rotate sepia: seam right"), 634, 65, FColor(149, 116, 129, 255)},
		{TEXT("hue-rotate sepia: near left edge"), 563, 65, FColor(181, 180, 146, 255)},
		{TEXT("hue-rotate sepia: outside left"), 559, 65, FColor(255, 255, 255, 255)},
		{TEXT("hue-rotate sepia: near right edge"), 697, 65, FColor(149, 116, 129, 255)},
		{TEXT("hue-rotate sepia: near top edge"), 590, 21, FColor(181, 180, 146, 255)},
		{TEXT("hue-rotate sepia: above top"), 590, 18, FColor(255, 255, 255, 255)},
		{TEXT("sepia blur hue-rotate: left interior"), 760, 65, FColor(151, 194, 152, 255)},
		{TEXT("sepia blur hue-rotate: right interior"), 850, 65, FColor(142, 100, 126, 255)},
		{TEXT("sepia blur hue-rotate: seam left"), 806, 65, FColor(149, 174, 147, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: seam right"), 814, 65, FColor(143, 114, 129, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: near left edge"), 743, 65, FColor(173, 207, 175, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: outside left"), 739, 65, FColor(207, 227, 208, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: near right edge"), 877, 65, FColor(174, 144, 163, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: near top edge"), 770, 21, FColor(189, 216, 190, 255), BlurTolerance},
		{TEXT("sepia blur hue-rotate: above top"), 770, 18, FColor(217, 233, 217, 255), BlurTolerance},
		{TEXT("grayscale invert blur: left interior"), 940, 65, FColor(178, 178, 178, 255)},
		{TEXT("grayscale invert blur: right interior"), 1030, 65, FColor(138, 138, 138, 255)},
		{TEXT("grayscale invert blur: seam left"), 986, 65, FColor(173, 173, 173, 255), BlurTolerance},
		{TEXT("grayscale invert blur: seam right"), 994, 65, FColor(142, 142, 142, 255), BlurTolerance},
		{TEXT("grayscale invert blur: near left edge"), 923, 65, FColor(188, 188, 188, 255), BlurTolerance},
		{TEXT("grayscale invert blur: outside left"), 919, 65, FColor(221, 221, 221, 255), BlurTolerance},
		{TEXT("grayscale invert blur: near right edge"), 1057, 65, FColor(164, 164, 164, 255), BlurTolerance},
		{TEXT("grayscale invert blur: near top edge"), 950, 21, FColor(203, 203, 203, 255), BlurTolerance},
		{TEXT("grayscale invert blur: above top"), 950, 18, FColor(230, 230, 230, 255), BlurTolerance},
	};

	const FExpectedPixel BackdropGlassPixels[] = {
		{TEXT("glass blur saturate brightness: left flat"), 90, 270, FColor(181, 89, 63, 255)},
		{TEXT("glass blur saturate brightness: before seam"), 164, 270, FColor(166, 91, 78, 255), BlurTolerance},
		{TEXT("glass blur saturate brightness: seam left"), 168, 270, FColor(132, 95, 112, 255), BlurTolerance},
		{TEXT("glass blur saturate brightness: seam right"), 172, 270, FColor(90, 99, 156, 255), BlurTolerance},
		{TEXT("glass blur saturate brightness: after seam"), 176, 270, FColor(61, 103, 186, 255), BlurTolerance},
		{TEXT("glass blur saturate brightness: right flat"), 250, 270, FColor(51, 104, 195, 255)},
		{TEXT("glass contrast blur invert: left flat"), 430, 270, FColor(221, 171, 83, 255)},
		{TEXT("glass contrast blur invert: before seam"), 514, 270, FColor(220, 165, 95, 255), BlurTolerance},
		{TEXT("glass contrast blur invert: seam left"), 518, 270, FColor(216, 152, 120, 255), BlurTolerance},
		{TEXT("glass contrast blur invert: seam right"), 522, 270, FColor(212, 135, 153, 255), BlurTolerance},
		{TEXT("glass contrast blur invert: after seam"), 526, 270, FColor(210, 124, 175, 255), BlurTolerance},
		{TEXT("glass contrast blur invert: right flat"), 590, 270, FColor(209, 120, 183, 255)},
	};

	const FExpectedPixel MatrixNeighbourPixels[] = {
		{TEXT("sepia drop-shadow: content"), 60, 430, FColor(165, 147, 114, 255)},
		{TEXT("sepia drop-shadow: shadow right of content"), 144, 450, FColor(32, 96, 208, 255)},
		{TEXT("sepia drop-shadow: shadow below content"), 100, 484, FColor(32, 96, 208, 255)},
		{TEXT("blur invert mask: interior"), 260, 440, FColor(164, 114, 89, 255)},
		{TEXT("invert mask: interior"), 440, 440, FColor(164, 114, 89, 255)},
	};
}

TEST_CLASS(ArcRmlFilterFoldTests, "ArcRmlUI.Pixels")
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

	TEST_METHOD(FilterChains_ColourMatricesAroundBlur_MatchChromePixels)
	{
		using namespace ArcRmlFilterFoldTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FClobberScope Clobber;
		ASSERT_THAT(IsTrue(Clobber.IsOn()));
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("Filter/ColorMatrixFold.rml"))));
		ASSERT_THAT(AreEqual(0, CountMismatches(View, FilterChainPixels), DescribeMismatches(View, FilterChainPixels)));
	}

	TEST_METHOD(BackdropFilterChains_ColourMatricesAroundBlur_MatchChromePixels)
	{
		using namespace ArcRmlFilterFoldTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FClobberScope Clobber;
		ASSERT_THAT(IsTrue(Clobber.IsOn()));
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("Filter/ColorMatrixFold.rml"))));
		ASSERT_THAT(AreEqual(0, CountMismatches(View, BackdropGlassPixels), DescribeMismatches(View, BackdropGlassPixels)));
	}

	TEST_METHOD(ColourMatrices_NextToDropShadowAndMask_MatchChromePixels)
	{
		using namespace ArcRmlFilterFoldTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FClobberScope Clobber;
		ASSERT_THAT(IsTrue(Clobber.IsOn()));
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("Filter/ColorMatrixFold.rml"))));
		ASSERT_THAT(AreEqual(0, CountMismatches(View, MatrixNeighbourPixels), DescribeMismatches(View, MatrixNeighbourPixels)));
	}

	TEST_METHOD(EmptyFilteredLayers_ReadBeforeAnyWrite_LeaveBackdropUntouched)
	{
		using namespace ArcRmlFilterFoldTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		FClobberScope Clobber;
		ASSERT_THAT(IsTrue(Clobber.IsOn()));
		ASSERT_THAT(IsTrue(RenderFixture(TEXT("Filter/ReadFirstClear.rml"))));

		int32 Mismatches = 0;
		const FString First = DescribeRectMismatches(View, FIntRect(20, 20, 540, 120), FColor(51, 102, 204, 255), Mismatches);
		ASSERT_THAT(AreEqual(0, Mismatches, First));

		const FExpectedPixel Control[] = {
			{TEXT("control interior"), 70, 190, FColor(51, 204, 204, 255)},
			{TEXT("control far outside"), 200, 190, FColor(255, 255, 255, 255)},
		};
		ASSERT_THAT(AreEqual(0, CountMismatches(View, Control), DescribeMismatches(View, Control)));
	}
};
