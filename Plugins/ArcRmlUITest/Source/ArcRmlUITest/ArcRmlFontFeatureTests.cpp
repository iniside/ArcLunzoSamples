// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlFontFeatureTestsPrivate
{
	constexpr float WidthTolerance = 0.5f;

	struct FFeatureCase
	{
		const TCHAR* DomId;
		float ChromeWidth;
	};

	// Same order as the fixture: the draw colour of case i is (0x20, 0x30 + 6 * i, 0x90). Chrome widths come from
	// chrome_layout_compare.py --lines on the dump of CssParity/Text/FontFeatureTest.rml.
	const FFeatureCase Cases[] = {
		{TEXT("sg-p1"), 40.1406f},
		{TEXT("sg-p0"), 61.5469f},
		{TEXT("sg-t1"), 59.5312f},
		{TEXT("sg-t0"), 59.5312f},
		{TEXT("sg-f1"), 59.5312f},
		{TEXT("sg-tn-off"), 40.1406f},
		{TEXT("sg-z"), 15.3906f},
		{TEXT("sg-zs"), 15.3906f},
		{TEXT("sg-zf"), 15.3906f},
		{TEXT("sg-fr-n"), 33.3906f},
		{TEXT("sg-fr"), 19.7812f},
		{TEXT("sg-o-n"), 53.1406f},
		{TEXT("sg-o"), 45.0781f},
		{TEXT("sg-l-n"), 15.9219f},
		{TEXT("sg-l-0"), 15.9219f},
		{TEXT("bc-n"), 102.1250f},
		{TEXT("bc-sc"), 105.8438f},
		{TEXT("bc-asc"), 104.1250f},
		{TEXT("jb-n"), 57.6094f},
		{TEXT("jb-t"), 57.6094f},
		{TEXT("sg-sep"), 102.3125f},
		{TEXT("sg-sp"), 108.4844f},
		{TEXT("sg-nosep"), 96.1562f},
	};

	FColor ColourOf(const FString& DomId)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cases); ++Index)
		{
			if (DomId == Cases[Index].DomId)
			{
				return FColor(0x20, 0x30 + 6 * Index, 0x90, 0xff);
			}
		}
		return FColor::Transparent;
	}

	bool SameRect(const FBox2f& A, const FBox2f& B)
	{
		return A.Min.Equals(B.Min, 0.01f) && A.Max.Equals(B.Max, 0.01f);
	}

	bool SameGlyphs(const TArray<FArcRmlTestDrawQuad>& A, const TArray<FArcRmlTestDrawQuad>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!SameRect(A[Index].AtlasRect, B[Index].AtlasRect))
			{
				return false;
			}
		}
		return true;
	}

	FString DescribeGlyphs(const TArray<FArcRmlTestDrawQuad>& Quads)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestDrawQuad& Quad : Quads)
		{
			Parts.Add(FString::Printf(TEXT("(%.1f,%.1f)"), Quad.AtlasRect.Min.X, Quad.AtlasRect.Min.Y));
		}
		return FString::Join(Parts, TEXT(" "));
	}
}

TEST_CLASS(ArcRmlFontFeatureTests, "ArcRmlUI.FontFeatures")
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
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0 && !View.GetProbe().bPendingAsyncWork
			&& View.ParseLayout(Elements);
	}

	float WidthOf(const TCHAR* DomId)
	{
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, DomId);
		return Lines.Num() == 1 ? Lines[0].Width : -1.0f;
	}

	bool SettleFrames()
	{
		for (int32 Frame = 0; Frame < 4; ++Frame)
		{
			FPlatformProcess::Sleep(0.05f);
			if (!View.Flush())
			{
				return false;
			}
		}
		return true;
	}

	// Outlined glyph quads of one fixture line, left to right; spaces draw no quad with an outline.
	TArray<FArcRmlTestDrawQuad> GlyphsOf(const TCHAR* DomId)
	{
		return GlyphsWithColour(ArcRmlFontFeatureTestsPrivate::ColourOf(DomId));
	}

	TArray<FArcRmlTestDrawQuad> GlyphsWithColour(const FColor& Colour)
	{
		TArray<FArcRmlTestDrawQuad> Outlined;
		for (const FArcRmlTestDrawQuad& Quad : ArcRmlUITestHelpers::GetGlyphQuads(View.GetDraws(), Colour))
		{
			if (Quad.Width > 0.0f && Quad.Height > 0.0f)
			{
				Outlined.Add(Quad);
			}
		}
		return Outlined;
	}

	TEST_METHOD(Fixture_ParsesWithoutDiagnostics)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(Widths_MatchChromeOracle)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const FFeatureCase& Case : Cases)
		{
			ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(Case.DomId), Case.ChromeWidth, WidthTolerance),
				FString::Printf(TEXT("%s is %.4f px, Chrome %.4f"), Case.DomId, WidthOf(Case.DomId), Case.ChromeWidth)));
		}
	}

	TEST_METHOD(SpaceGrotesk_TabularNums_MakeDigitsEqualWidth)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FMath::Abs(WidthOf(TEXT("sg-p1")) - WidthOf(TEXT("sg-p0"))) > WidthTolerance,
			FString::Printf(TEXT("the control is not proportional: 1111 is %.4f px, 0000 %.4f px"), WidthOf(TEXT("sg-p1")), WidthOf(TEXT("sg-p0")))));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("sg-t1")), WidthOf(TEXT("sg-t0")), WidthTolerance),
			FString::Printf(TEXT("tabular-nums: 1111 is %.4f px, 0000 %.4f px"), WidthOf(TEXT("sg-t1")), WidthOf(TEXT("sg-t0")))));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("sg-f1")), WidthOf(TEXT("sg-t1")), WidthTolerance),
			FString::Printf(TEXT("\"tnum\" 1 gives the width of tabular-nums: %.4f px against %.4f px"), WidthOf(TEXT("sg-f1")), WidthOf(TEXT("sg-t1")))));
	}

	TEST_METHOD(ClassAddedAtRuntime_TabularNumsReshapesTheLine)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const float Proportional = WidthOf(TEXT("sg-p1"));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Proportional, 40.1406f, WidthTolerance), FString::Printf(TEXT("before the class, 1111 is %.4f px, Chrome 40.1406"), Proportional)));
		View.SetRootClass(TEXT("tab"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("sg-p1")), 59.5312f, WidthTolerance),
			FString::Printf(TEXT("after the class sets tabular-nums, 1111 is %.4f px, Chrome 59.5312"), WidthOf(TEXT("sg-p1")))));
	}

	TEST_METHOD(FontFeatureSettings_OverrideFontVariantNumeric)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FMath::Abs(WidthOf(TEXT("sg-p1")) - WidthOf(TEXT("sg-t1"))) > WidthTolerance,
			FString::Printf(TEXT("tabular-nums changes 1111: %.4f px against %.4f px"), WidthOf(TEXT("sg-t1")), WidthOf(TEXT("sg-p1")))));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("sg-tn-off")), WidthOf(TEXT("sg-p1")), WidthTolerance),
			FString::Printf(TEXT("tabular-nums with \"tnum\" 0 is %.4f px, proportional %.4f px"), WidthOf(TEXT("sg-tn-off")), WidthOf(TEXT("sg-p1")))));
		ASSERT_THAT(IsTrue(SameGlyphs(GlyphsOf(TEXT("sg-tn-off")), GlyphsOf(TEXT("sg-p1"))), TEXT("the overridden line draws the proportional glyphs")));
	}

	TEST_METHOD(SpaceGrotesk_SlashedZeroAndZeroFeature_DrawADifferentZero)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Plain = GlyphsOf(TEXT("sg-z"));
		const TArray<FArcRmlTestDrawQuad> Slashed = GlyphsOf(TEXT("sg-zs"));
		const TArray<FArcRmlTestDrawQuad> Feature = GlyphsOf(TEXT("sg-zf"));
		ASSERT_THAT(AreEqual(1, Plain.Num()));
		ASSERT_THAT(AreEqual(1, Slashed.Num()));
		ASSERT_THAT(AreEqual(1, Feature.Num()));
		ASSERT_THAT(IsFalse(SameGlyphs(Plain, Slashed), FString::Printf(TEXT("slashed-zero draws the plain zero: %s"), *DescribeGlyphs(Slashed))));
		ASSERT_THAT(IsTrue(SameGlyphs(Slashed, Feature), FString::Printf(TEXT("\"zero\" 1 draws %s, slashed-zero %s"), *DescribeGlyphs(Feature), *DescribeGlyphs(Slashed))));
	}

	TEST_METHOD(SpaceGrotesk_DiagonalFractions_ReplaceTheSlashedFraction)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Plain = GlyphsOf(TEXT("sg-fr-n"));
		const TArray<FArcRmlTestDrawQuad> Fraction = GlyphsOf(TEXT("sg-fr"));
		ASSERT_THAT(AreEqual(3, Plain.Num()));
		ASSERT_THAT(AreEqual(3, Fraction.Num()));
		ASSERT_THAT(IsFalse(SameGlyphs(Plain, Fraction), FString::Printf(TEXT("1/2 with diagonal-fractions draws %s, as without %s"), *DescribeGlyphs(Fraction), *DescribeGlyphs(Plain))));
		ASSERT_THAT(IsTrue(FMath::Abs(WidthOf(TEXT("sg-fr")) - WidthOf(TEXT("sg-fr-n"))) > WidthTolerance,
			FString::Printf(TEXT("the fraction glyphs have other advances: %.4f px against %.4f px"), WidthOf(TEXT("sg-fr")), WidthOf(TEXT("sg-fr-n")))));
	}

	TEST_METHOD(SpaceGrotesk_Ordinal_ReplacesALetterAfterADigit)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Plain = GlyphsOf(TEXT("sg-o-n"));
		const TArray<FArcRmlTestDrawQuad> Ordinal = GlyphsOf(TEXT("sg-o"));
		ASSERT_THAT(AreEqual(4, Plain.Num()));
		ASSERT_THAT(AreEqual(4, Ordinal.Num()));
		ASSERT_THAT(IsFalse(SameGlyphs(Plain, Ordinal), FString::Printf(TEXT("1a2o with ordinal draws %s, as without %s"), *DescribeGlyphs(Ordinal), *DescribeGlyphs(Plain))));
		ASSERT_THAT(IsTrue(FMath::Abs(WidthOf(TEXT("sg-o")) - WidthOf(TEXT("sg-o-n"))) > WidthTolerance,
			FString::Printf(TEXT("the ordinal letters have other advances: %.4f px against %.4f px"), WidthOf(TEXT("sg-o")), WidthOf(TEXT("sg-o-n")))));
	}

	TEST_METHOD(SpaceGrotesk_LigaturesNoneAgainstNormal_SplitsFi)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, GlyphsOf(TEXT("sg-l-n")).Num(), TEXT("normal draws fi as one ligature glyph")));
		ASSERT_THAT(AreEqual(2, GlyphsOf(TEXT("sg-l-0")).Num(), TEXT("none draws f and i")));
	}

	TEST_METHOD(BarlowCondensed_SmallCapsAndAllSmallCaps_UseSmcpAndC2sc)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestDrawQuad> Plain = GlyphsOf(TEXT("bc-n"));
		const TArray<FArcRmlTestDrawQuad> Small = GlyphsOf(TEXT("bc-sc"));
		const TArray<FArcRmlTestDrawQuad> All = GlyphsOf(TEXT("bc-asc"));
		ASSERT_THAT(AreEqual(10, Plain.Num()));
		ASSERT_THAT(AreEqual(10, Small.Num()));
		ASSERT_THAT(AreEqual(10, All.Num()));
		ASSERT_THAT(IsTrue(SameRect(Plain[0].AtlasRect, Small[0].AtlasRect), TEXT("small-caps keeps the capital S")));
		ASSERT_THAT(IsFalse(SameRect(Plain[1].AtlasRect, Small[1].AtlasRect), TEXT("small-caps replaces the lower case m by a small capital")));
		ASSERT_THAT(IsFalse(SameRect(Plain[0].AtlasRect, All[0].AtlasRect), TEXT("all-small-caps replaces the capital S too")));
		ASSERT_THAT(IsTrue(FMath::Abs(WidthOf(TEXT("bc-asc")) - WidthOf(TEXT("bc-sc"))) > WidthTolerance && FMath::Abs(WidthOf(TEXT("bc-sc")) - WidthOf(TEXT("bc-n"))) > WidthTolerance,
			FString::Printf(TEXT("widths normal %.4f, small-caps %.4f, all-small-caps %.4f px"), WidthOf(TEXT("bc-n")), WidthOf(TEXT("bc-sc")), WidthOf(TEXT("bc-asc")))));
	}

	TEST_METHOD(JetBrainsMono_TabularNums_IsUnchanged)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("jb-t")), WidthOf(TEXT("jb-n")), WidthTolerance),
			FString::Printf(TEXT("tabular-nums in the monospaced face: %.4f px against %.4f px"), WidthOf(TEXT("jb-t")), WidthOf(TEXT("jb-n")))));
		ASSERT_THAT(AreEqual(4, GlyphsOf(TEXT("jb-n")).Num()));
		ASSERT_THAT(IsTrue(SameGlyphs(GlyphsOf(TEXT("jb-t")), GlyphsOf(TEXT("jb-n"))), TEXT("the face has no tnum, so the glyphs stay")));
	}

	TEST_METHOD(NarrowNoBreakSpaceGroups_AdvanceHalfASpaceAndDrawNoReplacementGlyph)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureTest")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(7, GlyphsOf(TEXT("sg-sep")).Num(), TEXT("the seven digits only, no replacement glyph for U+202F")));
		const float Digits = WidthOf(TEXT("sg-nosep"));
		const float TwoSpaces = WidthOf(TEXT("sg-sp")) - Digits;
		const float TwoSeparators = WidthOf(TEXT("sg-sep")) - Digits;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(TwoSeparators, TwoSpaces * 0.5f, WidthTolerance),
			FString::Printf(TEXT("two U+202F advance %.4f px, two spaces %.4f px, so each separator should be half a space"), TwoSeparators, TwoSpaces)));
	}

	TEST_METHOD(InvalidValuesAreDroppedAndANegativeFeatureValueTurnsTheFeatureOnAsChrome)
	{
		using namespace ArcRmlFontFeatureTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("FontFeatureInvalid")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TCHAR* const Dropped[] = {
			TEXT("Declaration 'font-variant-numeric: tabular-nums bogus;' is dropped: invalid value."),
			TEXT("Declaration 'font-variant-caps: sideways;' is dropped: invalid value."),
			TEXT("Declaration 'font-variant-ligatures: common-ligatures no-common-ligatures;' is dropped: invalid value."),
			TEXT("Declaration 'font-feature-settings: tnum;' is dropped: invalid value."),
		};
		for (const TCHAR* Message : Dropped)
		{
			ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, Message).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		}
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("Declaration 'font-feature-settings: \"tnum\" -1;' is dropped: invalid value.")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("inv-num")), WidthOf(TEXT("ctl-num")), 0.01f), TEXT("the dropped numeric value leaves the digits proportional")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("inv-set")), WidthOf(TEXT("ctl-num")), 0.01f), TEXT("the unquoted tag leaves the digits proportional")));
		constexpr float ChromeNegativeTnumWidth = 59.53f;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("inv-val")), ChromeNegativeTnumWidth, 0.5f),
			FString::Printf(TEXT("\"tnum\" -1 turns tabular digits on: %.4f px, Chrome %.2f px, proportional %.4f px"), WidthOf(TEXT("inv-val")), ChromeNegativeTnumWidth, WidthOf(TEXT("ctl-num")))));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(WidthOf(TEXT("inv-caps")), WidthOf(TEXT("ctl-caps")), 0.01f), TEXT("the dropped caps keyword leaves the capitals")));
		ASSERT_THAT(AreEqual(1, GlyphsWithColour(FColor(0x20, 0x30, 0x90, 0xff)).Num(), TEXT("the dropped ligature value leaves fi as one ligature glyph")));
		ASSERT_THAT(AreEqual(1, GlyphsWithColour(FColor(0x20, 0x36, 0x90, 0xff)).Num(), TEXT("the control draws fi as one ligature glyph")));
	}
};
