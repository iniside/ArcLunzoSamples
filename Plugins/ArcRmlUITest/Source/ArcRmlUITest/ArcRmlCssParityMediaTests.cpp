// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityMediaTestsPrivate
{
	struct FMediaCase
	{
		const TCHAR* Id;
		uint32 Rgb;
		bool bMatches;
	};

	struct FInvalidQuery
	{
		int32 Line;
		const TCHAR* Query;
	};

	struct FTopic
	{
		const TCHAR* const* Ids;
		int32 Count;
	};

	const FMediaCase Cases[] = {
		{TEXT("w-ge-1280"), 0x010101, true},
		{TEXT("w-gt-1280"), 0x020202, false},
		{TEXT("h-700-721"), 0x030303, true},
		{TEXT("ar-gt-16-10"), 0x040404, true},
		{TEXT("ar-le-4-3"), 0x050505, false},
		{TEXT("res-1-2"), 0x060606, true},
		{TEXT("res-gt-1"), 0x070707, false},
		{TEXT("w-ge-2000"), 0x080808, false},
		{TEXT("malformed"), 0x090909, false},
		{TEXT("w-reversed"), 0x0a0a0a, true},
		{TEXT("w-400-800"), 0x0b0b0b, false},
		{TEXT("w-1000-1280"), 0x0c0c0c, true},
		{TEXT("min-width"), 0x0d0d0d, true},
		{TEXT("max-width"), 0x0e0e0e, false},
		{TEXT("w-eq-1280"), 0x0f0f0f, true},
		{TEXT("ar-bare"), 0x101010, false},
		{TEXT("ar-eq-16-9"), 0x111111, true},
		{TEXT("not-w"), 0x121212, true},
		{TEXT("screen-and"), 0x131313, true},
		{TEXT("print"), 0x141414, false},
		{TEXT("not-print"), 0x151515, true},
		{TEXT("comma-list"), 0x161616, true},
		{TEXT("mixed-ops"), 0x171717, false},
		{TEXT("landscape"), 0x181818, true},
		{TEXT("min-orientation"), 0x191919, false},
		{TEXT("no-space"), 0x1a1a1a, true},
		{TEXT("fn-not"), 0x1b1b40, false},
		{TEXT("fn-and"), 0x1c1c40, false},
		{TEXT("fn-term"), 0x1d1d40, false},
		{TEXT("fn-or"), 0x1e1e40, true},
		{TEXT("dpi-ge"), 0x1f1f40, true},
		{TEXT("dpi-lt"), 0x202040, true},
		{TEXT("dpcm-ge"), 0x212140, true},
		{TEXT("dpcm-gt"), 0x222240, false},
		{TEXT("dpi-plain"), 0x232340, true},
		{TEXT("res-calc"), 0x242440, true},
		{TEXT("res-calc-dpi"), 0x252540, true},
		{TEXT("res-neg"), 0x262640, false},
		{TEXT("res-zero"), 0x272740, true},
		{TEXT("bool-width"), 0x282840, true},
		{TEXT("bool-height"), 0x292940, true},
		{TEXT("bool-ar"), 0x2a2a40, true},
		{TEXT("bool-res"), 0x2b2b40, true},
		{TEXT("bool-orient"), 0x2c2c40, true},
		{TEXT("bool-min"), 0x2d2d40, false},
		{TEXT("bool-not"), 0x2e2e40, false},
		{TEXT("screen-not"), 0x2f2f40, true},
		{TEXT("or"), 0x303040, true},
		{TEXT("not-or"), 0x313140, true},
		{TEXT("nested-and"), 0x323240, true},
		{TEXT("mixed-and-or"), 0x333340, false},
		{TEXT("not-not"), 0x343440, false},
		{TEXT("unknown-or"), 0x353540, true},
		{TEXT("unknown-and"), 0x363640, false},
		{TEXT("not-unknown"), 0x373740, false},
		{TEXT("not-screen-unknown"), 0x383840, false},
		{TEXT("not-tv-unknown"), 0x393940, true},
		{TEXT("all-and-or"), 0x3a3a40, false},
		{TEXT("only-screen"), 0x3b3b40, true},
		{TEXT("only-cond"), 0x3c3c40, false},
		{TEXT("empty-parens"), 0x3d3d40, false},
		{TEXT("calc-width"), 0x3e3e40, true},
		{TEXT("calc-mixed"), 0x3f3f40, true},
		{TEXT("calc-neg"), 0x404040, true},
		{TEXT("em-le"), 0x414140, true},
		{TEXT("em-gt"), 0x424240, false},
		{TEXT("rem-le"), 0x434340, true},
		{TEXT("em-plain"), 0x444440, false},
		{TEXT("vw"), 0x454540, true},
		{TEXT("neg-ge"), 0x464640, true},
		{TEXT("neg-not-lt"), 0x474740, true},
		{TEXT("unitless"), 0x484840, false},
		{TEXT("zero"), 0x494940, true},
		{TEXT("exp-len"), 0x4a4a40, true},
		{TEXT("ar-calc"), 0x4b4b40, true},
		{TEXT("ar-inf"), 0x4c4c40, false},
		{TEXT("ar-hex"), 0x4d4d40, false},
		{TEXT("ar-exp"), 0x4e4e40, true},
		{TEXT("ar-dot"), 0x4f4f40, true},
		{TEXT("ar-trailing-dot"), 0x505040, false},
		{TEXT("ar-plus"), 0x515140, true},
		{TEXT("ar-inf-ratio"), 0x525240, true},
		{TEXT("ar-zero-zero"), 0x535340, true},
		{TEXT("ar-zero"), 0x545440, true},
		{TEXT("ar-exact"), 0x555540, true},
		{TEXT("upper"), 0x565640, true},
		{TEXT("layer"), 0x575740, false},
		{TEXT("not-orient-foo"), 0x585840, false},
		{TEXT("hover-b"), 0x303080, true},
		{TEXT("hover-hover"), 0x313180, true},
		{TEXT("hover-none"), 0x323280, false},
		{TEXT("hover-foo"), 0x333380, false},
		{TEXT("hover-range"), 0x343480, false},
		{TEXT("hover-min"), 0x353580, false},
		{TEXT("anyhover-b"), 0x363680, true},
		{TEXT("anyhover-hover"), 0x373780, true},
		{TEXT("anyhover-none"), 0x383880, false},
		{TEXT("pointer-b"), 0x393980, true},
		{TEXT("pointer-fine"), 0x3a3a80, true},
		{TEXT("pointer-coarse"), 0x3b3b80, false},
		{TEXT("pointer-none"), 0x3c3c80, false},
		{TEXT("anypointer-b"), 0x3d3d80, true},
		{TEXT("anypointer-fine"), 0x3e3e80, true},
		{TEXT("anypointer-coarse"), 0x3f3f80, false},
		{TEXT("color-b"), 0x404080, true},
		{TEXT("color-8"), 0x414180, true},
		{TEXT("color-ge8"), 0x424280, true},
		{TEXT("color-gt8"), 0x434380, false},
		{TEXT("color-min"), 0x444480, true},
		{TEXT("color-max7"), 0x454580, false},
		{TEXT("color-0"), 0x464680, false},
		{TEXT("color-frac"), 0x474780, false},
		{TEXT("color-neg"), 0x484880, true},
		{TEXT("color-px"), 0x494980, false},
		{TEXT("color-calc"), 0x4a4a80, true},
		{TEXT("color-range2"), 0x4b4b80, true},
		{TEXT("ci-b"), 0x4c4c80, false},
		{TEXT("ci-0"), 0x4d4d80, true},
		{TEXT("ci-min"), 0x4e4e80, true},
		{TEXT("ci-gt0"), 0x4f4f80, false},
		{TEXT("mono-b"), 0x505080, false},
		{TEXT("mono-0"), 0x515180, true},
		{TEXT("mono-max"), 0x525280, true},
		{TEXT("mono-lt1"), 0x535380, true},
		{TEXT("prm-b"), 0x545480, false},
		{TEXT("prm-np"), 0x555580, true},
		{TEXT("prm-reduce"), 0x565680, false},
		{TEXT("prm-foo"), 0x575780, false},
		{TEXT("pcs-b"), 0x585880, true},
		{TEXT("pcs-light"), 0x595980, true},
		{TEXT("pcs-dark"), 0x5a5a80, false},
		{TEXT("pcs-np"), 0x5b5b80, false},
		{TEXT("pc-b"), 0x5c5c80, false},
		{TEXT("pc-np"), 0x5d5d80, true},
		{TEXT("pc-more"), 0x5e5e80, false},
		{TEXT("pc-less"), 0x5f5f80, false},
		{TEXT("pc-custom"), 0x606080, false},
		{TEXT("fc-b"), 0x616180, false},
		{TEXT("fc-none"), 0x626280, true},
		{TEXT("fc-active"), 0x636380, false},
		{TEXT("ic-b"), 0x646480, false},
		{TEXT("ic-none"), 0x656580, false},
		{TEXT("ic-inverted"), 0x666680, false},
		{TEXT("dr-b"), 0x676780, true},
		{TEXT("dr-standard"), 0x686880, true},
		{TEXT("dr-high"), 0x696980, false},
		{TEXT("upd-b"), 0x6a6a80, true},
		{TEXT("upd-fast"), 0x6b6b80, true},
		{TEXT("upd-slow"), 0x6c6c80, false},
		{TEXT("upd-none"), 0x6d6d80, false},
		{TEXT("scr-b"), 0x6e6e80, false},
		{TEXT("scr-none"), 0x6f6f80, true},
		{TEXT("scr-enabled"), 0x707080, false},
		{TEXT("scr-initial"), 0x717180, false},
		{TEXT("upper-hover"), 0x727280, true},
		{TEXT("cg-b"), 0xa0a010, true},
		{TEXT("cg-srgb"), 0xa1a110, true},
		{TEXT("cg-p3"), 0xa2a210, false},
		{TEXT("cg-rec2020"), 0xa3a310, false},
		{TEXT("cg-foo"), 0xa4a410, false},
		{TEXT("cg-min"), 0xa5a510, false},
		{TEXT("cg-range"), 0xa6a610, false},
		{TEXT("prt-b"), 0xa7a710, false},
		{TEXT("prt-np"), 0xa8a810, true},
		{TEXT("prt-reduce"), 0xa9a910, false},
		{TEXT("prt-foo"), 0xaaaa10, false},
		{TEXT("ic-not-none"), 0xabab10, false},
		{TEXT("ic-not-b"), 0xacac10, false},
	};

	const FInvalidQuery InvalidQueries[] = {
		{21, TEXT("(width >> 3px)")},
		{35, TEXT("(400px < width > 800px)")},
		{37, TEXT("(min-orientation: landscape)")},
		{39, TEXT("not(width > 2000px)")},
		{40, TEXT("screen and(width >= 1px)")},
		{41, TEXT("(width > 1px) and foo(bar)")},
		{42, TEXT("foo(bar) or (width > 1px)")},
		{50, TEXT("not (resolution > -1dppx)")},
		{57, TEXT("(min-width)")},
		{63, TEXT("(width > 2000px) and (height = 720px) or (width = 1280px)")},
		{65, TEXT("(foo) or (width > 1px)")},
		{66, TEXT("(foo) and (width > 1px)")},
		{67, TEXT("not (foo)")},
		{68, TEXT("not screen and (foo)")},
		{69, TEXT("not tv and (foo)")},
		{70, TEXT("all and (width > 1px) or (height > 1px)")},
		{72, TEXT("only (width > 1px)")},
		{73, TEXT("()")},
		{84, TEXT("not (width >= 100)")},
		{88, TEXT("not (aspect-ratio: inf)")},
		{89, TEXT("not (aspect-ratio > 0x10)")},
		{92, TEXT("not (aspect-ratio > 1.)")},
		{99, TEXT("layer")},
		{100, TEXT("not (orientation: foo)")},
		{104, TEXT("not (hover: foo)")},
		{105, TEXT("not (hover >= hover)")},
		{106, TEXT("not (min-hover: hover)")},
		{124, TEXT("not (color >= 1.5)")},
		{126, TEXT("not (color: 8px)")},
		{140, TEXT("not (prefers-reduced-motion: foo)")},
		{144, TEXT("not (prefers-color-scheme: no-preference)")},
		{153, TEXT("(inverted-colors)")},
		{154, TEXT("(inverted-colors: none)")},
		{155, TEXT("(inverted-colors: inverted)")},
		{172, TEXT("not (color-gamut: foo)")},
		{173, TEXT("not (min-color-gamut: srgb)")},
		{174, TEXT("not (color-gamut >= srgb)")},
		{178, TEXT("not (prefers-reduced-transparency: foo)")},
		{179, TEXT("not (inverted-colors: none)")},
		{180, TEXT("not (inverted-colors)")},
	};

	const TCHAR* const RangeFormsIds[] = {TEXT("w-ge-1280"), TEXT("w-gt-1280"), TEXT("h-700-721"), TEXT("ar-gt-16-10"), TEXT("ar-le-4-3"), TEXT("res-1-2"), TEXT("res-gt-1"), TEXT("w-ge-2000"), TEXT("malformed"), TEXT("w-reversed"), TEXT("w-eq-1280"), TEXT("ar-bare"), TEXT("ar-eq-16-9"), TEXT("ar-exact")};

	const TCHAR* const MinMaxIds[] = {TEXT("min-width"), TEXT("max-width"), TEXT("bool-min"), TEXT("dpi-plain"), TEXT("color-min"), TEXT("color-max7"), TEXT("ci-min"), TEXT("mono-max"), TEXT("min-orientation")};

	const TCHAR* const TwoSidedIds[] = {TEXT("h-700-721"), TEXT("w-400-800"), TEXT("w-1000-1280"), TEXT("res-1-2"), TEXT("w-reversed"), TEXT("mixed-ops"), TEXT("color-range2")};

	const TCHAR* const QueryListsAndTypesIds[] = {TEXT("screen-and"), TEXT("print"), TEXT("not-print"), TEXT("comma-list"), TEXT("all-and-or"), TEXT("only-screen"), TEXT("only-cond"), TEXT("not-screen-unknown"), TEXT("not-tv-unknown"), TEXT("layer"), TEXT("upper"), TEXT("upper-hover"), TEXT("landscape"), TEXT("min-orientation")};

	const TCHAR* const NotAndOnlyIds[] = {TEXT("not-w"), TEXT("not-not"), TEXT("not-or"), TEXT("not-print"), TEXT("only-screen"), TEXT("only-cond"), TEXT("not-screen-unknown"), TEXT("not-tv-unknown"), TEXT("screen-not"), TEXT("bool-not")};

	const TCHAR* const FunctionTokensIds[] = {TEXT("no-space"), TEXT("fn-not"), TEXT("fn-and"), TEXT("fn-term"), TEXT("fn-or")};

	const TCHAR* const Mq4GrammarIds[] = {TEXT("or"), TEXT("not-or"), TEXT("nested-and"), TEXT("mixed-and-or"), TEXT("not-not"), TEXT("unknown-or"), TEXT("unknown-and"), TEXT("not-unknown"), TEXT("empty-parens"), TEXT("all-and-or"), TEXT("screen-not"), TEXT("comma-list"), TEXT("mixed-ops")};

	const TCHAR* const CalcEmRemVwIds[] = {TEXT("calc-width"), TEXT("calc-mixed"), TEXT("calc-neg"), TEXT("em-le"), TEXT("em-gt"), TEXT("rem-le"), TEXT("em-plain"), TEXT("vw"), TEXT("neg-ge"), TEXT("neg-not-lt"), TEXT("unitless"), TEXT("zero"), TEXT("exp-len")};

	const TCHAR* const ResolutionIds[] = {TEXT("res-1-2"), TEXT("res-gt-1"), TEXT("dpi-ge"), TEXT("dpi-lt"), TEXT("dpcm-ge"), TEXT("dpcm-gt"), TEXT("dpi-plain"), TEXT("res-calc"), TEXT("res-calc-dpi"), TEXT("res-neg"), TEXT("res-zero")};

	const TCHAR* const RatiosIds[] = {TEXT("ar-gt-16-10"), TEXT("ar-le-4-3"), TEXT("ar-bare"), TEXT("ar-eq-16-9"), TEXT("ar-calc"), TEXT("ar-inf"), TEXT("ar-hex"), TEXT("ar-exp"), TEXT("ar-dot"), TEXT("ar-trailing-dot"), TEXT("ar-plus"), TEXT("ar-inf-ratio"), TEXT("ar-zero-zero"), TEXT("ar-zero"), TEXT("ar-exact")};

	const TCHAR* const BooleanFormsIds[] = {TEXT("bool-width"), TEXT("bool-height"), TEXT("bool-ar"), TEXT("bool-res"), TEXT("bool-orient"), TEXT("bool-min"), TEXT("bool-not"), TEXT("hover-b"), TEXT("anyhover-b"), TEXT("pointer-b"), TEXT("anypointer-b"), TEXT("color-b"), TEXT("ci-b"), TEXT("mono-b"), TEXT("prm-b"), TEXT("pcs-b"), TEXT("pc-b"), TEXT("fc-b"), TEXT("dr-b"), TEXT("upd-b"), TEXT("cg-b"), TEXT("prt-b"), TEXT("ic-b")};

	const TCHAR* const InteractionIds[] = {TEXT("hover-b"), TEXT("hover-hover"), TEXT("hover-none"), TEXT("hover-foo"), TEXT("hover-range"), TEXT("hover-min"), TEXT("anyhover-b"), TEXT("anyhover-hover"), TEXT("anyhover-none"), TEXT("pointer-b"), TEXT("pointer-fine"), TEXT("pointer-coarse"), TEXT("pointer-none"), TEXT("anypointer-b"), TEXT("anypointer-fine"), TEXT("anypointer-coarse"), TEXT("upper-hover"), TEXT("upd-b"), TEXT("upd-fast"), TEXT("upd-slow"), TEXT("upd-none"), TEXT("landscape"), TEXT("not-orient-foo"), TEXT("bool-orient")};

	const TCHAR* const PreferencesIds[] = {TEXT("prm-b"), TEXT("prm-np"), TEXT("prm-reduce"), TEXT("prm-foo"), TEXT("pcs-b"), TEXT("pcs-light"), TEXT("pcs-dark"), TEXT("pcs-np"), TEXT("pc-b"), TEXT("pc-np"), TEXT("pc-more"), TEXT("pc-less"), TEXT("pc-custom"), TEXT("fc-b"), TEXT("fc-none"), TEXT("fc-active"), TEXT("dr-b"), TEXT("dr-standard"), TEXT("dr-high"), TEXT("prt-b"), TEXT("prt-np"), TEXT("prt-reduce"), TEXT("prt-foo")};

	const TCHAR* const DroppedInvertedColorsIds[] = {TEXT("ic-b"), TEXT("ic-none"), TEXT("ic-inverted"), TEXT("ic-not-none"), TEXT("ic-not-b")};

	const TCHAR* const ScriptingIds[] = {TEXT("scr-b"), TEXT("scr-none"), TEXT("scr-enabled"), TEXT("scr-initial")};

	const TCHAR* const ColourIds[] = {TEXT("color-b"), TEXT("color-8"), TEXT("color-ge8"), TEXT("color-gt8"), TEXT("color-min"), TEXT("color-max7"), TEXT("color-0"), TEXT("color-frac"), TEXT("color-neg"), TEXT("color-px"), TEXT("color-calc"), TEXT("color-range2"), TEXT("ci-b"), TEXT("ci-0"), TEXT("ci-min"), TEXT("ci-gt0"), TEXT("mono-b"), TEXT("mono-0"), TEXT("mono-max"), TEXT("mono-lt1")};

	const TCHAR* const ColourGamutIds[] = {TEXT("cg-b"), TEXT("cg-srgb"), TEXT("cg-p3"), TEXT("cg-rec2020"), TEXT("cg-foo"), TEXT("cg-min"), TEXT("cg-range")};

	const FTopic AllTopics[] = {
		{RangeFormsIds, UE_ARRAY_COUNT(RangeFormsIds)},
		{MinMaxIds, UE_ARRAY_COUNT(MinMaxIds)},
		{TwoSidedIds, UE_ARRAY_COUNT(TwoSidedIds)},
		{QueryListsAndTypesIds, UE_ARRAY_COUNT(QueryListsAndTypesIds)},
		{NotAndOnlyIds, UE_ARRAY_COUNT(NotAndOnlyIds)},
		{FunctionTokensIds, UE_ARRAY_COUNT(FunctionTokensIds)},
		{Mq4GrammarIds, UE_ARRAY_COUNT(Mq4GrammarIds)},
		{CalcEmRemVwIds, UE_ARRAY_COUNT(CalcEmRemVwIds)},
		{ResolutionIds, UE_ARRAY_COUNT(ResolutionIds)},
		{RatiosIds, UE_ARRAY_COUNT(RatiosIds)},
		{BooleanFormsIds, UE_ARRAY_COUNT(BooleanFormsIds)},
		{InteractionIds, UE_ARRAY_COUNT(InteractionIds)},
		{PreferencesIds, UE_ARRAY_COUNT(PreferencesIds)},
		{DroppedInvertedColorsIds, UE_ARRAY_COUNT(DroppedInvertedColorsIds)},
		{ScriptingIds, UE_ARRAY_COUNT(ScriptingIds)},
		{ColourIds, UE_ARRAY_COUNT(ColourIds)},
		{ColourGamutIds, UE_ARRAY_COUNT(ColourGamutIds)},
	};

	const TCHAR* const FixturePath = TEXT("CssParity/Media/RangeTest.rml");

	FString SheetBinaryPath()
	{
		return FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/Media/RangeTest.rcssc"));
	}

	const FMediaCase* FindCase(const TCHAR* Id)
	{
		for (const FMediaCase& Case : Cases)
		{
			if (FCString::Strcmp(Case.Id, Id) == 0)
			{
				return &Case;
			}
		}
		return nullptr;
	}

	FString ExpectedColour(const FMediaCase& Case)
	{
		return Case.bMatches ? FString::Printf(TEXT("#%06xff"), Case.Rgb) : FString(TEXT("#000000ff"));
	}

	FString FindMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, const FTopic& Topic)
	{
		FString Failures;
		for (int32 Index = 0; Index < Topic.Count; ++Index)
		{
			const FMediaCase* Case = FindCase(Topic.Ids[Index]);
			if (!Case)
			{
				Failures += FString::Printf(TEXT("[%s has no case] "), Topic.Ids[Index]);
				continue;
			}
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Case->Id);
			if (!Element)
			{
				Failures += FString::Printf(TEXT("[%s is not in the layout] "), Case->Id);
				continue;
			}
			const FString Expected = ExpectedColour(*Case);
			if (!Element->BackgroundColor.Equals(Expected, ESearchCase::IgnoreCase))
			{
				Failures += FString::Printf(TEXT("[%s is %s, Chrome %s] "), Case->Id, *Element->BackgroundColor, *Expected);
			}
		}
		return Failures;
	}
}

TEST_CLASS(ArcRmlCssParityMediaTests, "ArcRmlUI.CssParity.Media")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndSettle(const FString& Path)
	{
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork;
	}

	bool OpenFromSource()
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(FixturePath);
		IFileManager::Get().Delete(*SheetBinaryPath(), false, true, true);
		FArcRmlTestView::DropCompiledDocument(Path);
		FArcRmlTestView::ClearDocumentCaches();
		return OpenAndSettle(Path) && IFileManager::Get().FileExists(*SheetBinaryPath());
	}

	bool ReopenFromCompiledSheet()
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(FixturePath);
		const FDateTime Compiled = IFileManager::Get().GetTimeStamp(*SheetBinaryPath());
		View.Close();
		FArcRmlTestView::ClearDocumentCaches();
		return OpenAndSettle(Path) && IFileManager::Get().GetTimeStamp(*SheetBinaryPath()) == Compiled;
	}

	FString CheckTopicOnBothPaths(const ArcRmlCssParityMediaTestsPrivate::FTopic& Topic)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Layout;
		if (!OpenFromSource() || !View.ParseLayout(Layout))
		{
			return TEXT("source open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		FString Failures = FindMismatches(Layout, Topic);
		if (!Failures.IsEmpty())
		{
			return TEXT("from source: ") + Failures;
		}
		Layout.Reset();
		if (!ReopenFromCompiledSheet() || !View.ParseLayout(Layout))
		{
			return TEXT("compiled sheet open failed: ") + ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		Failures = FindMismatches(Layout, Topic);
		return Failures.IsEmpty() ? FString() : TEXT("from compiled sheet: ") + Failures;
	}

	TEST_METHOD(RangeComparisons_MatchTheViewportAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {RangeFormsIds, UE_ARRAY_COUNT(RangeFormsIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(MinAndMaxPrefixes_MatchTheViewportAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {MinMaxIds, UE_ARRAY_COUNT(MinMaxIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(TwoSidedRanges_MatchOnlyWhenBothBoundsHold)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {TwoSidedIds, UE_ARRAY_COUNT(TwoSidedIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(QueryListsAndMediaTypes_MatchAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {QueryListsAndTypesIds, UE_ARRAY_COUNT(QueryListsAndTypesIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(NotAndOnly_NegateAndPassAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {NotAndOnlyIds, UE_ARRAY_COUNT(NotAndOnlyIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(FunctionTokensWithoutASpace_AreNotMatchedAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {FunctionTokensIds, UE_ARRAY_COUNT(FunctionTokensIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(Mq4ConditionGrammar_UsesThreeValuedLogicAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {Mq4GrammarIds, UE_ARRAY_COUNT(Mq4GrammarIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CalcEmRemAndViewportLengths_ResolveAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {CalcEmRemVwIds, UE_ARRAY_COUNT(CalcEmRemVwIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(ResolutionUnits_ConvertBetweenDppxDpiAndDpcmAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {ResolutionIds, UE_ARRAY_COUNT(ResolutionIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(AspectRatioValues_ParseStrictlyAsChromeDoes)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {RatiosIds, UE_ARRAY_COUNT(RatiosIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(BooleanFeatureForms_MatchWhenTheFeatureIsNonZeroOrNotNone)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {BooleanFormsIds, UE_ARRAY_COUNT(BooleanFormsIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(InteractionAndUpdateFeatures_ReportADesktopPointerDevice)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {InteractionIds, UE_ARRAY_COUNT(InteractionIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(PreferenceFeatures_ReportNoPreferencesInTheLightScheme)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {PreferencesIds, UE_ARRAY_COUNT(PreferencesIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(InvertedColors_IsAnUnknownFeatureAsInChrome)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {DroppedInvertedColorsIds, UE_ARRAY_COUNT(DroppedInvertedColorsIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(Scripting_OnlyNoneMatches)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {ScriptingIds, UE_ARRAY_COUNT(ScriptingIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(ColourFeatures_ReportEightBitsAndNoMonochrome)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {ColourIds, UE_ARRAY_COUNT(ColourIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(ColourGamutFeature_MatchesSrgbOnly)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		const FTopic Topic = {ColourGamutIds, UE_ARRAY_COUNT(ColourGamutIds)};
		const FString Failures = CheckTopicOnBothPaths(Topic);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(InvalidQueries_ReportInvalidAtRuleDiagnosticsWithTheirLines)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFromSource(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlDiagnostic>& Diagnostics = View.GetViewDiagnostics().Diagnostics;
		ASSERT_THAT(AreEqual(UE_ARRAY_COUNT(InvalidQueries), Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		for (const FInvalidQuery& Invalid : InvalidQueries)
		{
			int32 Found = 0;
			for (const FArcRmlDiagnostic& Diagnostic : Diagnostics)
			{
				if (Diagnostic.Line == Invalid.Line && Diagnostic.Kind == EArcRmlDiagnosticKind::InvalidAtRule && Diagnostic.File.EndsWith(TEXT("RangeTest.rcss"))
					&& Diagnostic.Message.Contains(FString::Printf(TEXT("@media query '%s'"), Invalid.Query)))
				{
					++Found;
				}
			}
			ASSERT_THAT(AreEqual(1, Found, FString::Printf(TEXT("line %d, query '%s': %s"), Invalid.Line, Invalid.Query, *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		}
	}

	TEST_METHOD(EveryFixtureBlockBelongsToATopic)
	{
		using namespace ArcRmlCssParityMediaTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFromSource(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(View.ParseLayout(Layout)));
		int32 Rows = 0;
		for (const FArcRmlTestLayoutElement& Element : Layout)
		{
			if (Element.Classes == TEXT("row"))
			{
				++Rows;
			}
		}
		ASSERT_THAT(AreEqual(static_cast<int32>(UE_ARRAY_COUNT(Cases)), Rows));
		for (const FMediaCase& Case : Cases)
		{
			int32 Topics = 0;
			for (const FTopic& Topic : AllTopics)
			{
				for (int32 Index = 0; Index < Topic.Count; ++Index)
				{
					if (FCString::Strcmp(Topic.Ids[Index], Case.Id) == 0)
					{
						++Topics;
						break;
					}
				}
			}
			ASSERT_THAT(IsTrue(Topics > 0, FString::Printf(TEXT("%s is in no topic"), Case.Id)));
			ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id) != nullptr, FString::Printf(TEXT("%s is not in the layout"), Case.Id)));
		}
	}
};

