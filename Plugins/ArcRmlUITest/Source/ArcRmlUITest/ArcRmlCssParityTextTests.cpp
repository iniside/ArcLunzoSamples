// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityTextTestsPrivate
{
	struct FChromeWidth
	{
		const TCHAR* DomId;
		float Width;
	};

	constexpr float WidthTolerance = 0.05f;
	constexpr float DpFourThirds = 4.0f / 3.0f;
	constexpr int32 PhaseCount = 4;

	const FChromeWidth WidthsAtDpOne[] = {
		{TEXT("chip"), 67.109375f},
		{TEXT("main-hand"), 54.0f},
		{TEXT("enter"), 27.0f},
		{TEXT("title"), 114.375f},
	};

	const FChromeWidth WidthsAtDpFourThirds[] = {
		{TEXT("enter"), 36.0f},
		{TEXT("title"), 152.484375f},
	};

	const FColor PhasesColour(0x10, 0xa0, 0xd0, 0xff);
	const FColor ShifterColour(0xd0, 0x10, 0xa0, 0xff);
	const FColor CounterColour(0xa0, 0xd0, 0x10, 0xff);
	const FColor UnderControlColour(0x20, 0xd0, 0xa0, 0xff);
	const FColor UnderShiftedColour(0xd0, 0xa0, 0x20, 0xff);
	const FColor UnderLateColour(0xa0, 0x20, 0xd0, 0xff);

	const FArcRmlTestDraw* FindUnderline(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
			{
				return &Draw;
			}
		}
		return nullptr;
	}

	bool SameRect(const FBox2f& A, const FBox2f& B)
	{
		return A.Min.Equals(B.Min, 0.01f) && A.Max.Equals(B.Max, 0.01f);
	}

	FString DescribeQuads(const TArray<FArcRmlTestDrawQuad>& Quads)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestDrawQuad& Quad : Quads)
		{
			Parts.Add(FString::Printf(TEXT("x %.2f y %.2f atlas %.1f,%.1f"), Quad.X, Quad.Y, Quad.AtlasRect.Min.X, Quad.AtlasRect.Min.Y));
		}
		return FString::Printf(TEXT("%d quads: %s"), Quads.Num(), *FString::Join(Parts, TEXT("; ")));
	}
}

TEST_CLASS(ArcRmlCssParityTextTests, "ArcRmlUI.CssParity.Text")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(float DpRatio)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Text/AdvanceTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		return ArcRmlUITestHelpers::OpenView(View, Path, DpRatio) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork && View.ParseLayout(Elements);
	}

	void ExpectWidths(const ArcRmlCssParityTextTestsPrivate::FChromeWidth* Widths, int32 Count, float DpRatio)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(DpRatio), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, Widths[Index].DomId);
			ASSERT_THAT(AreEqual(1, Lines.Num(), Widths[Index].DomId));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Lines[0].Width, Widths[Index].Width, WidthTolerance),
				FString::Printf(TEXT("%s at dp %.3f is %.4f px, Chrome kerning none %.2f"), Widths[Index].DomId, DpRatio, Lines[0].Width, Widths[Index].Width)));
		}
	}

	void ExpectFullGlyphsInFollowingFrames(const FColor& Colour, int32 Expected, TArray<FArcRmlTestDrawQuad>& OutFinalQuads)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		uint64 LastSerial = View.GetProbe().FrameSerial;
		int32 NewFrames = 0;
		for (int32 Round = 0; Round < 8; ++Round)
		{
			ASSERT_THAT(IsTrue(View.Flush()));
			if (View.GetProbe().FrameSerial == LastSerial)
			{
				continue;
			}
			LastSerial = View.GetProbe().FrameSerial;
			++NewFrames;
			OutFinalQuads = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), Colour);
			ASSERT_THAT(AreEqual(Expected, OutFinalQuads.Num(), FString::Printf(TEXT("frame %llu: %s"), LastSerial, *DescribeQuads(OutFinalQuads))));
		}
		ASSERT_THAT(IsTrue(NewFrames > 0, TEXT("no frame was drawn after the change")));
	}

	TEST_METHOD(WidthsAtDpOne_MatchChromeKerningNone)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ExpectWidths(WidthsAtDpOne, UE_ARRAY_COUNT(WidthsAtDpOne), 1.0f);
	}

	TEST_METHOD(WidthsAtDpFourThirds_MatchChromeKerningNone)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ExpectWidths(WidthsAtDpFourThirds, UE_ARRAY_COUNT(WidthsAtDpFourThirds), DpFourThirds);
	}

	TEST_METHOD(RunOfI_DrawsFourPhasesWithOwnAtlasRects)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Quads = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), PhasesColour);
		ASSERT_THAT(AreEqual(2 * PhaseCount, Quads.Num(), DescribeQuads(Quads)));
		for (int32 First = 0; First < PhaseCount; ++First)
		{
			for (int32 Second = First + 1; Second < PhaseCount; ++Second)
			{
				ASSERT_THAT(IsFalse(SameRect(Quads[First].AtlasRect, Quads[Second].AtlasRect),
					FString::Printf(TEXT("phases %d and %d share an atlas rect; %s"), First, Second, *DescribeQuads(Quads))));
			}
		}
		const float PenAdvance = 6.25f;
		const float PenLeft = 20.0f;
		for (int32 Index = 0; Index < Quads.Num(); ++Index)
		{
			const float WholePixel = FMath::FloorToFloat(PenLeft + PenAdvance * static_cast<float>(Index));
			const float Bearing = Quads[Index].X - WholePixel;
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Quads[Index].X, FMath::RoundToFloat(Quads[Index].X), 0.001f),
				FString::Printf(TEXT("quad %d starts at the fractional x %.3f; %s"), Index, Quads[Index].X, *DescribeQuads(Quads))));
			ASSERT_THAT(IsTrue(FMath::Abs(Bearing - (Quads[0].X - PenLeft)) <= 1.0f,
				FString::Printf(TEXT("quad %d starts at %.2f, whole pixel of its pen %.2f; %s"), Index, Quads[Index].X, WholePixel, *DescribeQuads(Quads))));
		}
		for (int32 Index = 0; Index < PhaseCount; ++Index)
		{
			ASSERT_THAT(IsTrue(SameRect(Quads[Index].AtlasRect, Quads[Index + PhaseCount].AtlasRect),
				FString::Printf(TEXT("glyph %d and %d sit on the same phase but draw different bitmaps; %s"), Index, Index + PhaseCount, *DescribeQuads(Quads))));
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Quads[Index + PhaseCount].X - Quads[Index].X, 4.0f * 6.25f, 0.001f),
				FString::Printf(TEXT("glyph %d and %d are not four advances apart; %s"), Index, Index + PhaseCount, *DescribeQuads(Quads))));
		}
	}

	TEST_METHOD(FractionalShiftOfShownText_DrawsEveryGlyphInTheNextFrames)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Before = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), ShifterColour);
		ASSERT_THAT(AreEqual(8, Before.Num(), DescribeQuads(Before)));
		View.SetRootClass(TEXT("shift"), true);
		TArray<FArcRmlTestDrawQuad> After;
		ExpectFullGlyphsInFollowingFrames(ShifterColour, 8, After);
		ASSERT_THAT(IsTrue(After.Num() > 0, TEXT("no glyph was drawn after the shift")));
		ASSERT_THAT(IsFalse(SameRect(Before[0].AtlasRect, After[0].AtlasRect),
			FString::Printf(TEXT("the quarter-pixel shift did not change the drawn phase; before %s; after %s"), *DescribeQuads(Before), *DescribeQuads(After))));
	}

	TEST_METHOD(CentredCounterChangingPhase_DrawsEveryGlyphInTheNextFrames)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Three = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), CounterColour);
		ASSERT_THAT(AreEqual(3, Three.Num(), DescribeQuads(Three)));
		View.SetModelValue(TEXT("advance"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("count")), ArcRmlUITestHelpers::MakeString(TEXT("12")));
		TArray<FArcRmlTestDrawQuad> Two;
		ExpectFullGlyphsInFollowingFrames(CounterColour, 2, Two);
		ASSERT_THAT(IsTrue(Two.Num() > 0, TEXT("no glyph was drawn after the change to 12")));
		ASSERT_THAT(IsFalse(SameRect(Three[0].AtlasRect, Two[0].AtlasRect),
			FString::Printf(TEXT("the first glyph kept its phase; three %s; two %s"), *DescribeQuads(Three), *DescribeQuads(Two))));
		View.SetModelValue(TEXT("advance"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("count")), ArcRmlUITestHelpers::MakeString(TEXT("123")));
		TArray<FArcRmlTestDrawQuad> ThreeAgain;
		ExpectFullGlyphsInFollowingFrames(CounterColour, 3, ThreeAgain);
	}

	void ExpectUnderlineOnGlyphOrigin(const TCHAR* DomId, const FColor& Colour)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		const TArray<FArcRmlTestTextLine> ControlLines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("under-control"));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, DomId);
		ASSERT_THAT(AreEqual(1, ControlLines.Num()));
		ASSERT_THAT(AreEqual(1, Lines.Num(), DomId));
		const FArcRmlTestDraw* ControlUnderline = FindUnderline(View.GetDraws(), UnderControlColour);
		const FArcRmlTestDraw* Underline = FindUnderline(View.GetDraws(), Colour);
		ASSERT_THAT(IsNotNull(ControlUnderline));
		ASSERT_THAT(IsNotNull(Underline, DomId));
		const TArray<FArcRmlTestDrawQuad> Glyphs = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), Colour);
		ASSERT_THAT(AreEqual(8, Glyphs.Num(), DescribeQuads(Glyphs)));

		const float UnderlineOffset = ControlUnderline->Bounds.Min.Y - FMath::RoundToFloat(ControlLines[0].Baseline);
		const float PixelOrigin = FMath::FloorToFloat(FMath::RoundToFloat(Lines[0].X * 4.0f) / 4.0f);
		const float RoundedBaseline = FMath::RoundToFloat(Lines[0].Baseline);
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Underline->Bounds.Min.X, PixelOrigin, 0.001f),
			FString::Printf(TEXT("%s: the underline starts at x %.3f, the glyph pixel origin is %.3f (line x %.3f)"), DomId, Underline->Bounds.Min.X, PixelOrigin,
				Lines[0].X)));
		ASSERT_THAT(IsTrue(FMath::Abs(Glyphs[0].X - PixelOrigin) <= 1.0f,
			FString::Printf(TEXT("%s: the first glyph quad starts at x %.3f, the pixel origin is %.3f"), DomId, Glyphs[0].X, PixelOrigin)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Underline->Bounds.Min.Y, RoundedBaseline + UnderlineOffset, 0.001f),
			FString::Printf(TEXT("%s: the underline starts at y %.3f, the rounded baseline %.3f plus the control offset %.3f (unrounded baseline %.3f)"), DomId,
				Underline->Bounds.Min.Y, RoundedBaseline, UnderlineOffset, Lines[0].Baseline)));
	}

	TEST_METHOD(UnderlineAtQuarterOffset_SitsOnTheGlyphOrigin)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectUnderlineOnGlyphOrigin(TEXT("under-shifted"), UnderShiftedColour);
	}

	TEST_METHOD(UnderlineAtThreeQuarterOffset_SitsOnTheGlyphOrigin)
	{
		using namespace ArcRmlCssParityTextTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectUnderlineOnGlyphOrigin(TEXT("under-late"), UnderLateColour);
	}
};

namespace ArcRmlCssParityShapingTestsPrivate
{
	constexpr float WidthTolerance = 0.05f;
	constexpr float DpFourThirds = 4.0f / 3.0f;
	constexpr float SecondBlockTop = 280.0f;

	const ArcRmlCssParityTextTestsPrivate::FChromeWidth HeadersAtDpOne[] = {
		{TEXT("hdr-compat"), 131.046875f},
		{TEXT("hdr-item"), 97.59375f},
		{TEXT("hdr-storage"), 114.3125f},
		{TEXT("hdr-inv"), 79.203125f},
	};

	const ArcRmlCssParityTextTestsPrivate::FChromeWidth HeadersAtDpFourThirds[] = {
		{TEXT("hdr-compat"), 174.734375f},
		{TEXT("hdr-item"), 130.125f},
		{TEXT("hdr-storage"), 152.421875f},
		{TEXT("hdr-inv"), 105.609375f},
	};

	const ArcRmlCssParityTextTestsPrivate::FChromeWidth ScaledAtDpFourThirds[] = {
		{TEXT("sz-shoulders"), 64.484375f},
		{TEXT("sz-main-hand"), 77.3125f},
		{TEXT("sz-count"), 55.984375f},
		{TEXT("sz-kerned"), 156.703125f},
	};

	bool IsFiniteRect(const FArcRmlInspectorRect& Rect)
	{
		return FMath::IsFinite(Rect.Position.X) && FMath::IsFinite(Rect.Position.Y) && FMath::IsFinite(Rect.Size.X) && FMath::IsFinite(Rect.Size.Y);
	}

	const FColor MonoColour(0x10, 0xa0, 0xd0, 0xff);
	const FColor FallbackColour(0xd0, 0x10, 0xa0, 0xff);
	const FColor SeparatorColour(0x20, 0x70, 0xd0, 0xff);
	const FColor LigatureColour(0xa0, 0xd0, 0x10, 0xff);
	const FColor SpacedColour(0x20, 0xd0, 0xa0, 0xff);
	const FColor ZeroWidthColour(0xd0, 0xa0, 0x20, 0xff);
	const FColor ZeroWidthControlColour(0xa0, 0x20, 0xd0, 0xff);
	const FColor ShadowColour(0xff, 0x00, 0x00, 0xff);
	const FColor OutlineColour(0x00, 0xff, 0x00, 0xff);
	const FColor FillColour(0xff, 0xff, 0xff, 0xff);
	const FColor NormalFillColour(0x2a, 0x7f, 0xd4, 0xff);
	const FColor NormalStrokeColour(0xd4, 0x34, 0x2a, 0xff);
	const FColor StrokeFirstFillColour(0xd4, 0xb0, 0x2a, 0xff);
	const FColor StrokeFirstStrokeColour(0x2a, 0xd4, 0x8c, 0xff);
	const FColor DmgCurrentColour(0x6b, 0xa3, 0xd3, 0xff);
	const float InkTolerance = 2.0f;

	struct FChromeInk
	{
		FColor Colour;
		FBox2f Box;
	};

	const FChromeInk StrokeInks[] = {
		{NormalStrokeColour, FBox2f(FVector2f(40.0f, 48.0f), FVector2f(117.0f, 75.0f))},
		{StrokeFirstStrokeColour, FBox2f(FVector2f(40.0f, 128.0f), FVector2f(116.0f, 155.0f))},
	};

	bool IsTextured(const FArcRmlTestDraw& Draw, const FColor& Colour)
	{
		return Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bTextured && Draw.FirstColourPremultiplied == Colour;
	}

	int32 FindLast(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		int32 Found = INDEX_NONE;
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (IsTextured(Draws[Index], Colour))
			{
				Found = Index;
			}
		}
		return Found;
	}

	int32 FindFirstFill(const TArray<FArcRmlTestDraw>& Draws, bool bSecondBlock)
	{
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (IsTextured(Draws[Index], FillColour) && (Draws[Index].Bounds.Min.Y >= SecondBlockTop) == bSecondBlock)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

TEST_CLASS(ArcRmlCssParityShapingTests, "ArcRmlUI.CssParity.TextShaping")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Name, float DpRatio)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("CssParity/Text/%s.rml"), Name));
		FArcRmlTestView::DropCompiledDocument(Path);
		return ArcRmlUITestHelpers::OpenView(View, Path, DpRatio) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork && View.ParseLayout(Elements);
	}

	void ExpectWidth(const TCHAR* DomId, float Chrome, float Tolerance = ArcRmlCssParityShapingTestsPrivate::WidthTolerance)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, DomId);
		ASSERT_THAT(AreEqual(1, Lines.Num(), DomId));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Lines[0].Width, Chrome, Tolerance),
			FString::Printf(TEXT("%s is %.4f px, Chrome %.4f"), DomId, Lines[0].Width, Chrome)));
	}

	void ExpectWidths(const ArcRmlCssParityTextTestsPrivate::FChromeWidth* Widths, int32 Count, float DpRatio, float Tolerance)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), DpRatio), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			ExpectWidth(Widths[Index].DomId, Widths[Index].Width, Tolerance);
		}
	}

	void ExpectQuads(const FColor& Colour, int32 Expected, const TCHAR* What)
	{
		const TArray<FArcRmlTestDrawQuad> Quads = ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), Colour);
		int32 Outlined = 0;
		FString Sizes;
		for (const FArcRmlTestDrawQuad& Quad : Quads)
		{
			Outlined += (Quad.Width > 0.0f && Quad.Height > 0.0f) ? 1 : 0;
			Sizes += FString::Printf(TEXT(" %.1fx%.1f"), Quad.Width, Quad.Height);
		}
		ASSERT_THAT(AreEqual(Expected, Outlined, FString::Printf(TEXT("%s; sizes%s"), What, *Sizes)));
	}

	TEST_METHOD(HeadersAtDpOne_MatchChromeKerningNormal)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ExpectWidths(HeadersAtDpOne, UE_ARRAY_COUNT(HeadersAtDpOne), 1.0f, WidthTolerance);
	}

	TEST_METHOD(HeadersAtDpFourThirds_MatchChromeKerningNormal)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ExpectWidths(HeadersAtDpFourThirds, UE_ARRAY_COUNT(HeadersAtDpFourThirds), DpFourThirds, WidthTolerance);
	}

	TEST_METHOD(KerningNormalAndNone_MatchChromeWidths)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("kern-on"), 204.171875f);
		ExpectWidth(TEXT("kern-off"), 214.453125f);
	}

	TEST_METHOD(TwelveAndTenPixelSizesInOneDocument_EachMatchChrome)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("s12"), 119.59375f);
		ExpectWidth(TEXT("s10"), 99.65625f);
	}

	TEST_METHOD(MonoArrowAndNotEqual_MatchChromeAndDrawHarfBuzzGlyphs)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("mono"), 132.0f);
		ExpectQuads(MonoColour, 5, TEXT("a, -> ligature, b, != ligature, c are the five outlined glyphs HarfBuzz returns"));
	}

	TEST_METHOD(FallbackOnlyCharacter_MatchesChromeAndDrawsThreeGlyphs)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("fallback"), 38.8125f);
		ExpectQuads(FallbackColour, 3, TEXT("A, the fallback-face glyph, A"));
	}

	TEST_METHOD(NarrowNoBreakSpace_IsHalfASpaceAndDrawsNoReplacementGlyph)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("nnbsp"), 38.0625f);
		ExpectQuads(SeparatorColour, 4, TEXT("the four digits only, no replacement glyph for U+202F"));
	}

	TEST_METHOD(LetterSpacing_MatchesChromeAndDisablesLigatures)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("lig"), 65.046875f);
		ExpectWidth(TEXT("lig-spaced"), 78.078125f);
		ExpectQuads(LigatureColour, 4, TEXT("fi, fl, ffi, ffl as four ligature glyphs"));
		ExpectQuads(SpacedColour, 10, TEXT("letter-spacing 1px turns ligatures off, so ten letters"));
	}

	TEST_METHOD(ZeroWidthSpaceInMono_DoesNotSplitShapingIntoAFallbackSegment)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ShapingTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("zw"), 38.40625f);
		ExpectWidth(TEXT("zw-control"), 38.40625f);
		ExpectQuads(ZeroWidthControlColour, 3, TEXT("a, the -> ligature, b"));
		ExpectQuads(ZeroWidthColour, 3, TEXT("the ligature survives U+200B, so the text was shaped as one segment"));
	}

	TEST_METHOD(ScaledPixelTextAtDpFourThirds_MatchesChrome)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ScaleTest"), DpFourThirds), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const ArcRmlCssParityTextTestsPrivate::FChromeWidth& Expected : ScaledAtDpFourThirds)
		{
			ExpectWidth(Expected.DomId, Expected.Width);
		}
	}

	TEST_METHOD(LargeFontSize_MatchesChromeWidthPastFiveHundredTwelvePixels)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ScaleTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("big-single"), 540.0f);
		ExpectWidth(TEXT("big-pair"), 768.0f);
	}

	TEST_METHOD(ZeroFontSize_StaysFiniteAndAdvancesByLetterSpacingOnly)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("ScaleTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			ASSERT_THAT(IsTrue(IsFiniteRect(Element.Rect), FString::Printf(TEXT("%s has a non-finite box"), *Element.DomId)));
			for (const FArcRmlTestTextLine& Line : Element.Lines)
			{
				ASSERT_THAT(IsTrue(FMath::IsFinite(Line.X) && FMath::IsFinite(Line.Baseline) && FMath::IsFinite(Line.Width),
					FString::Printf(TEXT("%s has a non-finite line"), *Element.DomId)));
			}
		}
		ExpectWidth(TEXT("zero-spaced"), 6.0f);
		ExpectWidth(TEXT("zero-plain"), 0.0f);
	}

#if WITH_EDITOR
	TEST_METHOD(WoffFace_IsRejectedWhenLoadedAndTextFallsBack)
	{
		TestRunner->AddExpectedErrorPlain(TEXT("HarfBuzz cannot read font face"), EAutomationExpectedErrorFlags::Contains, 2);
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("WoffTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FArcRmlTestView::ReloadFonts({TEXT("/ArcRmlUITest/UI/Rml/Fonts/ArcWoffTest.woff")});
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));
		const TArray<FArcRmlTestTextLine> Woff = ArcRmlUITestHelpers::GetLines(Elements, TEXT("woff"));
		const TArray<FArcRmlTestTextLine> Control = ArcRmlUITestHelpers::GetLines(Elements, TEXT("control"));
		ASSERT_THAT(AreEqual(1, Woff.Num()));
		ASSERT_THAT(AreEqual(1, Control.Num()));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Woff[0].Width, Control[0].Width, 0.001f),
			FString::Printf(TEXT("the rejected face is not used: woff text %.4f px, Space Grotesk fallback %.4f px"), Woff[0].Width, Control[0].Width)));
		ASSERT_THAT(IsTrue(!FMath::IsNearlyEqual(Woff[0].Width, 4.0f * 9.6f, 0.5f),
			FString::Printf(TEXT("the text took the JetBrains Mono advance (4 x 9.6 px) of the rejected face: %.4f px"), Woff[0].Width)));
	}

	TEST_METHOD(TrueTypeFace_StillLoadsAfterAFontReload)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("WoffTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("mono"), 132.0f);
		FArcRmlTestView::ReloadFonts({TEXT("/ArcRmlUITest/UI/Rml/Fonts/JetBrainsMono-Regular.ttf")});
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));
		ExpectWidth(TEXT("mono"), 132.0f);
	}
#endif

	TEST_METHOD(ShadowAndFallbackGlyph_EveryShadowDrawPrecedesEveryFillDraw)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DrawOrderTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 LastShadow = FindLast(Draws, ShadowColour);
		const int32 FirstFill = FindFirstFill(Draws, false);
		ASSERT_THAT(IsTrue(LastShadow != INDEX_NONE, TEXT("no shadow draw")));
		ASSERT_THAT(IsTrue(FirstFill != INDEX_NONE, TEXT("no fill draw")));
		ASSERT_THAT(IsTrue(LastShadow < FirstFill, FString::Printf(TEXT("last shadow draw %d, first fill draw %d of %d"), LastShadow, FirstFill, Draws.Num())));
	}

	void ExpectStrokeOrder(const FColor& Fill, const FColor& Stroke, bool bStrokeOverFill)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("PaintOrderTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 FillDraw = FindLast(Draws, Fill);
		const int32 StrokeDraw = FindLast(Draws, Stroke);
		ASSERT_THAT(IsTrue(FillDraw != INDEX_NONE, TEXT("no fill draw")));
		ASSERT_THAT(IsTrue(StrokeDraw != INDEX_NONE, TEXT("no stroke draw")));
		ASSERT_THAT(IsTrue((StrokeDraw > FillDraw) == bStrokeOverFill,
			FString::Printf(TEXT("stroke draw %d, fill draw %d; the stroke must draw %s the fill"), StrokeDraw, FillDraw, bStrokeOverFill ? TEXT("over") : TEXT("under"))));
	}

	TEST_METHOD(PaintOrderNormal_DrawsTheStrokeOverTheFill)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ExpectStrokeOrder(NormalFillColour, NormalStrokeColour, true);
	}

	TEST_METHOD(PaintOrderStrokeFill_DrawsTheStrokeUnderTheFill)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ExpectStrokeOrder(StrokeFirstFillColour, StrokeFirstStrokeColour, false);
	}

	TEST_METHOD(TextStroke_InkBoxMatchesChrome)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("PaintOrderTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		for (const FChromeInk& Ink : StrokeInks)
		{
			const int32 Index = FindLast(Draws, Ink.Colour);
			ASSERT_THAT(IsTrue(Index != INDEX_NONE, TEXT("no stroke draw")));
			const FBox2f& Box = Draws[Index].Bounds;
			const FString Context = FString::Printf(TEXT("stroke ink %.1f,%.1f to %.1f,%.1f; Chrome %.1f,%.1f to %.1f,%.1f"),
				Box.Min.X, Box.Min.Y, Box.Max.X, Box.Max.Y, Ink.Box.Min.X, Ink.Box.Min.Y, Ink.Box.Max.X, Ink.Box.Max.Y);
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Box.Min.X, Ink.Box.Min.X, InkTolerance) && FMath::IsNearlyEqual(Box.Min.Y, Ink.Box.Min.Y, InkTolerance)
				&& FMath::IsNearlyEqual(Box.Max.X, Ink.Box.Max.X, InkTolerance) && FMath::IsNearlyEqual(Box.Max.Y, Ink.Box.Max.Y, InkTolerance), Context));
		}
	}

	TEST_METHOD(CurrentColourStrokeWithoutPaintOrder_DrawsOverTheFill)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		const FString Path = TEXT("/ArcRmlUITest/UI/Rml/Test/CssGroup2Test.rml");
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path, 1.0f) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		TArray<int32> Found;
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (IsTextured(Draws[Index], DmgCurrentColour))
			{
				Found.Add(Index);
			}
		}
		ASSERT_THAT(AreEqual(2, Found.Num(), TEXT(".dmg-current draws its fill and its stroke")));
		const FBox2f& First = Draws[Found[0]].Bounds;
		const FBox2f& Second = Draws[Found[1]].Bounds;
		ASSERT_THAT(IsTrue(Second.Min.X < First.Min.X && Second.Max.X > First.Max.X,
			FString::Printf(TEXT("the second draw (%.1f to %.1f) is not the stroke around the first (%.1f to %.1f)"), Second.Min.X, Second.Max.X, First.Min.X, First.Max.X)));
	}

	TEST_METHOD(OutlineOnWrappedText_EveryOutlineDrawPrecedesEveryFillDraw)
	{
		using namespace ArcRmlCssParityShapingTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("DrawOrderTest"), 1.0f), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(2, ArcRmlUITestHelpers::GetLines(Elements, TEXT("lines")).Num(), TEXT("the text wraps to two lines")));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 LastOutline = FindLast(Draws, OutlineColour);
		const int32 FirstFill = FindFirstFill(Draws, true);
		ASSERT_THAT(IsTrue(LastOutline != INDEX_NONE, TEXT("no outline draw")));
		ASSERT_THAT(IsTrue(FirstFill != INDEX_NONE, TEXT("no fill draw")));
		ASSERT_THAT(IsTrue(LastOutline < FirstFill, FString::Printf(TEXT("last outline draw %d, first fill draw %d of %d"), LastOutline, FirstFill, Draws.Num())));
	}
};

namespace ArcRmlCssParityTextWrapTestsPrivate
{
	struct FChromeLine
	{
		const TCHAR* Path;
		const TCHAR* Text;
		float X;
		float Width;
		float Top;
		bool bLoose;
	};

	struct FChromeCase
	{
		const TCHAR* DomId;
		const FChromeLine* Lines;
		int32 NumLines;
	};

	struct FPatternCase
	{
		const TCHAR* JustifiedId;
		const TCHAR* LeftId;
		const TCHAR* Suffixes;
		const TCHAR* Pattern;
	};

	struct FLineCountPair
	{
		const TCHAR* PrettyId;
		const TCHAR* GreedyId;
	};

	constexpr float WidthTolerance = 0.05f;
	constexpr float PositionTolerance = 0.15f;
	constexpr float LooseTolerance = 1.0f;
	constexpr float TopTolerance = 0.6f;
	constexpr float SpreadThreshold = 0.01f;

	const FLineCountPair PrettyPairs[] = {
		{TEXT("hint-pretty"), TEXT("hint-greedy")},
		{TEXT("drag-pretty"), TEXT("drag-greedy")},
		{TEXT("p1-pretty"), TEXT("p1-greedy")},
		{TEXT("p2-pretty"), TEXT("p2-greedy")},
		{TEXT("crafts-pretty"), TEXT("crafts-greedy")},
		{TEXT("long-pretty"), TEXT("long-greedy")},
	};

	const FLineCountPair CasePairs[] = {
		{TEXT("prewrap-pretty"), TEXT("prewrap-greedy")},
		{TEXT("nowrap-pretty"), TEXT("nowrap-greedy")},
		{TEXT("breakall-pretty"), TEXT("breakall-greedy")},
		{TEXT("elemb-pretty"), TEXT("elemb-greedy")},
		{TEXT("fit-pretty"), TEXT("fit-greedy")},
		{TEXT("nwfor-pretty"), TEXT("nwfor-greedy")},
		{TEXT("indent-pretty"), TEXT("indent-greedy")},
		{TEXT("leadfloat-pretty"), TEXT("leadfloat-greedy")},
		{TEXT("openfloat-pretty"), TEXT("openfloat-greedy")},
	};

	const FPatternCase PatternCases[] = {
		{TEXT("tj-auto"), TEXT("tjl-auto"), TEXT("0.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 11.0"), TEXT("-G -- I- I- -- I- -- -- -G -")},
		{TEXT("tj-none"), TEXT("tjl-none"), TEXT("0.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 11.0"), TEXT("-- -- -- -- -- -- -- -- -- -")},
		{TEXT("tj-word"), TEXT("tjl-word"), TEXT("0.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 11.0"), TEXT("-G -- -- -- -- I- -- -- -G -")},
		{TEXT("tj-char"), TEXT("tjl-char"), TEXT("0.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 11.0"), TEXT("IG I- I- I- I- I- I- -- IG I")},
		{TEXT("tj-dist"), TEXT("tjl-dist"), TEXT("0.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 11.0"), TEXT("IG I- I- I- I- I- I- -- IG I")},
		{TEXT("tj-inherit"), TEXT("tjl-inherit"), TEXT("0.0 2.0 4.0 5.0 7.0"), TEXT("IG -G I- IG I")},
		{TEXT("tj-cursive"), TEXT("tjl-cursive"), TEXT("0.0 1.0 2.0 4.0 5.0 6.0 7.0"), TEXT("I- -- IG I- -- I- I")},
		{TEXT("tj-cursive-auto"), TEXT("tjl-cursive-auto"), TEXT("0.0 1.0 2.0 4.0 5.0 6.0 7.0"), TEXT("-- -- -G -- -- -- -")},
		{TEXT("je-justify"), TEXT("je-left"), TEXT("0.0 2.0 4.0 6.0 8.0 10.0 12.0 14.0 16.0 18.0 20.0 22.0 24.0 26.0 28.0"), TEXT("-G IG -G IG -G IG -G IG -G -G -G IG -G IG -")},
	};

	const FChromeLine Lines_hint_pretty[] = {
		{TEXT("0.0.1.0"), TEXT("Click any"), 35.0938f, 51.8125f, 10.0f, false},
		{TEXT("0.0.1.0"), TEXT("slot to see"), 30.2969f, 61.3906f, 26.0f, false},
		{TEXT("0.0.1.0"), TEXT("compatible"), 28.2969f, 65.3906f, 42.0f, false},
		{TEXT("0.0.1.0"), TEXT("gear"), 47.8750f, 26.2344f, 58.0f, false},
	};

	const FChromeLine Lines_hint_greedy[] = {
		{TEXT("0.0.3.0"), TEXT("Click any slot"), 122.5625f, 76.8594f, 10.0f, false},
		{TEXT("0.0.3.0"), TEXT("to see"), 142.8281f, 36.3438f, 26.0f, false},
		{TEXT("0.0.3.0"), TEXT("compatible"), 128.2969f, 65.3906f, 42.0f, false},
		{TEXT("0.0.3.0"), TEXT("gear"), 147.8750f, 26.2344f, 58.0f, false},
	};

	const FChromeLine Lines_drag_pretty[] = {
		{TEXT("0.0.5.0"), TEXT("Drag items here to"), 241.7188f, 106.5625f, 10.0f, false},
		{TEXT("0.0.5.0"), TEXT("store them"), 263.1406f, 63.7188f, 26.0f, false},
	};

	const FChromeLine Lines_drag_greedy[] = {
		{TEXT("0.0.7.0"), TEXT("Drag items here to store"), 375.0156f, 139.9688f, 10.0f, false},
		{TEXT("0.0.7.0"), TEXT("them"), 429.8438f, 30.3125f, 26.0f, false},
	};

	const FChromeLine Lines_p1_pretty[] = {
		{TEXT("0.0.9.0"), TEXT("The caravan reached the old bridge at dusk,"), 20.0000f, 339.7500f, 80.0f, false},
		{TEXT("0.0.9.0"), TEXT("and the guards waved it through without asking"), 20.0000f, 373.0781f, 101.0f, false},
		{TEXT("0.0.9.0"), TEXT("for papers."), 20.0000f, 84.3594f, 122.0f, false},
	};

	const FChromeLine Lines_p1_greedy[] = {
		{TEXT("0.0.11.0"), TEXT("The caravan reached the old bridge at dusk, and"), 420.0000f, 372.6406f, 80.0f, false},
		{TEXT("0.0.11.0"), TEXT("the guards waved it through without asking for"), 420.0000f, 366.3750f, 101.0f, false},
		{TEXT("0.0.11.0"), TEXT("papers."), 420.0000f, 57.6406f, 122.0f, false},
	};

	const FChromeLine Lines_p2_pretty[] = {
		{TEXT("0.0.13.0"), TEXT("Gather enough iron ore and"), 20.0000f, 184.7969f, 170.0f, false},
		{TEXT("0.0.13.0"), TEXT("coal to forge a stronger"), 20.0000f, 161.0781f, 188.0f, false},
		{TEXT("0.0.13.0"), TEXT("blade before the next"), 20.0000f, 147.7031f, 206.0f, false},
		{TEXT("0.0.13.0"), TEXT("storm season arrives at"), 20.0000f, 157.4688f, 224.0f, false},
		{TEXT("0.0.13.0"), TEXT("the coast."), 20.0000f, 69.1406f, 242.0f, false},
	};

	const FChromeLine Lines_p2_greedy[] = {
		{TEXT("0.0.15.0"), TEXT("Gather enough iron ore and"), 230.0000f, 184.7969f, 170.0f, false},
		{TEXT("0.0.15.0"), TEXT("coal to forge a stronger"), 230.0000f, 161.0781f, 188.0f, false},
		{TEXT("0.0.15.0"), TEXT("blade before the next"), 230.0000f, 147.7031f, 206.0f, false},
		{TEXT("0.0.15.0"), TEXT("storm season arrives at the"), 230.0000f, 184.3750f, 224.0f, false},
		{TEXT("0.0.15.0"), TEXT("coast."), 230.0000f, 42.2344f, 242.0f, false},
	};

	const FChromeLine Lines_p1_noop[] = {
		{TEXT("0.0.17.0"), TEXT("The caravan reached the old bridge at"), 20.0000f, 294.4844f, 380.0f, false},
		{TEXT("0.0.17.0"), TEXT("dusk, and the guards waved it through"), 20.0000f, 298.4688f, 401.0f, false},
		{TEXT("0.0.17.0"), TEXT("without asking for papers."), 20.0000f, 203.7031f, 422.0f, false},
	};

	const FChromeLine Lines_crafts_pretty[] = {
		{TEXT("0.0.19.0"), TEXT("Completed crafts"), 1011.9219f, 101.1562f, 10.0f, false},
		{TEXT("0.0.19.0"), TEXT("appear here"), 1027.5781f, 69.8438f, 26.0f, false},
		{TEXT("0.0.19.0"), TEXT("to collect"), 1034.5781f, 55.8438f, 42.0f, false},
	};

	const FChromeLine Lines_crafts_greedy[] = {
		{TEXT("0.0.21.0"), TEXT("Completed crafts"), 1161.9219f, 101.1562f, 10.0f, false},
		{TEXT("0.0.21.0"), TEXT("appear here to"), 1169.6719f, 85.6562f, 26.0f, false},
		{TEXT("0.0.21.0"), TEXT("collect"), 1192.4688f, 40.0469f, 42.0f, false},
	};

	const FChromeLine Lines_p3_pretty[] = {
		{TEXT("0.0.23.0"), TEXT("The caravan reached the"), 20.0000f, 195.7969f, 280.0f, false},
		{TEXT("0.0.23.1.0"), TEXT("old bridge"), 215.7969f, 78.5312f, 280.0f, false},
		{TEXT("0.0.23.2"), TEXT("at dusk,"), 294.3281f, 65.4375f, 280.0f, false},
		{TEXT("0.0.23.2"), TEXT("and the guards waved it through without asking"), 20.0000f, 373.0781f, 301.0f, false},
		{TEXT("0.0.23.2"), TEXT("for papers."), 20.0000f, 84.3594f, 322.0f, false},
	};

	const FChromeLine Lines_long_pretty[] = {
		{TEXT("0.0.25.0"), TEXT("The caravan reached the"), 440.0000f, 167.7344f, 170.0f, false},
		{TEXT("0.0.25.0"), TEXT("old bridge at dusk, and the"), 440.0000f, 181.6562f, 188.0f, false},
		{TEXT("0.0.25.0"), TEXT("guards waved it through"), 440.0000f, 165.8594f, 206.0f, false},
		{TEXT("0.0.25.0"), TEXT("without asking for papers,"), 440.0000f, 177.7500f, 224.0f, false},
		{TEXT("0.0.25.0"), TEXT("then the merchants"), 440.0000f, 134.9688f, 242.0f, false},
		{TEXT("0.0.25.0"), TEXT("unloaded their carts"), 440.0000f, 138.1406f, 260.0f, false},
		{TEXT("0.0.25.0"), TEXT("in the square before"), 440.0000f, 137.2969f, 278.0f, false},
		{TEXT("0.0.25.0"), TEXT("nightfall came."), 440.0000f, 100.1719f, 296.0f, false},
	};

	const FChromeLine Lines_long_greedy[] = {
		{TEXT("0.0.27.0"), TEXT("The caravan reached the"), 660.0000f, 167.7344f, 170.0f, false},
		{TEXT("0.0.27.0"), TEXT("old bridge at dusk, and the"), 660.0000f, 181.6562f, 188.0f, false},
		{TEXT("0.0.27.0"), TEXT("guards waved it through"), 660.0000f, 165.8594f, 206.0f, false},
		{TEXT("0.0.27.0"), TEXT("without asking for papers,"), 660.0000f, 177.7500f, 224.0f, false},
		{TEXT("0.0.27.0"), TEXT("then the merchants"), 660.0000f, 134.9688f, 242.0f, false},
		{TEXT("0.0.27.0"), TEXT("unloaded their carts in the"), 660.0000f, 180.6562f, 260.0f, false},
		{TEXT("0.0.27.0"), TEXT("square before nightfall"), 660.0000f, 154.7500f, 278.0f, false},
		{TEXT("0.0.27.0"), TEXT("came."), 660.0000f, 40.2031f, 296.0f, false},
	};

	const FChromeLine Lines_pre_nowrap[] = {
		{TEXT("0.0.29.0"), TEXT("first line"), 880.0000f, 63.3281f, 280.0f, false},
		{TEXT("0.0.29.0"), TEXT("second line that is long"), 880.0000f, 180.5000f, 301.0f, false},
	};

	const FChromeLine Lines_nowrap[] = {
		{TEXT("0.0.31.0"), TEXT("a long line that never wraps at all"), 880.0000f, 254.5781f, 340.0f, false},
	};

	const FChromeLine Lines_wrap_only[] = {
		{TEXT("0.0.33.0"), TEXT("a long line"), 880.0000f, 78.4375f, 380.0f, false},
		{TEXT("0.0.33.0"), TEXT("that does"), 880.0000f, 75.4375f, 401.0f, false},
		{TEXT("0.0.33.0"), TEXT("wrap here"), 880.0000f, 76.7031f, 422.0f, false},
	};

	const FChromeCase PrettyParagraphCases[] = {
		{TEXT("hint-pretty"), Lines_hint_pretty, UE_ARRAY_COUNT(Lines_hint_pretty)},
		{TEXT("hint-greedy"), Lines_hint_greedy, UE_ARRAY_COUNT(Lines_hint_greedy)},
		{TEXT("drag-pretty"), Lines_drag_pretty, UE_ARRAY_COUNT(Lines_drag_pretty)},
		{TEXT("drag-greedy"), Lines_drag_greedy, UE_ARRAY_COUNT(Lines_drag_greedy)},
		{TEXT("p1-pretty"), Lines_p1_pretty, UE_ARRAY_COUNT(Lines_p1_pretty)},
		{TEXT("p1-greedy"), Lines_p1_greedy, UE_ARRAY_COUNT(Lines_p1_greedy)},
		{TEXT("p2-pretty"), Lines_p2_pretty, UE_ARRAY_COUNT(Lines_p2_pretty)},
		{TEXT("p2-greedy"), Lines_p2_greedy, UE_ARRAY_COUNT(Lines_p2_greedy)},
		{TEXT("p1-noop"), Lines_p1_noop, UE_ARRAY_COUNT(Lines_p1_noop)},
		{TEXT("crafts-pretty"), Lines_crafts_pretty, UE_ARRAY_COUNT(Lines_crafts_pretty)},
		{TEXT("crafts-greedy"), Lines_crafts_greedy, UE_ARRAY_COUNT(Lines_crafts_greedy)},
		{TEXT("p3-pretty"), Lines_p3_pretty, UE_ARRAY_COUNT(Lines_p3_pretty)},
		{TEXT("long-pretty"), Lines_long_pretty, UE_ARRAY_COUNT(Lines_long_pretty)},
		{TEXT("long-greedy"), Lines_long_greedy, UE_ARRAY_COUNT(Lines_long_greedy)},
		{TEXT("pre-nowrap"), Lines_pre_nowrap, UE_ARRAY_COUNT(Lines_pre_nowrap)},
		{TEXT("nowrap"), Lines_nowrap, UE_ARRAY_COUNT(Lines_nowrap)},
		{TEXT("wrap-only"), Lines_wrap_only, UE_ARRAY_COUNT(Lines_wrap_only)},
	};

	const FChromeLine Lines_brfloat_pretty[] = {
		{TEXT("0.0.13.0"), TEXT("The caravan reached the old bridge at dusk, and"), 600.0000f, 372.6406f, 130.0f, false},
		{TEXT("0.0.13.0"), TEXT("the guards waved it through without asking for"), 600.0000f, 366.3750f, 151.0f, false},
		{TEXT("0.0.13.0"), TEXT("papers."), 600.0000f, 57.6406f, 172.0f, false},
		{TEXT("0.0.13.2"), TEXT("more"), 600.0000f, 38.8750f, 193.0f, false},
		{TEXT("0.0.13.3.0"), TEXT("x"), 970.5000f, 9.5000f, 193.0f, false},
	};

	const FChromeLine Lines_ibfloat_pretty[] = {
		{TEXT("0.0.15.1"), TEXT("The caravan reached the old bridge at dusk,"), 1040.0000f, 339.2188f, 130.0f, false},
		{TEXT("0.0.15.1"), TEXT("and the guards waved it"), 1040.0000f, 191.0625f, 151.0f, false},
		{TEXT("0.0.15.2.0"), TEXT("through"), 1231.0625f, 62.6875f, 151.0f, false},
		{TEXT("0.0.15.3"), TEXT("without"), 1293.7500f, 65.0625f, 151.0f, false},
		{TEXT("0.0.15.3"), TEXT("asking for papers."), 1000.0000f, 138.6406f, 172.0f, false},
	};

	const FChromeLine Lines_ib_pretty[] = {
		{TEXT("0.0.17.0"), TEXT("The caravan reached the old bridge at dusk,"), 1000.0000f, 339.7500f, 230.0f, false},
		{TEXT("0.0.17.0"), TEXT("and the guards waved it"), 1000.0000f, 191.0625f, 251.0f, false},
		{TEXT("0.0.17.1.0"), TEXT("through"), 1191.0625f, 62.6875f, 251.0f, false},
		{TEXT("0.0.17.2"), TEXT("without asking"), 1253.7500f, 119.3594f, 251.0f, false},
		{TEXT("0.0.17.2"), TEXT("for papers."), 1000.0000f, 84.3594f, 272.0f, false},
	};

	const FChromeLine Lines_leadfloat_pretty[] = {
		{TEXT("0.0.19.1"), TEXT("The caravan reached the old"), 50.0000f, 219.9062f, 260.0f, false},
		{TEXT("0.0.19.1"), TEXT("bridge at dusk, and the guards"), 50.0000f, 237.2344f, 281.0f, false},
		{TEXT("0.0.19.1"), TEXT("waved it through without asking"), 10.0000f, 251.0469f, 302.0f, false},
		{TEXT("0.0.19.1"), TEXT("for papers."), 10.0000f, 84.3594f, 323.0f, false},
	};

	const FChromeLine Lines_leadfloat_greedy[] = {
		{TEXT("0.0.21.1"), TEXT("The caravan reached the old"), 370.0000f, 219.9062f, 260.0f, false},
		{TEXT("0.0.21.1"), TEXT("bridge at dusk, and the guards"), 370.0000f, 237.2344f, 281.0f, false},
		{TEXT("0.0.21.1"), TEXT("waved it through without asking for"), 330.0000f, 277.7656f, 302.0f, false},
		{TEXT("0.0.21.1"), TEXT("papers."), 330.0000f, 57.6406f, 323.0f, false},
	};

	const FChromeLine Lines_sibfloat[] = {
		{TEXT("0.0.23.1.0"), TEXT("The caravan reached the old"), 690.0000f, 219.9062f, 260.0f, false},
		{TEXT("0.0.23.1.0"), TEXT("bridge at dusk, and the guards"), 690.0000f, 237.2344f, 281.0f, false},
		{TEXT("0.0.23.1.0"), TEXT("waved it through without asking"), 650.0000f, 251.0469f, 302.0f, false},
		{TEXT("0.0.23.1.0"), TEXT("for papers."), 650.0000f, 84.3594f, 323.0f, false},
	};

	const FChromeLine Lines_midfloat_pretty[] = {
		{TEXT("0.0.25.0"), TEXT("The caravan reached the old"), 50.0000f, 219.9062f, 380.0f, false},
		{TEXT("0.0.25.2"), TEXT("bridge at dusk, and the guards"), 50.0000f, 237.2344f, 401.0f, false},
		{TEXT("0.0.25.2"), TEXT("waved it through without asking"), 10.0000f, 251.0469f, 422.0f, false},
		{TEXT("0.0.25.2"), TEXT("for papers."), 10.0000f, 84.3594f, 443.0f, false},
	};

	const FChromeLine Lines_tallfloat_pretty[] = {
		{TEXT("0.0.27.1"), TEXT("The caravan reached the old bridge at dusk,"), 370.0000f, 339.7500f, 380.0f, false},
		{TEXT("0.0.27.1"), TEXT("and the guards waved it through without asking"), 370.0000f, 373.0781f, 401.0f, false},
		{TEXT("0.0.27.1"), TEXT("for papers."), 370.0000f, 84.3594f, 422.0f, false},
	};

	const FChromeLine Lines_openfloat_pretty[] = {
		{TEXT("0.0.49.0.1"), TEXT("The caravan reached the old"), 640.0000f, 219.9062f, 640.0f, false},
		{TEXT("0.0.49.0.1"), TEXT("bridge at dusk, and the guards"), 640.0000f, 237.2344f, 661.0f, false},
		{TEXT("0.0.49.0.1"), TEXT("waved it through without asking"), 600.0000f, 251.0469f, 682.0f, false},
		{TEXT("0.0.49.0.1"), TEXT("for papers."), 600.0000f, 84.3594f, 703.0f, false},
	};

	const FChromeLine Lines_openfloat_greedy[] = {
		{TEXT("0.0.51.0.1"), TEXT("The caravan reached the old"), 990.0000f, 219.9062f, 640.0f, false},
		{TEXT("0.0.51.0.1"), TEXT("bridge at dusk, and the guards"), 990.0000f, 237.2344f, 661.0f, false},
		{TEXT("0.0.51.0.1"), TEXT("waved it through without asking for"), 950.0000f, 277.7656f, 682.0f, false},
		{TEXT("0.0.51.0.1"), TEXT("papers."), 950.0000f, 57.6406f, 703.0f, false},
	};

	const FChromeCase PrettyFloatCases[] = {
		{TEXT("brfloat-pretty"), Lines_brfloat_pretty, UE_ARRAY_COUNT(Lines_brfloat_pretty)},
		{TEXT("ibfloat-pretty"), Lines_ibfloat_pretty, UE_ARRAY_COUNT(Lines_ibfloat_pretty)},
		{TEXT("ib-pretty"), Lines_ib_pretty, UE_ARRAY_COUNT(Lines_ib_pretty)},
		{TEXT("leadfloat-pretty"), Lines_leadfloat_pretty, UE_ARRAY_COUNT(Lines_leadfloat_pretty)},
		{TEXT("leadfloat-greedy"), Lines_leadfloat_greedy, UE_ARRAY_COUNT(Lines_leadfloat_greedy)},
		{TEXT("sibfloat"), Lines_sibfloat, UE_ARRAY_COUNT(Lines_sibfloat)},
		{TEXT("midfloat-pretty"), Lines_midfloat_pretty, UE_ARRAY_COUNT(Lines_midfloat_pretty)},
		{TEXT("tallfloat-pretty"), Lines_tallfloat_pretty, UE_ARRAY_COUNT(Lines_tallfloat_pretty)},
		{TEXT("openfloat-pretty"), Lines_openfloat_pretty, UE_ARRAY_COUNT(Lines_openfloat_pretty)},
		{TEXT("openfloat-greedy"), Lines_openfloat_greedy, UE_ARRAY_COUNT(Lines_openfloat_greedy)},
	};

	const FChromeLine Lines_prewrap_pretty[] = {
		{TEXT("0.0.1.0"), TEXT("The caravan reached the old bridge at dusk, and"), 10.0000f, 380.8906f, 10.0f, false},
		{TEXT("0.0.1.0"), TEXT("the guards waved it through without asking"), 10.0000f, 343.7812f, 31.0f, false},
		{TEXT("0.0.1.0"), TEXT("for papers."), 10.0000f, 84.3594f, 52.0f, false},
	};

	const FChromeLine Lines_prewrap_greedy[] = {
		{TEXT("0.0.3.0"), TEXT("The caravan reached the old bridge at dusk, and"), 400.0000f, 380.8906f, 10.0f, false},
		{TEXT("0.0.3.0"), TEXT("the guards waved it through without asking for"), 400.0000f, 370.5000f, 31.0f, false},
		{TEXT("0.0.3.0"), TEXT("papers."), 400.0000f, 57.6406f, 52.0f, false},
	};

	const FChromeLine Lines_nowrap_pretty[] = {
		{TEXT("0.0.5.0"), TEXT("The caravan reached the old"), 800.0000f, 219.9062f, 10.0f, false},
		{TEXT("0.0.5.0"), TEXT("bridge at dusk, and"), 800.0000f, 148.6250f, 31.0f, false},
		{TEXT("0.0.5.0"), TEXT("the guards waved it"), 800.0000f, 153.5312f, 52.0f, false},
		{TEXT("0.0.5.1.0"), TEXT("through without asking"), 800.0000f, 182.0312f, 73.0f, false},
		{TEXT("0.0.5.2"), TEXT("for"), 982.0312f, 26.7344f, 73.0f, false},
		{TEXT("0.0.5.2"), TEXT("papers."), 800.0000f, 57.6406f, 94.0f, false},
	};

	const FChromeLine Lines_nowrap_greedy[] = {
		{TEXT("0.0.7.0"), TEXT("The caravan reached the old"), 1100.0000f, 219.9062f, 10.0f, false},
		{TEXT("0.0.7.0"), TEXT("bridge at dusk, and the guards"), 1100.0000f, 237.2344f, 31.0f, false},
		{TEXT("0.0.7.0"), TEXT("waved it"), 1100.0000f, 64.9219f, 52.0f, false},
		{TEXT("0.0.7.1.0"), TEXT("through without asking"), 1100.0000f, 182.0312f, 73.0f, false},
		{TEXT("0.0.7.2"), TEXT("for"), 1282.0312f, 26.7344f, 73.0f, false},
		{TEXT("0.0.7.2"), TEXT("papers."), 1100.0000f, 57.6406f, 94.0f, false},
	};

	const FChromeLine Lines_breakall_pretty[] = {
		{TEXT("0.0.9.0"), TEXT("The caravan reached the old bridg"), 10.0000f, 264.8594f, 130.0f, false},
		{TEXT("0.0.9.0"), TEXT("e at dusk, and the guards waved i"), 10.0000f, 258.0781f, 151.0f, false},
		{TEXT("0.0.9.0"), TEXT("t through without asking for pape"), 10.0000f, 263.6094f, 172.0f, false},
		{TEXT("0.0.9.0"), TEXT("rs."), 10.0000f, 18.3594f, 193.0f, false},
	};

	const FChromeLine Lines_breakall_greedy[] = {
		{TEXT("0.0.11.0"), TEXT("The caravan reached the old bridg"), 300.0000f, 264.8594f, 130.0f, false},
		{TEXT("0.0.11.0"), TEXT("e at dusk, and the guards waved it"), 300.0000f, 265.4375f, 151.0f, false},
		{TEXT("0.0.11.0"), TEXT("through without asking for paper"), 300.0000f, 258.1719f, 172.0f, false},
		{TEXT("0.0.11.0"), TEXT("s."), 300.0000f, 12.4844f, 193.0f, false},
	};

	const FChromeLine Lines_nwfor_pretty[] = {
		{TEXT("0.0.53.0"), TEXT("The caravan reached the"), 1300.0000f, 191.6875f, 640.0f, false},
		{TEXT("0.0.53.0"), TEXT("old bridge at dusk, and"), 1300.0000f, 176.8594f, 661.0f, false},
		{TEXT("0.0.53.0"), TEXT("the guards waved it"), 1300.0000f, 153.5312f, 682.0f, false},
		{TEXT("0.0.53.1.0"), TEXT("through without asking"), 1300.0000f, 182.0312f, 703.0f, false},
		{TEXT("0.0.53.2"), TEXT("for papers."), 1300.0000f, 84.3594f, 724.0f, false},
	};

	const FChromeLine Lines_nwfor_greedy[] = {
		{TEXT("0.0.55.0"), TEXT("The caravan reached the"), 1550.0000f, 191.6875f, 640.0f, false},
		{TEXT("0.0.55.0"), TEXT("old bridge at dusk, and"), 1550.0000f, 176.8594f, 661.0f, false},
		{TEXT("0.0.55.0"), TEXT("the guards waved it"), 1550.0000f, 153.5312f, 682.0f, false},
		{TEXT("0.0.55.1.0"), TEXT("through without asking"), 1550.0000f, 182.0312f, 703.0f, false},
		{TEXT("0.0.55.2"), TEXT("for papers."), 1550.0000f, 84.3594f, 724.0f, false},
	};

	const FChromeLine Lines_spanspaces[] = {
		{TEXT("0.0.57.0"), TEXT("aaaa"), 10.0000f, 40.9375f, 800.0f, false},
		{TEXT("0.0.57.0"), TEXT("bbb"), 10.0000f, 30.9219f, 821.0f, false},
		{TEXT("0.0.57.1.0"), TEXT(""), 40.9219f, 12.3438f, 821.0f, false},
		{TEXT("0.0.57.1.0"), TEXT("ccc"), 10.0000f, 28.7188f, 842.0f, false},
	};

	const FChromeLine Lines_spanspaces_pretty[] = {
		{TEXT("0.0.59.0"), TEXT("aaaa"), 100.0000f, 40.9375f, 800.0f, false},
		{TEXT("0.0.59.0"), TEXT("bbb"), 100.0000f, 30.9219f, 821.0f, false},
		{TEXT("0.0.59.1.0"), TEXT(""), 130.9219f, 12.3438f, 821.0f, false},
		{TEXT("0.0.59.1.0"), TEXT("ccc"), 100.0000f, 28.7188f, 842.0f, false},
	};

	const FChromeLine Lines_indent_pretty[] = {
		{TEXT("0.0.61.0"), TEXT("The caravan reached the old bridge at dusk,"), 200.0000f, 356.2188f, 800.0f, false},
		{TEXT("0.0.61.0"), TEXT("and the guards waved it through without asking"), 200.0000f, 377.2031f, 821.0f, false},
		{TEXT("0.0.61.0"), TEXT("for papers."), 200.0000f, 84.3594f, 842.0f, false},
	};

	const FChromeLine Lines_indent_greedy[] = {
		{TEXT("0.0.63.0"), TEXT("The caravan reached the old bridge at dusk, and"), 600.0000f, 389.1094f, 800.0f, false},
		{TEXT("0.0.63.0"), TEXT("the guards waved it through without asking for"), 600.0000f, 370.5000f, 821.0f, false},
		{TEXT("0.0.63.0"), TEXT("papers."), 600.0000f, 57.6406f, 842.0f, false},
	};

	const FChromeCase PrettyModeCases[] = {
		{TEXT("prewrap-pretty"), Lines_prewrap_pretty, UE_ARRAY_COUNT(Lines_prewrap_pretty)},
		{TEXT("prewrap-greedy"), Lines_prewrap_greedy, UE_ARRAY_COUNT(Lines_prewrap_greedy)},
		{TEXT("nowrap-pretty"), Lines_nowrap_pretty, UE_ARRAY_COUNT(Lines_nowrap_pretty)},
		{TEXT("nowrap-greedy"), Lines_nowrap_greedy, UE_ARRAY_COUNT(Lines_nowrap_greedy)},
		{TEXT("breakall-pretty"), Lines_breakall_pretty, UE_ARRAY_COUNT(Lines_breakall_pretty)},
		{TEXT("breakall-greedy"), Lines_breakall_greedy, UE_ARRAY_COUNT(Lines_breakall_greedy)},
		{TEXT("nwfor-pretty"), Lines_nwfor_pretty, UE_ARRAY_COUNT(Lines_nwfor_pretty)},
		{TEXT("nwfor-greedy"), Lines_nwfor_greedy, UE_ARRAY_COUNT(Lines_nwfor_greedy)},
		{TEXT("spanspaces"), Lines_spanspaces, UE_ARRAY_COUNT(Lines_spanspaces)},
		{TEXT("spanspaces-pretty"), Lines_spanspaces_pretty, UE_ARRAY_COUNT(Lines_spanspaces_pretty)},
		{TEXT("indent-pretty"), Lines_indent_pretty, UE_ARRAY_COUNT(Lines_indent_pretty)},
		{TEXT("indent-greedy"), Lines_indent_greedy, UE_ARRAY_COUNT(Lines_indent_greedy)},
	};

	const FChromeLine Lines_elemb_pretty[] = {
		{TEXT("0.0.29.0"), TEXT("The caravan reached the old bridge at dusk, and"), 800.0000f, 372.6406f, 380.0f, false},
		{TEXT("0.0.29.0"), TEXT("the guards waved it through without asking"), 800.0000f, 339.6562f, 401.0f, false},
		{TEXT("0.0.29.0"), TEXT("for pa"), 800.0000f, 46.2344f, 422.0f, false},
		{TEXT("0.0.29.1.0"), TEXT("pers"), 846.2344f, 34.0938f, 422.0f, false},
		{TEXT("0.0.29.2"), TEXT("."), 880.3281f, 4.0625f, 422.0f, false},
	};

	const FChromeLine Lines_elemb_greedy[] = {
		{TEXT("0.0.31.0"), TEXT("The caravan reached the old bridge at dusk, and"), 1200.0000f, 372.6406f, 380.0f, false},
		{TEXT("0.0.31.0"), TEXT("the guards waved it through without asking for"), 1200.0000f, 366.3750f, 401.0f, false},
		{TEXT("0.0.31.0"), TEXT("pa"), 1200.0000f, 19.5156f, 422.0f, false},
		{TEXT("0.0.31.1.0"), TEXT("pers"), 1219.5156f, 34.0938f, 422.0f, false},
		{TEXT("0.0.31.2"), TEXT("."), 1253.6094f, 4.0625f, 422.0f, false},
	};

	const FChromeLine Lines_padspan_pretty[] = {
		{TEXT("0.0.33.0"), TEXT("The caravan reached the old bridge at dusk,"), 10.0000f, 339.7500f, 520.0f, false},
		{TEXT("0.0.33.0"), TEXT("and the guards waved it through without asking"), 10.0000f, 373.0781f, 541.0f, false},
		{TEXT("0.0.33.0"), TEXT("for"), 10.0000f, 22.6094f, 562.0f, false},
		{TEXT("0.0.33.1.0"), TEXT("papers."), 42.6094f, 61.7500f, 562.0f, false},
	};

	const FChromeLine Lines_padopen_pretty[] = {
		{TEXT("0.0.35.0"), TEXT("The caravan reached the old bridge at dusk,"), 400.0000f, 339.7500f, 520.0f, false},
		{TEXT("0.0.35.0"), TEXT("and the guards waved it through without asking"), 400.0000f, 373.0781f, 541.0f, false},
		{TEXT("0.0.35.1.0"), TEXT("for papers."), 412.0000f, 84.3594f, 562.0f, false},
	};

	const FChromeLine Lines_fit_greedy[] = {
		{TEXT("0.0.37.0"), TEXT("The caravan reached the old bridge at dusk, and"), 800.0000f, 372.6406f, 520.0f, false},
		{TEXT("0.0.37.0"), TEXT("the guards waved it through without asking for"), 800.0000f, 366.3750f, 541.0f, false},
		{TEXT("0.0.37.0"), TEXT("papers."), 800.0000f, 57.6406f, 562.0f, false},
	};

	const FChromeLine Lines_fit_pretty[] = {
		{TEXT("0.0.39.0"), TEXT("The caravan reached the old bridge at dusk, and"), 1200.0000f, 372.6406f, 520.0f, false},
		{TEXT("0.0.39.0"), TEXT("the guards waved it through without asking"), 1200.0000f, 339.6562f, 541.0f, false},
		{TEXT("0.0.39.0"), TEXT("for papers."), 1200.0000f, 84.3594f, 562.0f, false},
	};

	const FChromeCase PrettyGlueCases[] = {
		{TEXT("elemb-pretty"), Lines_elemb_pretty, UE_ARRAY_COUNT(Lines_elemb_pretty)},
		{TEXT("elemb-greedy"), Lines_elemb_greedy, UE_ARRAY_COUNT(Lines_elemb_greedy)},
		{TEXT("padspan-pretty"), Lines_padspan_pretty, UE_ARRAY_COUNT(Lines_padspan_pretty)},
		{TEXT("padopen-pretty"), Lines_padopen_pretty, UE_ARRAY_COUNT(Lines_padopen_pretty)},
		{TEXT("fit-greedy"), Lines_fit_greedy, UE_ARRAY_COUNT(Lines_fit_greedy)},
		{TEXT("fit-pretty"), Lines_fit_pretty, UE_ARRAY_COUNT(Lines_fit_pretty)},
	};

	const FChromeLine Lines_nbsp_prewrap[] = {
		{TEXT("0.0.41.0"), TEXT("aaaa aaaa"), 10.0000f, 81.8438f, 640.0f, false},
		{TEXT("0.0.41.0"), TEXT("bbbbbbbbbbbb"), 10.0000f, 127.7656f, 661.0f, false},
	};

	const FChromeLine Lines_hang_right[] = {
		{TEXT("0.0.43.0"), TEXT("aaaa bbbb"), 167.8594f, 94.4844f, 640.0f, false},
		{TEXT("0.0.43.0"), TEXT("cccc"), 211.7188f, 38.2812f, 661.0f, false},
	};

	const FChromeLine Lines_hang_center[] = {
		{TEXT("0.0.45.0"), TEXT("aaaa bbbb"), 308.9219f, 94.4844f, 640.0f, false},
		{TEXT("0.0.45.0"), TEXT("cccc"), 330.8594f, 38.2812f, 661.0f, false},
	};

	const FChromeLine Lines_hang_justify[] = {
		{TEXT("0.0.47.0"), TEXT("aaaa bbbb"), 450.0000f, 112.3438f, 640.0f, false},
		{TEXT("0.0.47.0"), TEXT("cccc dd"), 450.0000f, 63.0000f, 661.0f, false},
	};

	const FChromeLine Lines_trail_center[] = {
		{TEXT("0.0.65.0"), TEXT("The caravan reached the old"), 1015.0469f, 219.9062f, 800.0f, false},
		{TEXT("0.0.65.0"), TEXT("bridge at dusk, and the guards"), 1006.3750f, 237.2344f, 821.0f, false},
		{TEXT("0.0.65.0"), TEXT("waved it"), 1092.5312f, 64.9219f, 842.0f, false},
		{TEXT("0.0.65.1.0"), TEXT("through without asking"), 1020.6094f, 182.0312f, 863.0f, false},
		{TEXT("0.0.65.2"), TEXT("for"), 1202.6406f, 26.7344f, 863.0f, false},
		{TEXT("0.0.65.2"), TEXT("papers."), 1096.1719f, 57.6406f, 884.0f, false},
	};

	const FChromeLine Lines_trail_right[] = {
		{TEXT("0.0.67.0"), TEXT("The caravan reached the old"), 1330.0938f, 219.9062f, 800.0f, false},
		{TEXT("0.0.67.0"), TEXT("bridge at dusk, and the guards"), 1312.7656f, 237.2344f, 821.0f, false},
		{TEXT("0.0.67.0"), TEXT("waved it"), 1485.0781f, 64.9219f, 842.0f, false},
		{TEXT("0.0.67.1.0"), TEXT("through without asking"), 1341.2344f, 182.0312f, 863.0f, false},
		{TEXT("0.0.67.2"), TEXT("for"), 1523.2656f, 26.7344f, 863.0f, false},
		{TEXT("0.0.67.2"), TEXT("papers."), 1492.3594f, 57.6406f, 884.0f, false},
	};

	const FChromeLine Lines_trail_float_right[] = {
		{TEXT("0.0.69.0"), TEXT("The caravan reached the old"), 1630.0938f, 219.9062f, 800.0f, false},
		{TEXT("0.0.69.0"), TEXT("bridge."), 1795.7500f, 54.2500f, 821.0f, false},
		{TEXT("0.0.69.2"), TEXT("more"), 1801.6250f, 38.8750f, 842.0f, false},
		{TEXT("0.0.69.3.0"), TEXT("x"), 1840.5000f, 9.5000f, 842.0f, false},
	};

	const FChromeLine Lines_trail_span_right[] = {
		{TEXT("0.0.71.0"), TEXT("The caravan"), 1026.5156f, 93.4844f, 960.0f, false},
		{TEXT("0.0.71.0"), TEXT("reached"), 1025.8906f, 67.4688f, 981.0f, false},
		{TEXT("0.0.71.1.0"), TEXT("the"), 1093.3594f, 26.6406f, 981.0f, false},
		{TEXT("0.0.71.1.0"), TEXT("old"), 1021.2969f, 24.1250f, 1002.0f, false},
		{TEXT("0.0.71.2"), TEXT("bridge at"), 1045.4219f, 74.5781f, 1002.0f, false},
		{TEXT("0.0.71.2"), TEXT("dusk"), 1082.8594f, 37.1406f, 1023.0f, false},
	};

	const FChromeCase LineEndCases[] = {
		{TEXT("nbsp-prewrap"), Lines_nbsp_prewrap, UE_ARRAY_COUNT(Lines_nbsp_prewrap)},
		{TEXT("hang-right"), Lines_hang_right, UE_ARRAY_COUNT(Lines_hang_right)},
		{TEXT("hang-center"), Lines_hang_center, UE_ARRAY_COUNT(Lines_hang_center)},
		{TEXT("hang-justify"), Lines_hang_justify, UE_ARRAY_COUNT(Lines_hang_justify)},
		{TEXT("trail-center"), Lines_trail_center, UE_ARRAY_COUNT(Lines_trail_center)},
		{TEXT("trail-right"), Lines_trail_right, UE_ARRAY_COUNT(Lines_trail_right)},
		{TEXT("trail-float-right"), Lines_trail_float_right, UE_ARRAY_COUNT(Lines_trail_float_right)},
		{TEXT("trail-span-right"), Lines_trail_span_right, UE_ARRAY_COUNT(Lines_trail_span_right)},
	};

	const FChromeLine Lines_ip_a[] = {
		{TEXT("0.0.77.0"), TEXT("The caravan"), 1000.0000f, 105.8281f, 800.0f, false},
	};

	const FChromeLine Lines_ip_b[] = {
		{TEXT("0.0.79.0"), TEXT("The caravan"), 1000.0000f, 93.4844f, 830.0f, false},
	};

	const FChromeLine Lines_ip_c[] = {
		{TEXT("0.0.81.0"), TEXT("The caravan"), 1000.0000f, 97.6094f, 860.0f, false},
	};

	const FChromeLine Lines_ip_d[] = {
		{TEXT("0.0.83.0"), TEXT("dusk, and"), 1000.0000f, 74.0625f, 890.0f, false},
	};

	const FChromeLine Lines_ip_e[] = {
		{TEXT("0.0.85.0"), TEXT("dusk,"), 1000.0000f, 44.7500f, 920.0f, false},
		{TEXT("0.0.85.0"), TEXT("and"), 1000.0000f, 29.3125f, 941.0f, false},
	};

	const FChromeLine Lines_ip_f[] = {
		{TEXT("0.0.87.0"), TEXT("dusk,"), 1000.0000f, 41.1562f, 950.0f, false},
	};

	const FChromeLine Lines_ip_g[] = {
		{TEXT("0.0.89.0"), TEXT("a Tb"), 1000.0000f, 32.6094f, 980.0f, false},
	};

	const FChromeLine Lines_ip_h[] = {
		{TEXT("0.0.91.0"), TEXT("aTb"), 1000.0000f, 28.9219f, 1010.0f, false},
	};

	const FChromeCase ParagraphStartCases[] = {
		{TEXT("ip-a"), Lines_ip_a, UE_ARRAY_COUNT(Lines_ip_a)},
		{TEXT("ip-b"), Lines_ip_b, UE_ARRAY_COUNT(Lines_ip_b)},
		{TEXT("ip-c"), Lines_ip_c, UE_ARRAY_COUNT(Lines_ip_c)},
		{TEXT("ip-d"), Lines_ip_d, UE_ARRAY_COUNT(Lines_ip_d)},
		{TEXT("ip-e"), Lines_ip_e, UE_ARRAY_COUNT(Lines_ip_e)},
		{TEXT("ip-f"), Lines_ip_f, UE_ARRAY_COUNT(Lines_ip_f)},
		{TEXT("ip-g"), Lines_ip_g, UE_ARRAY_COUNT(Lines_ip_g)},
		{TEXT("ip-h"), Lines_ip_h, UE_ARRAY_COUNT(Lines_ip_h)},
	};

	const FChromeLine Lines_pctdollar_ba[] = {
		{TEXT("0.0.73.0"), TEXT("aaaaaa"), 1200.0000f, 55.2031f, 960.0f, false},
		{TEXT("0.0.73.0"), TEXT("a% bbb"), 1200.0000f, 56.5781f, 981.0f, false},
		{TEXT("0.0.73.0"), TEXT("bbb$c"), 1200.0000f, 50.2969f, 1002.0f, false},
		{TEXT("0.0.73.0"), TEXT("cccccc"), 1200.0000f, 57.4219f, 1023.0f, false},
		{TEXT("0.0.73.0"), TEXT("|d"), 1200.0000f, 14.1562f, 1044.0f, false},
	};

	const FChromeLine Lines_ba_probe[] = {
		{TEXT("0.0.75.0"), TEXT("a%"), 1300.0000f, 21.5625f, 960.0f, false},
		{TEXT("0.0.75.0"), TEXT("b"), 1300.0000f, 10.3125f, 981.0f, false},
		{TEXT("0.0.75.0"), TEXT("$c"), 1300.0000f, 19.3906f, 1002.0f, false},
		{TEXT("0.0.75.0"), TEXT("|"), 1300.0000f, 3.8438f, 1023.0f, false},
		{TEXT("0.0.75.0"), TEXT("d"), 1300.0000f, 10.3125f, 1044.0f, false},
		{TEXT("0.0.75.0"), TEXT("-"), 1300.0000f, 7.2812f, 1065.0f, false},
		{TEXT("0.0.75.0"), TEXT("e."), 1300.0000f, 13.4219f, 1086.0f, false},
		{TEXT("0.0.75.0"), TEXT("f"), 1300.0000f, 7.0781f, 1107.0f, false},
		{TEXT("0.0.75.0"), TEXT("(g)"), 1300.0000f, 22.1562f, 1128.0f, false},
		{TEXT("0.0.75.0"), TEXT("h'i\"j,"), 1300.0000f, 32.0625f, 1149.0f, false},
		{TEXT("0.0.75.0"), TEXT("k/"), 1300.0000f, 14.5312f, 1170.0f, false},
		{TEXT("0.0.75.0"), TEXT("l!"), 1300.0000f, 7.9688f, 1191.0f, false},
		{TEXT("0.0.75.0"), TEXT("m?"), 1300.0000f, 23.0469f, 1212.0f, false},
		{TEXT("0.0.75.0"), TEXT("n;"), 1300.0000f, 13.8594f, 1233.0f, false},
		{TEXT("0.0.75.0"), TEXT("o:"), 1300.0000f, 13.9375f, 1254.0f, false},
		{TEXT("0.0.75.0"), TEXT("p]"), 1300.0000f, 15.5781f, 1275.0f, false},
		{TEXT("0.0.75.0"), TEXT("q"), 1300.0000f, 10.3125f, 1296.0f, false},
		{TEXT("0.0.75.0"), TEXT("[r}"), 1300.0000f, 18.3125f, 1317.0f, false},
		{TEXT("0.0.75.0"), TEXT("s"), 1300.0000f, 8.4375f, 1338.0f, false},
		{TEXT("0.0.75.0"), TEXT("{t"), 1300.0000f, 14.2812f, 1359.0f, false},
	};

	const FChromeCase BreakAllCases[] = {
		{TEXT("pctdollar-ba"), Lines_pctdollar_ba, UE_ARRAY_COUNT(Lines_pctdollar_ba)},
		{TEXT("ba-probe"), Lines_ba_probe, UE_ARRAY_COUNT(Lines_ba_probe)},
	};

	const FChromeLine Lines_j_multi[] = {
		{TEXT("0.0.1.0"), TEXT("The caravan reached the old bridge at"), 10.0000f, 300.0000f, 10.0f, false},
		{TEXT("0.0.1.0"), TEXT("dusk, and the guards waved it through"), 10.0000f, 300.0000f, 31.0f, false},
		{TEXT("0.0.1.0"), TEXT("without asking for papers."), 10.0000f, 203.7031f, 52.0f, false},
	};

	const FChromeLine Lines_j_span[] = {
		{TEXT("0.0.3.0"), TEXT("The caravan reached the"), 330.0000f, 211.5312f, 10.0f, false},
		{TEXT("0.0.3.1.0"), TEXT("old bridge"), 547.5312f, 82.4688f, 10.0f, false},
		{TEXT("0.0.3.1.0"), TEXT("at dusk, and the"), 330.0000f, 146.0312f, 31.0f, false},
		{TEXT("0.0.3.2"), TEXT("guards waved it"), 482.0312f, 147.9531f, 31.0f, false},
		{TEXT("0.0.3.2"), TEXT("through without asking for papers."), 330.0000f, 270.4844f, 52.0f, false},
	};

	const FChromeLine Lines_j_words[] = {
		{TEXT("0.0.5.0.0"), TEXT("The"), 650.0000f, 28.7031f, 10.0f, false},
		{TEXT("0.0.5.2.0"), TEXT("caravan"), 683.7188f, 60.6875f, 10.0f, false},
		{TEXT("0.0.5.4.0"), TEXT("reached"), 749.4219f, 63.3594f, 10.0f, false},
		{TEXT("0.0.5.6.0"), TEXT("the"), 817.7969f, 26.6406f, 10.0f, false},
		{TEXT("0.0.5.8.0"), TEXT("old"), 849.4531f, 24.1250f, 10.0f, false},
		{TEXT("0.0.5.10.0"), TEXT("bridge"), 878.5938f, 50.3125f, 10.0f, false},
		{TEXT("0.0.5.12.0"), TEXT("at"), 933.9219f, 16.0625f, 10.0f, false},
		{TEXT("0.0.5.14.0"), TEXT("dusk,"), 650.0000f, 40.6250f, 31.0f, true},
		{TEXT("0.0.5.16.0"), TEXT("and"), 708.3906f, 29.3125f, 31.0f, true},
		{TEXT("0.0.5.18.0"), TEXT("the"), 755.4688f, 26.6406f, 31.0f, true},
		{TEXT("0.0.5.20.0"), TEXT("guards"), 799.8750f, 53.7500f, 31.0f, true},
		{TEXT("0.0.5.22.0"), TEXT("waved"), 871.3906f, 49.5312f, 31.0f, true},
		{TEXT("0.0.5.24.0"), TEXT("it"), 938.6875f, 11.2812f, 31.0f, false},
		{TEXT("0.0.5.26.0"), TEXT("through."), 650.0000f, 66.7344f, 52.0f, false},
	};

	const FChromeLine Lines_j_long[] = {
		{TEXT("0.0.7.0"), TEXT("Incomprehensibilities"), 970.0000f, 165.1406f, 10.0f, false},
		{TEXT("0.0.7.0"), TEXT("overflow"), 970.0000f, 66.5312f, 31.0f, false},
		{TEXT("0.0.7.0"), TEXT("here"), 970.0000f, 34.5000f, 52.0f, false},
	};

	const FChromeLine Lines_j_br[] = {
		{TEXT("0.0.9.0"), TEXT("The caravan reached the old bridge at"), 1100.0000f, 300.0000f, 10.0f, false},
		{TEXT("0.0.9.0"), TEXT("dusk, and the guards waved it"), 1100.0000f, 231.6875f, 31.0f, false},
		{TEXT("0.0.9.2"), TEXT("through without asking for papers at"), 1100.0000f, 300.0000f, 52.0f, false},
		{TEXT("0.0.9.2"), TEXT("the gate of the city."), 1100.0000f, 156.5625f, 73.0f, false},
	};

	const FChromeLine Lines_j_prewrap[] = {
		{TEXT("0.0.11.0"), TEXT("The caravan reached the old bridge at"), 1420.0000f, 304.1250f, 10.0f, false},
		{TEXT("0.0.11.0"), TEXT("dusk, and the guards waved it"), 1420.0000f, 304.1250f, 31.0f, false},
		{TEXT("0.0.11.0"), TEXT("through without asking for papers."), 1420.0000f, 270.4844f, 52.0f, false},
	};

	const FChromeLine Lines_j_newline[] = {
		{TEXT("0.0.13.0"), TEXT("The caravan reached the old bridge at"), 10.0000f, 304.1250f, 200.0f, false},
		{TEXT("0.0.13.0"), TEXT("dusk, and the"), 10.0000f, 104.8125f, 221.0f, false},
		{TEXT("0.0.13.0"), TEXT("guards waved it through without"), 10.0000f, 304.1250f, 242.0f, false},
		{TEXT("0.0.13.0"), TEXT("asking for papers."), 10.0000f, 138.6406f, 263.0f, false},
	};

	const FChromeLine Lines_j_ls[] = {
		{TEXT("0.0.15.0"), TEXT("The caravan reached the old"), 330.0000f, 300.0000f, 200.0f, false},
		{TEXT("0.0.15.0"), TEXT("bridge at dusk, and the guards"), 330.0000f, 300.0000f, 221.0f, false},
		{TEXT("0.0.15.0"), TEXT("waved it through without"), 330.0000f, 300.0000f, 242.0f, false},
		{TEXT("0.0.15.0"), TEXT("asking for papers."), 330.0000f, 174.6406f, 263.0f, false},
	};

	const FChromeLine Lines_j_ib[] = {
		{TEXT("0.0.17.0"), TEXT("The caravan reached the"), 650.0000f, 209.9219f, 200.0f, false},
		{TEXT("0.0.17.1.0"), TEXT("old"), 863.9219f, 24.1250f, 200.0f, false},
		{TEXT("0.0.17.2"), TEXT("bridge"), 892.0469f, 57.9531f, 200.0f, false},
		{TEXT("0.0.17.2"), TEXT("at dusk, and the guards waved it"), 650.0000f, 300.0000f, 221.0f, false},
		{TEXT("0.0.17.2"), TEXT("through without asking for papers."), 650.0000f, 270.4844f, 242.0f, false},
	};

	const FChromeLine Lines_j_pretty[] = {
		{TEXT("0.0.19.0"), TEXT("The caravan reached the old bridge at dusk,"), 970.0000f, 380.0000f, 200.0f, false},
		{TEXT("0.0.19.0"), TEXT("and the guards waved it through without asking"), 970.0000f, 380.0000f, 221.0f, false},
		{TEXT("0.0.19.0"), TEXT("for papers."), 970.0000f, 84.3594f, 242.0f, false},
	};

	const FChromeLine Lines_j_nbsp[] = {
		{TEXT("0.0.21.0"), TEXT("The caravan reached the old bridge"), 1420.0000f, 300.0000f, 200.0f, false},
		{TEXT("0.0.21.0"), TEXT("at dusk, and the guards waved it"), 1420.0000f, 300.0000f, 221.0f, false},
		{TEXT("0.0.21.0"), TEXT("through without asking for papers."), 1420.0000f, 270.4844f, 242.0f, false},
	};

	const FChromeLine Lines_j_sym[] = {
		{TEXT("0.0.23.0.0"), TEXT("aaa"), 10.0000f, 27.6094f, 400.0f, false},
		{TEXT("0.0.23.1"), TEXT("†"), 37.6094f, 11.9219f, 400.0f, false},
		{TEXT("0.0.23.2.0"), TEXT("bbb"), 49.5312f, 30.9219f, 400.0f, false},
		{TEXT("0.0.23.3"), TEXT("†"), 80.4531f, 11.9219f, 400.0f, false},
		{TEXT("0.0.23.4.0"), TEXT("ccc"), 92.3750f, 28.7188f, 400.0f, false},
		{TEXT("0.0.23.6.0"), TEXT("ddd"), 126.6094f, 30.9219f, 400.0f, false},
		{TEXT("0.0.23.7"), TEXT("‰"), 157.5312f, 24.0469f, 400.0f, false},
		{TEXT("0.0.23.8.0"), TEXT("eee"), 181.5781f, 28.4219f, 400.0f, false},
		{TEXT("0.0.23.9"), TEXT("fff ggg hhh iii jjj"), 10.0000f, 117.6719f, 421.0f, false},
	};

	const FChromeLine Lines_j_float[] = {
		{TEXT("0.0.25.1"), TEXT("The caravan reached the old"), 370.0000f, 260.0000f, 400.0f, false},
		{TEXT("0.0.25.1"), TEXT("bridge at dusk, and the guards"), 370.0000f, 260.0000f, 421.0f, false},
		{TEXT("0.0.25.1"), TEXT("waved it through without asking for"), 330.0000f, 300.0000f, 442.0f, false},
		{TEXT("0.0.25.1"), TEXT("papers."), 330.0000f, 57.6406f, 463.0f, false},
	};

	const FChromeLine Lines_j_trail[] = {
		{TEXT("0.0.27.0"), TEXT("The caravan reached the old bridge at"), 650.0000f, 300.0000f, 400.0f, false},
		{TEXT("0.0.27.0"), TEXT("dusk, and the guards waved it"), 650.0000f, 300.0000f, 421.0f, false},
		{TEXT("0.0.27.1.0"), TEXT("through without asking"), 650.0000f, 182.0312f, 442.0f, false},
		{TEXT("0.0.27.2"), TEXT("for papers."), 832.0312f, 88.4688f, 442.0f, false},
	};

	const FChromeLine Lines_j_nowrap[] = {
		{TEXT("0.0.29.0"), TEXT("Short nowrap line"), 970.0000f, 135.8594f, 400.0f, false},
	};

	const FChromeLine Lines_tj_inherit[] = {
		{TEXT("0.0.65.0.0"), TEXT("ab"), 1440.0000f, 80.8594f, 860.0f, false},
		{TEXT("0.0.65.2.0"), TEXT("cd ef"), 1555.6562f, 40.2188f, 860.0f, false},
		{TEXT("0.0.65.4.0"), TEXT("gh"), 1661.3438f, 81.4375f, 860.0f, false},
		{TEXT("0.0.65.5.0"), TEXT("ij"), 1742.7812f, 69.2188f, 860.0f, false},
		{TEXT("0.0.65.7.0"), TEXT("kl"), 1846.7969f, 43.2031f, 860.0f, false},
	};

	const FChromeLine Lines_tjl_inherit[] = {
		{TEXT("0.0.67.0.0"), TEXT("ab"), 1440.0000f, 19.5156f, 884.0f, false},
		{TEXT("0.0.67.2.0"), TEXT("cd ef"), 1463.6406f, 40.2188f, 884.0f, false},
		{TEXT("0.0.67.4.0"), TEXT("gh"), 1507.9844f, 20.0938f, 884.0f, false},
		{TEXT("0.0.67.5.0"), TEXT("ij"), 1528.0781f, 7.8750f, 884.0f, false},
		{TEXT("0.0.67.7.0"), TEXT("kl"), 1540.0781f, 12.5312f, 884.0f, false},
	};

	const FChromeCase JustifyCases[] = {
		{TEXT("j-multi"), Lines_j_multi, UE_ARRAY_COUNT(Lines_j_multi)},
		{TEXT("j-span"), Lines_j_span, UE_ARRAY_COUNT(Lines_j_span)},
		{TEXT("j-words"), Lines_j_words, UE_ARRAY_COUNT(Lines_j_words)},
		{TEXT("j-long"), Lines_j_long, UE_ARRAY_COUNT(Lines_j_long)},
		{TEXT("j-br"), Lines_j_br, UE_ARRAY_COUNT(Lines_j_br)},
		{TEXT("j-prewrap"), Lines_j_prewrap, UE_ARRAY_COUNT(Lines_j_prewrap)},
		{TEXT("j-newline"), Lines_j_newline, UE_ARRAY_COUNT(Lines_j_newline)},
		{TEXT("j-ls"), Lines_j_ls, UE_ARRAY_COUNT(Lines_j_ls)},
		{TEXT("j-ib"), Lines_j_ib, UE_ARRAY_COUNT(Lines_j_ib)},
		{TEXT("j-pretty"), Lines_j_pretty, UE_ARRAY_COUNT(Lines_j_pretty)},
		{TEXT("j-nbsp"), Lines_j_nbsp, UE_ARRAY_COUNT(Lines_j_nbsp)},
		{TEXT("j-sym"), Lines_j_sym, UE_ARRAY_COUNT(Lines_j_sym)},
		{TEXT("j-float"), Lines_j_float, UE_ARRAY_COUNT(Lines_j_float)},
		{TEXT("j-trail"), Lines_j_trail, UE_ARRAY_COUNT(Lines_j_trail)},
		{TEXT("j-nowrap"), Lines_j_nowrap, UE_ARRAY_COUNT(Lines_j_nowrap)},
		{TEXT("tj-inherit"), Lines_tj_inherit, UE_ARRAY_COUNT(Lines_tj_inherit)},
		{TEXT("tjl-inherit"), Lines_tjl_inherit, UE_ARRAY_COUNT(Lines_tjl_inherit)},
	};

	const FChromeLine Lines_jl_justify[] = {
		{TEXT("0.0.31.0"), TEXT("The caravan reached the old bridge at"), 10.0000f, 300.0000f, 600.0f, false},
		{TEXT("0.0.31.0"), TEXT("dusk, and the guards waved it through"), 10.0000f, 300.0000f, 621.0f, false},
		{TEXT("0.0.31.0"), TEXT("without asking for papers."), 10.0000f, 300.0000f, 642.0f, false},
	};

	const FChromeLine Lines_jl_center[] = {
		{TEXT("0.0.33.0"), TEXT("The caravan reached the old bridge at"), 330.0000f, 300.0000f, 600.0f, false},
		{TEXT("0.0.33.0"), TEXT("dusk, and the guards waved it through"), 330.0000f, 300.0000f, 621.0f, false},
		{TEXT("0.0.33.0"), TEXT("without asking for papers."), 378.1406f, 203.7031f, 642.0f, false},
	};

	const FChromeLine Lines_jl_right[] = {
		{TEXT("0.0.35.0"), TEXT("The caravan reached the old bridge at"), 650.0000f, 300.0000f, 600.0f, false},
		{TEXT("0.0.35.0"), TEXT("dusk, and the guards waved it through"), 650.0000f, 300.0000f, 621.0f, false},
		{TEXT("0.0.35.0"), TEXT("without asking for papers."), 746.2969f, 203.7031f, 642.0f, false},
	};

	const FChromeLine Lines_jl_centered[] = {
		{TEXT("0.0.37.0"), TEXT("The caravan reached the old bridge at"), 972.7500f, 294.4844f, 600.0f, false},
		{TEXT("0.0.37.0"), TEXT("dusk, and the guards waved it through"), 970.7656f, 298.4688f, 621.0f, false},
		{TEXT("0.0.37.0"), TEXT("without asking for papers."), 1066.2969f, 203.7031f, 642.0f, false},
	};

	const FChromeLine Lines_jl_br[] = {
		{TEXT("0.0.39.0"), TEXT("The caravan reached the old bridge"), 1420.0000f, 300.0000f, 600.0f, false},
		{TEXT("0.0.39.2"), TEXT("at dusk, and the guards waved it"), 1420.0000f, 300.0000f, 621.0f, false},
		{TEXT("0.0.39.2"), TEXT("through."), 1420.0000f, 66.7344f, 642.0f, false},
	};

	const FChromeCase JustifyLastCases[] = {
		{TEXT("jl-justify"), Lines_jl_justify, UE_ARRAY_COUNT(Lines_jl_justify)},
		{TEXT("jl-center"), Lines_jl_center, UE_ARRAY_COUNT(Lines_jl_center)},
		{TEXT("jl-right"), Lines_jl_right, UE_ARRAY_COUNT(Lines_jl_right)},
		{TEXT("jl-centered"), Lines_jl_centered, UE_ARRAY_COUNT(Lines_jl_centered)},
		{TEXT("jl-br"), Lines_jl_br, UE_ARRAY_COUNT(Lines_jl_br)},
	};

	const FChromeLine Lines_ws_spans[] = {
		{TEXT("0.0.1.0.0"), TEXT("The caravan reached the old bridge at"), 10.0000f, 294.4844f, 10.0f, false},
		{TEXT("0.0.1.2.0"), TEXT("dusk,"), 10.0000f, 40.6250f, 31.0f, true},
		{TEXT("0.0.1.4.0"), TEXT("and"), 54.7500f, 29.3125f, 31.0f, true},
		{TEXT("0.0.1.6.0"), TEXT("the guards waved it through"), 88.1875f, 220.3125f, 31.0f, true},
		{TEXT("0.0.1.6.0"), TEXT("without asking."), 10.0000f, 119.2812f, 52.0f, true},
	};

	const FChromeLine Lines_ws_collapse[] = {
		{TEXT("0.0.3.0"), TEXT("aaa"), 330.0000f, 31.7188f, 10.0f, false},
		{TEXT("0.0.3.1.0"), TEXT("bbb"), 361.7188f, 30.9219f, 10.0f, false},
		{TEXT("0.0.3.4"), TEXT("ccc"), 396.7656f, 28.7188f, 10.0f, false},
		{TEXT("0.0.3.5.0"), TEXT("ddd"), 431.4844f, 39.1406f, 10.0f, false},
		{TEXT("0.0.3.6"), TEXT("eee"), 476.6250f, 32.5312f, 10.0f, false},
		{TEXT("0.0.3.8"), TEXT("fff"), 509.1562f, 17.3750f, 10.0f, false},
	};

	const FChromeLine Lines_ws_br[] = {
		{TEXT("0.0.11.0"), TEXT("line one"), 1409.7969f, 60.4062f, 10.0f, false},
		{TEXT("0.0.11.2"), TEXT("line two"), 1409.4219f, 61.1406f, 31.0f, false},
		{TEXT("0.0.11.5.0"), TEXT("three"), 1419.0625f, 41.8594f, 52.0f, false},
		{TEXT("0.0.11.9.0"), TEXT("four"), 1423.7969f, 32.4062f, 73.0f, false},
	};

	const FChromeLine Lines_ws_center[] = {
		{TEXT("0.0.27.1.0"), TEXT("aaa"), 448.6719f, 27.6094f, 400.0f, false},
		{TEXT("0.0.27.3.0"), TEXT("bbb"), 480.4062f, 30.9219f, 400.0f, false},
	};

	const FChromeLine Lines_ws_oof[] = {
		{TEXT("0.0.29.0.0"), TEXT("aaa"), 650.0000f, 27.6094f, 400.0f, false},
		{TEXT("0.0.29.6.0"), TEXT("bbb"), 681.7344f, 30.9219f, 400.0f, false},
	};

	const FChromeLine Lines_ws_inline_start[] = {
		{TEXT("0.0.31.0"), TEXT("aaa"), 970.0000f, 27.6094f, 400.0f, false},
		{TEXT("0.0.31.1.1.0"), TEXT("bbb"), 1001.7344f, 30.9219f, 400.0f, false},
	};

	const FChromeLine Lines_ws_j_words[] = {
		{TEXT("0.0.33.0.0"), TEXT("The"), 1290.0000f, 28.7031f, 400.0f, false},
		{TEXT("0.0.33.2.0"), TEXT("caravan"), 1323.7188f, 60.6875f, 400.0f, false},
		{TEXT("0.0.33.4.0"), TEXT("reached"), 1389.4219f, 63.3594f, 400.0f, false},
		{TEXT("0.0.33.6.0"), TEXT("the"), 1457.7969f, 26.6406f, 400.0f, false},
		{TEXT("0.0.33.8.0"), TEXT("old"), 1489.4531f, 24.1250f, 400.0f, false},
		{TEXT("0.0.33.10.0"), TEXT("bridge"), 1518.5938f, 50.3125f, 400.0f, false},
		{TEXT("0.0.33.12.0"), TEXT("at"), 1573.9219f, 16.0625f, 400.0f, false},
		{TEXT("0.0.33.14.0"), TEXT("dusk,"), 1290.0000f, 40.6250f, 421.0f, true},
		{TEXT("0.0.33.16.0"), TEXT("and"), 1348.3906f, 29.3125f, 421.0f, true},
		{TEXT("0.0.33.18.0"), TEXT("the"), 1395.4688f, 26.6406f, 421.0f, true},
		{TEXT("0.0.33.20.0"), TEXT("guards"), 1439.8750f, 53.7500f, 421.0f, true},
		{TEXT("0.0.33.22.0"), TEXT("waved"), 1511.3906f, 49.5312f, 421.0f, true},
		{TEXT("0.0.33.24.0"), TEXT("it"), 1578.6875f, 11.2812f, 421.0f, false},
		{TEXT("0.0.33.26.0"), TEXT("through."), 1290.0000f, 66.7344f, 442.0f, false},
	};

	const FChromeCase WsInlineCases[] = {
		{TEXT("ws-spans"), Lines_ws_spans, UE_ARRAY_COUNT(Lines_ws_spans)},
		{TEXT("ws-collapse"), Lines_ws_collapse, UE_ARRAY_COUNT(Lines_ws_collapse)},
		{TEXT("ws-br"), Lines_ws_br, UE_ARRAY_COUNT(Lines_ws_br)},
		{TEXT("ws-center"), Lines_ws_center, UE_ARRAY_COUNT(Lines_ws_center)},
		{TEXT("ws-oof"), Lines_ws_oof, UE_ARRAY_COUNT(Lines_ws_oof)},
		{TEXT("ws-inline-start"), Lines_ws_inline_start, UE_ARRAY_COUNT(Lines_ws_inline_start)},
		{TEXT("j-words"), Lines_ws_j_words, UE_ARRAY_COUNT(Lines_ws_j_words)},
	};

	const FChromeLine Lines_ws_blocks[] = {
		{TEXT("0.0.5.1.0"), TEXT("one"), 650.0000f, 29.1719f, 16.0f, false},
		{TEXT("0.0.5.3.0"), TEXT("two"), 650.0000f, 29.9062f, 43.0f, false},
		{TEXT("0.0.5.5.0"), TEXT("inline"), 650.0000f, 40.8594f, 70.0f, false},
		{TEXT("0.0.5.7.0"), TEXT("three"), 650.0000f, 41.8594f, 97.0f, false},
		{TEXT("0.0.5.11.0"), TEXT("four"), 680.0000f, 32.4062f, 124.0f, false},
	};

	const FChromeCase WsBlockCases[] = {
		{TEXT("ws-blocks"), Lines_ws_blocks, UE_ARRAY_COUNT(Lines_ws_blocks)},
	};

	const FChromeLine Lines_ws_prewrap[] = {
		{TEXT("0.0.13.0.0"), TEXT("aaa"), 10.0000f, 27.6094f, 200.0f, false},
		{TEXT("0.0.13.2.0"), TEXT("bbb"), 41.7344f, 30.9219f, 200.0f, false},
		{TEXT("0.0.13.4.0"), TEXT("ccc"), 80.8906f, 28.7188f, 200.0f, false},
		{TEXT("0.0.13.6.0"), TEXT("ddd"), 121.9531f, 30.9219f, 200.0f, false},
	};

	const FChromeLine Lines_ws_preline[] = {
		{TEXT("0.0.17.0.0"), TEXT("aaa"), 650.0000f, 27.6094f, 200.0f, false},
		{TEXT("0.0.17.2.0"), TEXT("bbb"), 650.0000f, 30.9219f, 221.0f, false},
		{TEXT("0.0.17.4.0"), TEXT("ccc"), 685.0469f, 28.7188f, 221.0f, false},
	};

	const FChromeLine Lines_ws_for[] = {
		{TEXT("0.0.19.1.0"), TEXT("The"), 970.0000f, 28.7031f, 200.0f, false},
		{TEXT("0.0.19.2.0"), TEXT("caravan"), 998.7031f, 60.6875f, 200.0f, false},
		{TEXT("0.0.19.3.0"), TEXT("reached"), 1059.3906f, 63.3594f, 200.0f, false},
		{TEXT("0.0.19.4.0"), TEXT("the"), 1122.7500f, 26.6406f, 200.0f, false},
		{TEXT("0.0.19.5.0"), TEXT("old"), 1149.3906f, 24.1250f, 200.0f, false},
		{TEXT("0.0.19.6.0"), TEXT("bridge"), 1173.5156f, 50.3125f, 200.0f, false},
	};

	const FChromeLine Lines_ws_for_spaced[] = {
		{TEXT("0.0.21.1.0.0"), TEXT("The"), 970.0000f, 28.7031f, 240.0f, false},
		{TEXT("0.0.21.2.0.0"), TEXT("caravan"), 1002.8281f, 60.6875f, 240.0f, false},
		{TEXT("0.0.21.3.0.0"), TEXT("reached"), 1067.6406f, 63.3594f, 240.0f, false},
		{TEXT("0.0.21.4.0.0"), TEXT("the"), 1135.1250f, 26.6406f, 240.0f, false},
		{TEXT("0.0.21.5.0.0"), TEXT("old"), 1165.8906f, 24.1250f, 240.0f, false},
		{TEXT("0.0.21.6.0.0"), TEXT("bridge"), 1194.1406f, 50.3125f, 240.0f, false},
	};

	const FChromeLine Lines_ws_contents[] = {
		{TEXT("0.0.23.0.0"), TEXT("aaa"), 1290.0000f, 27.6094f, 200.0f, false},
		{TEXT("0.0.23.1.1.0"), TEXT("bbb"), 1321.7344f, 30.9219f, 200.0f, false},
		{TEXT("0.0.23.2.0"), TEXT("ccc"), 1356.7812f, 28.7188f, 200.0f, false},
	};

	const FChromeCase WsModeCases[] = {
		{TEXT("ws-prewrap"), Lines_ws_prewrap, UE_ARRAY_COUNT(Lines_ws_prewrap)},
		{TEXT("ws-preline"), Lines_ws_preline, UE_ARRAY_COUNT(Lines_ws_preline)},
		{TEXT("ws-for"), Lines_ws_for, UE_ARRAY_COUNT(Lines_ws_for)},
		{TEXT("ws-for-spaced"), Lines_ws_for_spaced, UE_ARRAY_COUNT(Lines_ws_for_spaced)},
		{TEXT("ws-contents"), Lines_ws_contents, UE_ARRAY_COUNT(Lines_ws_contents)},
	};

	const FChromeLine Lines_ws_preblocks[] = {
		{TEXT("0.0.15.0.0"), TEXT("one"), 330.0000f, 29.1719f, 206.0f, false},
		{TEXT("0.0.15.2.0"), TEXT("two"), 330.0000f, 29.9062f, 281.0f, false},
	};

	const FChromeLine Lines_ws_prewrapblocks[] = {
		{TEXT("0.0.43.0.0"), TEXT("one"), 970.0000f, 29.1719f, 606.0f, false},
		{TEXT("0.0.43.2.0"), TEXT("two"), 970.0000f, 29.9062f, 681.0f, false},
	};

	const FChromeLine Lines_ws_prelineblocks[] = {
		{TEXT("0.0.45.0.0"), TEXT("one"), 1290.0000f, 29.1719f, 606.0f, false},
		{TEXT("0.0.45.2.0"), TEXT("two"), 1290.0000f, 29.9062f, 660.0f, false},
	};

	const FChromeLine Lines_ws_preblocks_lf[] = {
		{TEXT("0.0.47.0.0"), TEXT("one"), 10.0000f, 29.1719f, 806.0f, false},
		{TEXT("0.0.47.2.0"), TEXT("two"), 10.0000f, 29.9062f, 860.0f, false},
	};

	const FChromeCase WsPreservedBlockCases[] = {
		{TEXT("ws-preblocks"), Lines_ws_preblocks, UE_ARRAY_COUNT(Lines_ws_preblocks)},
		{TEXT("ws-prewrapblocks"), Lines_ws_prewrapblocks, UE_ARRAY_COUNT(Lines_ws_prewrapblocks)},
		{TEXT("ws-prelineblocks"), Lines_ws_prelineblocks, UE_ARRAY_COUNT(Lines_ws_prelineblocks)},
		{TEXT("ws-preblocks-lf"), Lines_ws_preblocks_lf, UE_ARRAY_COUNT(Lines_ws_preblocks_lf)},
	};

	struct FChromeHeight
	{
		const TCHAR* DomId;
		float Height;
	};

	const FChromeHeight WsPreservedBlockHeights[] = {
		{TEXT("ws-preblocks"), 108.0f},
		{TEXT("ws-prewrapblocks"), 108.0f},
		{TEXT("ws-prelineblocks"), 87.0f},
		{TEXT("ws-preblocks-lf"), 87.0f},
	};

	const FChromeLine Lines_ws_flex[] = {
		{TEXT("0.0.7.1.0"), TEXT("aaa"), 970.0000f, 27.6094f, 10.0f, false},
		{TEXT("0.0.7.3.0"), TEXT("bbb"), 1103.9844f, 30.9219f, 10.0f, false},
		{TEXT("0.0.7.5.0"), TEXT("ccc"), 1241.2812f, 28.7188f, 10.0f, false},
	};

	const FChromeLine Lines_ws_grid[] = {
		{TEXT("0.0.9.1.0"), TEXT("aaa"), 970.0000f, 27.6094f, 60.0f, false},
		{TEXT("0.0.9.3.0"), TEXT("bbb"), 1070.0000f, 30.9219f, 60.0f, false},
		{TEXT("0.0.9.5.0"), TEXT("ccc"), 1170.0000f, 28.7188f, 60.0f, false},
	};

	const FChromeCase WsFlexGridCases[] = {
		{TEXT("ws-flex"), Lines_ws_flex, UE_ARRAY_COUNT(Lines_ws_flex)},
		{TEXT("ws-grid"), Lines_ws_grid, UE_ARRAY_COUNT(Lines_ws_grid)},
	};

	FString Normalize(const FString& Text)
	{
		FString Result;
		bool bPendingSpace = false;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			const TCHAR Character = Text[Index];
			if (Character == TEXT(' ') || Character == TEXT('\t') || Character == TEXT('\n') || Character == TEXT('\r') || Character == TCHAR(0x00A0))
			{
				bPendingSpace = !Result.IsEmpty();
				continue;
			}
			if (bPendingSpace)
			{
				Result.AppendChar(TEXT(' '));
				bPendingSpace = false;
			}
			Result.AppendChar(Character);
		}
		return Result;
	}

	FString DescribeLines(const TArray<FArcRmlTestTextLine>& Lines)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestTextLine& Line : Lines)
		{
			Parts.Add(FString::Printf(TEXT("'%s' x %.3f w %.3f"), *Line.Text, Line.X, Line.Width));
		}
		return FString::Join(Parts, TEXT(" | "));
	}
}

TEST_CLASS(ArcRmlCssParityTextWrapTests, "ArcRmlUI.CssParity.TextWrap")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Name)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(FString::Printf(TEXT("CssParity/Text/%s.rml"), Name));
		FArcRmlTestView::DropCompiledDocument(Path);
		return ArcRmlUITestHelpers::OpenView(View, Path, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork && View.ParseLayout(Elements);
	}

	void ExpectCases(const TCHAR* Fixture, const ArcRmlCssParityTextWrapTestsPrivate::FChromeCase* Cases, int32 NumCases)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(Fixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (int32 CaseIndex = 0; CaseIndex < NumCases; ++CaseIndex)
		{
			const FChromeCase& Case = Cases[CaseIndex];
			TMap<FString, int32> Expected;
			for (int32 Row = 0; Row < Case.NumLines; ++Row)
			{
				++Expected.FindOrAdd(Case.Lines[Row].Path);
			}
			TMap<FString, int32> Seen;
			float FirstBaseline = 0.0f;
			for (int32 Row = 0; Row < Case.NumLines; ++Row)
			{
				const FChromeLine& Chrome = Case.Lines[Row];
				const FArcRmlTestLayoutElement* Node = ArcRmlUITestHelpers::FindByPath(Elements, Chrome.Path);
				ASSERT_THAT(IsNotNull(Node, FString::Printf(TEXT("%s: no node at %s"), Case.DomId, Chrome.Path)));
				ASSERT_THAT(AreEqual(Expected[Chrome.Path], Node->Lines.Num(), FString::Printf(TEXT("%s node %s lines: %s"), Case.DomId, Chrome.Path, *DescribeLines(Node->Lines))));
				const int32 LineIndex = Seen.FindOrAdd(Chrome.Path)++;
				const FArcRmlTestTextLine& Line = Node->Lines[LineIndex];
				const FString Context = FString::Printf(TEXT("%s node %s line %d: %s"), Case.DomId, Chrome.Path, LineIndex, *DescribeLines(Node->Lines));
				ASSERT_THAT(AreEqual(FString(Chrome.Text), Normalize(Line.Text), Context));
				const float PositionLimit = Chrome.bLoose ? LooseTolerance : PositionTolerance;
				const float WidthLimit = Chrome.bLoose ? LooseTolerance : WidthTolerance;
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Line.X, Chrome.X, PositionLimit), FString::Printf(TEXT("x %.3f, Chrome %.3f; %s"), Line.X, Chrome.X, *Context)));
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Line.Width, Chrome.Width, WidthLimit), FString::Printf(TEXT("width %.3f, Chrome %.3f; %s"), Line.Width, Chrome.Width, *Context)));
				if (Row == 0)
				{
					FirstBaseline = Line.Baseline - Chrome.Top;
				}
				ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Line.Baseline - Chrome.Top, FirstBaseline, TopTolerance),
					FString::Printf(TEXT("baseline %.2f is %.2f from the first line's, Chrome top is %.2f; %s"), Line.Baseline, Line.Baseline - Chrome.Top - FirstBaseline, Chrome.Top, *Context)));
			}
		}
	}

	int32 CountLines(const TCHAR* DomId)
	{
		return ArcRmlUITestHelpers::GetLines(Elements, DomId).Num();
	}

	void ExpectSameLineCount(const TCHAR* Fixture, const ArcRmlCssParityTextWrapTestsPrivate::FLineCountPair* Pairs, int32 NumPairs)
	{
		ASSERT_THAT(IsTrue(OpenFixture(Fixture), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (int32 Index = 0; Index < NumPairs; ++Index)
		{
			ASSERT_THAT(AreEqual(CountLines(Pairs[Index].GreedyId), CountLines(Pairs[Index].PrettyId),
				FString::Printf(TEXT("%s has a different line count than %s"), Pairs[Index].PrettyId, Pairs[Index].GreedyId)));
		}
	}

	FString BuildSpreadPattern(const ArcRmlCssParityTextWrapTestsPrivate::FPatternCase& Case, bool& bOutFound)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		bOutFound = false;
		const FArcRmlTestLayoutElement* Justified = ArcRmlUITestHelpers::FindByDomId(Elements, Case.JustifiedId);
		const FArcRmlTestLayoutElement* Left = ArcRmlUITestHelpers::FindByDomId(Elements, Case.LeftId);
		if (!Justified || !Left)
		{
			return FString();
		}
		TArray<FString> Suffixes;
		FString(Case.Suffixes).ParseIntoArray(Suffixes, TEXT(" "));
		TArray<FArcRmlTestTextLine> JustifiedLines;
		TArray<FArcRmlTestTextLine> LeftLines;
		for (const FString& Suffix : Suffixes)
		{
			const FArcRmlTestLayoutElement* JustifiedNode = ArcRmlUITestHelpers::FindByPath(Elements, Justified->Path + TEXT(".") + Suffix);
			const FArcRmlTestLayoutElement* LeftNode = ArcRmlUITestHelpers::FindByPath(Elements, Left->Path + TEXT(".") + Suffix);
			if (!JustifiedNode || !LeftNode || JustifiedNode->Lines.Num() != 1 || LeftNode->Lines.Num() != 1)
			{
				return FString();
			}
			JustifiedLines.Add(JustifiedNode->Lines[0]);
			LeftLines.Add(LeftNode->Lines[0]);
		}
		FString Pattern;
		for (int32 Index = 0; Index < JustifiedLines.Num(); ++Index)
		{
			Pattern.AppendChar(JustifiedLines[Index].Width - LeftLines[Index].Width > SpreadThreshold ? TEXT('I') : TEXT('-'));
			if (Index + 1 < JustifiedLines.Num())
			{
				const float JustifiedGap = JustifiedLines[Index + 1].X - JustifiedLines[Index].X - JustifiedLines[Index].Width;
				const float LeftGap = LeftLines[Index + 1].X - LeftLines[Index].X - LeftLines[Index].Width;
				Pattern.AppendChar(JustifiedGap - LeftGap > SpreadThreshold ? TEXT('G') : TEXT('-'));
				Pattern.AppendChar(TEXT(' '));
			}
		}
		bOutFound = true;
		return Pattern;
	}

	void ExpectPatterns(const FString& Only)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("JustifyTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		int32 Checked = 0;
		for (const FPatternCase& Case : PatternCases)
		{
			if (!Only.IsEmpty() && Only != Case.JustifiedId)
			{
				continue;
			}
			bool bFound = false;
			const FString Pattern = BuildSpreadPattern(Case, bFound);
			ASSERT_THAT(IsTrue(bFound, FString::Printf(TEXT("%s: nodes missing or wrapped"), Case.JustifiedId)));
			ASSERT_THAT(AreEqual(FString(Case.Pattern), Pattern, FString::Printf(TEXT("%s: spread of free space (I inside a text, G after it) differs from Chrome"), Case.JustifiedId)));
			++Checked;
		}
		ASSERT_THAT(IsTrue(Checked > 0));
	}

	TEST_METHOD(PrettyParagraphs_BreakAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyTest"), PrettyParagraphCases, UE_ARRAY_COUNT(PrettyParagraphCases));
	}

	TEST_METHOD(PrettyParagraphs_NeverAddALineToTheGreedyBreak)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectSameLineCount(TEXT("PrettyTest"), PrettyPairs, UE_ARRAY_COUNT(PrettyPairs));
	}

	TEST_METHOD(PrettyCases_NeverAddALineToTheGreedyBreak)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectSameLineCount(TEXT("PrettyCasesTest"), CasePairs, UE_ARRAY_COUNT(CasePairs));
	}

	TEST_METHOD(PrettyBesideFloatsAndAtomicInlines_BreakAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), PrettyFloatCases, UE_ARRAY_COUNT(PrettyFloatCases));
	}

	TEST_METHOD(PrettyWithPreWrapNoWrapAndBreakAll_BreakAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), PrettyModeCases, UE_ARRAY_COUNT(PrettyModeCases));
	}

	TEST_METHOD(PrettyWithGluedWordsAndPadding_BreakAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), PrettyGlueCases, UE_ARRAY_COUNT(PrettyGlueCases));
	}

	TEST_METHOD(LineEndWhiteSpace_RemovedHungAndNbspKeptAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), LineEndCases, UE_ARRAY_COUNT(LineEndCases));
	}

	TEST_METHOD(PreWrapSpacesAtParagraphStart_AreNotKernedWithTheNextWord)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), ParagraphStartCases, UE_ARRAY_COUNT(ParagraphStartCases));
	}

	TEST_METHOD(BreakAllOpportunities_MatchChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("PrettyCasesTest"), BreakAllCases, UE_ARRAY_COUNT(BreakAllCases));
	}

	TEST_METHOD(LayoutDump_ExportsNbspAsU00A0)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("CssParityTextWrapNbsp")));
		IFileManager::Get().DeleteDirectory(*Dir, false, true);
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("PrettyCasesTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.DumpLayout(Dir);
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		FString Html;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, TEXT("PrettyCasesTest.html"))), TEXT("the layout dump wrote no html")));
		ASSERT_THAT(IsTrue(Html.Contains(TEXT("aaaa aaaa \x00A0") TEXT("bbbbbbbbbbbb")), TEXT("the dump text has no U+00A0 where the source has &nbsp;")));
		ASSERT_THAT(IsFalse(Html.Contains(TEXT("&nbsp;")), TEXT("the dump carries a literal &nbsp;")));
		ASSERT_THAT(IsFalse(Html.Contains(TEXT("&amp;nbsp;")), TEXT("the dump escaped the &nbsp; again")));
	}

	TEST_METHOD(JustifiedLines_MatchChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("JustifyTest"), JustifyCases, UE_ARRAY_COUNT(JustifyCases));
	}

	TEST_METHOD(TextAlignLast_MatchesChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("JustifyTest"), JustifyLastCases, UE_ARRAY_COUNT(JustifyLastCases));
	}

	TEST_METHOD(TextJustifyAuto_SpreadsAsChrome)
	{
		ExpectPatterns(TEXT("tj-auto"));
	}

	TEST_METHOD(TextJustifyNone_SpreadsNothing)
	{
		ExpectPatterns(TEXT("tj-none"));
	}

	TEST_METHOD(TextJustifyInterWord_SpreadsAsChrome)
	{
		ExpectPatterns(TEXT("tj-word"));
	}

	TEST_METHOD(TextJustifyInterCharacter_SpreadsAsChrome)
	{
		ExpectPatterns(TEXT("tj-char"));
	}

	TEST_METHOD(TextJustifyDistribute_SpreadsLikeInterCharacter)
	{
		ExpectPatterns(TEXT("tj-dist"));
	}

	TEST_METHOD(TextJustify_IsInherited)
	{
		ExpectPatterns(TEXT("tj-inherit"));
	}

	TEST_METHOD(TextJustifyInterCharacter_LeavesCursiveScriptWhole)
	{
		ExpectPatterns(TEXT("tj-cursive"));
	}

	TEST_METHOD(TextJustifyAuto_LeavesCursiveScriptWhole)
	{
		ExpectPatterns(TEXT("tj-cursive-auto"));
	}

	TEST_METHOD(JustifyAroundEmoji_SpreadsOncePerGraphemeCluster)
	{
		ExpectPatterns(TEXT("je-justify"));
	}

	TEST_METHOD(WhiteSpaceBetweenInlineElements_ShowsAndBreaksAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("WhitespaceTest"), WsInlineCases, UE_ARRAY_COUNT(WsInlineCases));
	}

	TEST_METHOD(WhiteSpaceBetweenBlocks_MakesNoBox)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("WhitespaceTest"), WsBlockCases, UE_ARRAY_COUNT(WsBlockCases));
	}

	TEST_METHOD(WhiteSpaceModesAndDataFor_MatchChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("WhitespaceTest"), WsModeCases, UE_ARRAY_COUNT(WsModeCases));
	}

	TEST_METHOD(PreservedLineFeedsBetweenBlocks_AddLinesAsChrome)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("WhitespaceTest"), WsPreservedBlockCases, UE_ARRAY_COUNT(WsPreservedBlockCases));
		for (const FChromeHeight& Expected : WsPreservedBlockHeights)
		{
			const FArcRmlTestLayoutElement* Container = ArcRmlUITestHelpers::FindByDomId(Elements, Expected.DomId);
			ASSERT_THAT(IsNotNull(Container, FString::Printf(TEXT("no element #%s"), Expected.DomId)));
			ASSERT_THAT(IsNear(Expected.Height, Container->Rect.Size.Y, 0.05f, FString::Printf(TEXT("#%s height"), Expected.DomId)));
		}
	}

	TEST_METHOD(WhiteSpaceInFlexAndGrid_MakesNoBox)
	{
		using namespace ArcRmlCssParityTextWrapTestsPrivate;
		ExpectCases(TEXT("WhitespaceTest"), WsFlexGridCases, UE_ARRAY_COUNT(WsFlexGridCases));
	}

	TEST_METHOD(DivHoldingOnlyWhiteSpace_IsNotEmpty)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("WhitespaceTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* WithSpace = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ws-empty-space"));
		const FArcRmlTestLayoutElement* Without = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ws-empty-none"));
		ASSERT_THAT(IsNotNull(WithSpace));
		ASSERT_THAT(IsNotNull(Without));
		ASSERT_THAT(IsNear(40.0f, WithSpace->Rect.Size.X, 0.05f, TEXT("<div> </div> matched :empty")));
		ASSERT_THAT(IsNear(80.0f, Without->Rect.Size.X, 0.05f, TEXT("<div></div> did not match :empty")));
	}
};
