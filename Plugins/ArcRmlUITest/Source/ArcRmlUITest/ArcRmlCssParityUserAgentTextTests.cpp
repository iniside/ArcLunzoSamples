// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityUserAgentTextTestsPrivate
{
	struct FInlineCase
	{
		const TCHAR* Id;
		float X;
		float Width;
		float Height;
	};

	struct FLineCase
	{
		const TCHAR* Id;
		float X;
		float Width;
	};

	struct FValueCase
	{
		const TCHAR* Id;
		float Value;
	};

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	// Chrome 156, cdp_measure.mjs on the layout dump of UserAgentTextTest (Saved/ArcRml/Step9bChrome/dump).
	const FInlineCase ChromeWeightBoxes[] = {
		{TEXT("w-b1"), 51.9844f, 112.5938f, 21.0f},
		{TEXT("w-b2"), 86.0938f, 78.4844f, 21.0f},
		{TEXT("w-b3"), 120.7969f, 43.7812f, 21.0f},
		{TEXT("w-lighter"), 46.9531f, 55.0938f, 21.0f},
		{TEXT("w-strong"), 106.125f, 52.9062f, 21.0f},
	};

	const FLineCase ChromeWeightLines[] = {
		{TEXT("w-b1"), 51.9844f, 34.1094f},
		{TEXT("w-b2"), 86.0938f, 34.7031f},
	};

	// Chrome 156 getComputedStyle font-weight (Saved/ArcRml/Step9bChrome/ua_text_loaded.measure.json).
	const FValueCase ChromeWeights[] = {
		{TEXT("w-b1"), 400.0f},
		{TEXT("w-b2"), 700.0f},
		{TEXT("w-b3"), 900.0f},
		{TEXT("w-lighter"), 400.0f},
		{TEXT("w-strong"), 900.0f},
		{TEXT("k-l99"), 99.0f},
		{TEXT("k-l549"), 100.0f},
		{TEXT("k-l550"), 400.0f},
		{TEXT("k-l750"), 700.0f},
		{TEXT("k-l900"), 700.0f},
		{TEXT("k-b900"), 900.0f},
	};

	const FValueCase ChromeInheritedWeights[] = {
		{TEXT("i-b"), 700.0f},
		{TEXT("i-inherit"), 700.0f},
		{TEXT("i-unset"), 700.0f},
		{TEXT("i-b2"), 900.0f},
		{TEXT("i-inherit2"), 900.0f},
	};

	const FValueCase ChromeInheritedSizes[] = {
		{TEXT("i-small"), 13.3333f},
		{TEXT("i-size"), 13.3333f},
		{TEXT("i-size-unset"), 13.3333f},
	};

	const FLineCase ChromeInheritedThLines[] = {
		{TEXT("i-th-inherit"), 103.9219f, 10.1562f},
		{TEXT("i-th-block"), 103.6875f, 10.625f},
	};

	// Chrome 156 with body.edited's values written inline (Saved/ArcRml/Step9bChrome/ua_text_edited.measure.json).
	const FValueCase ChromeEditedWeights[] = {
		{TEXT("w-light"), 500.0f},
		{TEXT("w-b1"), 700.0f},
		{TEXT("w-b2"), 900.0f},
		{TEXT("w-b3"), 900.0f},
	};

	const FValueCase ChromeEditedSizes[] = {
		{TEXT("s-small1"), 16.6667f},
		{TEXT("s-small2"), 13.8889f},
		{TEXT("s-larger"), 24.0f},
		{TEXT("s-sub"), 16.6667f},
		{TEXT("s-sup"), 16.6667f},
	};

	const FInlineCase ChromeEditedSizeBoxes[] = {
		{TEXT("s-small1"), 55.1406f, 92.625f, 21.0f},
		{TEXT("s-small2"), 100.2812f, 47.4844f, 18.0f},
		{TEXT("s-larger"), 152.9062f, 66.9688f, 31.0f},
		{TEXT("s-sub"), 238.1406f, 10.0469f, 21.0f},
		{TEXT("s-sup"), 278.5781f, 10.0469f, 21.0f},
	};

	const FLineCase ChromeEditedThLines[] = {
		{TEXT("t-th1"), 103.9219f, 10.1562f},
		{TEXT("t-th2"), 103.9219f, 10.1562f},
		{TEXT("t-th3"), 198.8438f, 10.1562f},
	};

	const FInlineCase ChromeSizeBoxes[] = {
		{TEXT("s-small1"), 45.7188f, 74.1406f, 17.0f},
		{TEXT("s-small2"), 81.8438f, 38.0156f, 14.0f},
		{TEXT("s-larger"), 123.9844f, 53.5781f, 25.0f},
		{TEXT("s-sub"), 192.1719f, 8.0469f, 17.0f},
		{TEXT("s-sup"), 224.5312f, 8.0469f, 17.0f},
	};

	const FInlineCase ChromeMonospaceBoxes[] = {
		{TEXT("c-code"), 39.4375f, 76.8125f, 21.0f},
		{TEXT("c-pre"), 8.0f, 1264.0f, 42.0f},
	};

	constexpr float ChromePreWidestLine = 105.6094f;

	const FLineCase ChromeThLines[] = {
		{TEXT("t-th1"), 103.9219f, 10.1562f},
		{TEXT("t-th2"), 9.0f, 10.1562f},
		{TEXT("t-th3"), 198.8438f, 10.1562f},
	};

	const FInlineCase ChromeHrBox = {TEXT("hr1"), 8.0f, 1264.0f, 2.0f};

	// Chrome 156 screenshot of Saved/ArcRml/Step9bChrome/hr_pixels.html, the same page as UserAgentHrPixelTest.
	const FChromePixel ChromeHrPixels[] = {
		{TEXT("above the hr"), 50, 9, FColor(255, 255, 255, 255)},
		{TEXT("hr top side"), 50, 10, FColor(154, 154, 154, 255)},
		{TEXT("hr top-left corner"), 10, 10, FColor(154, 154, 154, 255)},
		{TEXT("hr bottom side"), 50, 11, FColor(238, 238, 238, 255)},
		{TEXT("hr bottom-right corner"), 109, 11, FColor(238, 238, 238, 255)},
		{TEXT("under the hr"), 50, 12, FColor(255, 255, 255, 255)},
		{TEXT("4px hr top side"), 50, 30, FColor(154, 154, 154, 255)},
		{TEXT("4px hr top side, last row"), 50, 33, FColor(154, 154, 154, 255)},
		{TEXT("4px hr bottom side, first row"), 50, 34, FColor(238, 238, 238, 255)},
		{TEXT("4px hr bottom side"), 50, 37, FColor(238, 238, 238, 255)},
		{TEXT("4px hr left side"), 11, 35, FColor(154, 154, 154, 255)},
		{TEXT("4px hr right side"), 108, 32, FColor(238, 238, 238, 255)},
		{TEXT("black hr top side"), 50, 50, FColor(154, 154, 154, 255)},
		{TEXT("black hr bottom side"), 50, 51, FColor(238, 238, 238, 255)},
		{TEXT("blue hr top side"), 50, 70, FColor(30, 60, 120, 255)},
		{TEXT("blue hr bottom side"), 50, 71, FColor(63, 127, 255, 255)},
		{TEXT("left of the hr"), 9, 10, FColor(255, 255, 255, 255)},
		{TEXT("right of the hr"), 110, 11, FColor(255, 255, 255, 255)},
	};

	// Chrome 156 screenshot of Saved/ArcRml/Step9bChrome/hr_pixels2.html (hr_pixels2.png), the boxes added to UserAgentHrPixelTest.
	const FChromePixel ChromeInsetPixels[] = {
		{TEXT("#303030 top side"), 50, 90, FColor(0, 0, 0, 255)},
		{TEXT("#303030 left side"), 11, 94, FColor(0, 0, 0, 255)},
		{TEXT("#303030 bottom side"), 50, 97, FColor(132, 132, 132, 255)},
		{TEXT("#303030 right side"), 112, 94, FColor(132, 132, 132, 255)},
		{TEXT("under #303030"), 50, 98, FColor(255, 255, 255, 255)},
		{TEXT("#f0f0f0 top side"), 50, 111, FColor(156, 156, 156, 255)},
		{TEXT("#f0f0f0 left side"), 10, 114, FColor(156, 156, 156, 255)},
		{TEXT("#f0f0f0 bottom side"), 50, 116, FColor(240, 240, 240, 255)},
		{TEXT("#f0f0f0 right side"), 113, 114, FColor(240, 240, 240, 255)},
		{TEXT("#ffffff top side"), 50, 130, FColor(171, 171, 171, 255)},
		{TEXT("#ffffff left side"), 11, 134, FColor(171, 171, 171, 255)},
		{TEXT("#ffffff bottom side"), 50, 137, FColor(255, 255, 255, 255)},
		{TEXT("right of #ffffff"), 114, 134, FColor(255, 255, 255, 255)},
	};

	constexpr float Tolerance = 0.5f;
	constexpr float SizeTolerance = 0.01f;
	constexpr int32 PixelTolerance = 2;

	FString DescribeInline(const TArray<FArcRmlTestLayoutElement>& Layout, const FInlineCase& Case, float MaxError)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Case.X) > MaxError || FMath::Abs(Rect.Size.X - Case.Width) > MaxError || FMath::Abs(Rect.Size.Y - Case.Height) > MaxError)
		{
			return FString::Printf(TEXT("[%s is x %.2f, %.2fx%.2f, Chrome x %.2f, %.2fx%.2f] "), Case.Id, Rect.Position.X, Rect.Size.X, Rect.Size.Y, Case.X, Case.Width, Case.Height);
		}
		return FString();
	}

	template <int32 N>
	FString DescribeInlines(const TArray<FArcRmlTestLayoutElement>& Layout, const FInlineCase (&Cases)[N], float MaxError = Tolerance)
	{
		FString Mismatches;
		for (const FInlineCase& Case : Cases)
		{
			Mismatches += DescribeInline(Layout, Case, MaxError);
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribeLines(const TArray<FArcRmlTestLayoutElement>& Layout, const FLineCase (&Cases)[N], float MaxError = Tolerance)
	{
		FString Mismatches;
		for (const FLineCase& Case : Cases)
		{
			const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Layout, Case.Id);
			if (Lines.IsEmpty())
			{
				Mismatches += FString::Printf(TEXT("[%s has no text line] "), Case.Id);
			}
			else if (FMath::Abs(Lines[0].X - Case.X) > MaxError || FMath::Abs(Lines[0].Width - Case.Width) > MaxError)
			{
				Mismatches += FString::Printf(TEXT("[%s text is x %.2f, width %.2f, Chrome x %.2f, width %.2f] "), Case.Id, Lines[0].X, Lines[0].Width, Case.X, Case.Width);
			}
		}
		return Mismatches;
	}

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= PixelTolerance && FMath::Abs(Measured.G - Expected.G) <= PixelTolerance
			&& FMath::Abs(Measured.B - Expected.B) <= PixelTolerance && FMath::Abs(Measured.A - Expected.A) <= PixelTolerance;
	}

	template <int32 N>
	FString DescribeWeights(const TArray<FArcRmlTestLayoutElement>& Layout, const FValueCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FValueCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element || Element->FontWeight != static_cast<int32>(Case.Value))
			{
				Mismatches += FString::Printf(TEXT("[%s font-weight %d, Chrome %d] "), Case.Id, Element ? Element->FontWeight : -1, static_cast<int32>(Case.Value));
			}
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribeSizes(const TArray<FArcRmlTestLayoutElement>& Layout, const FValueCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FValueCase& Case : Cases)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id);
			if (!Element || FMath::Abs(Element->FontSize - Case.Value) > SizeTolerance)
			{
				Mismatches += FString::Printf(TEXT("[%s font-size %.4f, Chrome %.4f] "), Case.Id, Element ? Element->FontSize : -1.0f, Case.Value);
			}
		}
		return Mismatches;
	}

	template <int32 N>
	FString DescribePixels(const FArcRmlTestView& View, const FChromePixel (&Pixels)[N])
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
}

TEST_CLASS(ArcRmlCssParityUserAgentTextTests, "ArcRmlUI.CssParity.UserAgentText")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(TEXT("CssParity/UserAgent/UserAgentTextTest.rml"))) && View.FlushUntilSettled()
			&& View.GetProbe().bHasDocument && View.ParseLayout(Layout);
	}

	TEST_METHOD(BolderAndLighter_ResolveAgainstTheParentWeightAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeWeights(Layout, ChromeWeights) + DescribeInlines(Layout, ChromeWeightBoxes) + DescribeLines(Layout, ChromeWeightLines),
			TEXT("bolder: below 350 is 400, below 550 is 700, below 900 is 900; lighter: below 100 unchanged, below 550 is 100, below 750 is 400, else 700")));
	}

	TEST_METHOD(InheritAndUnset_TakeTheParentsComputedValueAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(),
			DescribeWeights(Layout, ChromeInheritedWeights) + DescribeSizes(Layout, ChromeInheritedSizes) + DescribeLines(Layout, ChromeInheritedThLines),
			TEXT("inherit and unset under a bolder b, a smaller small and a centred th take the parent's computed value, not its relative keyword again")));
	}

	TEST_METHOD(RuntimeChange_RecomputesRelativeValuesAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("edited"), true);
		Layout.Reset();
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout)));
		ASSERT_THAT(AreEqual(FString(),
			DescribeWeights(Layout, ChromeEditedWeights) + DescribeSizes(Layout, ChromeEditedSizes) + DescribeInlines(Layout, ChromeEditedSizeBoxes)
				+ DescribeLines(Layout, ChromeEditedThLines),
			TEXT("font-weight 300 to 500, font-size 16 to 20px and text-align left to start after load recompute b, small, sub, sup, larger and th")));
	}

	TEST_METHOD(SmallerAndLarger_ScaleTheParentSizeByOnePointTwoAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeInlines(Layout, ChromeSizeBoxes), TEXT("small is 13.33px, small in small 11.11px, larger 19.2px, sub and sup 13.33px")));
	}

	TEST_METHOD(CodeAndPre_TakeTheGenericMonospaceFamilyAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		float WidestPreLine = 0.0f;
		for (const FArcRmlTestTextLine& Line : ArcRmlUITestHelpers::GetLines(Layout, TEXT("c-pre")))
		{
			WidestPreLine = FMath::Max(WidestPreLine, Line.Width);
		}
		FString Mismatches = DescribeInlines(Layout, ChromeMonospaceBoxes);
		if (FMath::Abs(WidestPreLine - ChromePreWidestLine) > Tolerance)
		{
			Mismatches += FString::Printf(TEXT("[c-pre widest line %.2f, Chrome %.2f] "), WidestPreLine, ChromePreWidestLine);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches, TEXT("code and pre draw in the monospace family (JetBrains Mono) at the author's 16px")));
	}

	TEST_METHOD(Th_IsCentredOnlyUnderAStartParentAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeLines(Layout, ChromeThLines), TEXT("th is centred under the initial text-align and follows an inherited left or right")));
	}

	TEST_METHOD(Hr_HasItsOnePixelInsetBorderAsChrome)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeInline(Layout, ChromeHrBox, Tolerance), TEXT("hr is 2px high: a 1px inset border on each side")));
	}
};

TEST_CLASS(ArcRmlUserAgentHrPixelTests, "ArcRmlUI.Pixels.UserAgentHr")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(InsetBorders_PaintChromesDarkTopAndLightBottom)
	{
		using namespace ArcRmlCssParityUserAgentTextTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/UserAgent/UserAgentHrPixelTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(AreEqual(FString(), DescribePixels(View, ChromeHrPixels),
			TEXT("a currentcolor inset side paints from rgb(238,238,238): top and left 154, bottom and right 238; #3366cc gives (30,60,120) and (63,127,255)")));
		ASSERT_THAT(AreEqual(FString(), DescribePixels(View, ChromeInsetPixels),
			TEXT("2px inset: #303030 darkens to black and lightens to 132; #f0f0f0 (luminance above 0.83) keeps its bottom; white darkens to 171")));
	}
};
