// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlLayoutTestsPrivate
{
	constexpr float Tolerance = 0.5f;

	FString DescribeBox(const FArcRmlTestLayoutElement* Element)
	{
		if (Element == nullptr)
		{
			return TEXT("missing");
		}
		return FString::Printf(TEXT("%s x %.2f y %.2f w %.2f h %.2f"), *Element->DomId, Element->Rect.Position.X, Element->Rect.Position.Y,
			Element->Rect.Size.X, Element->Rect.Size.Y);
	}
}

TEST_CLASS(ArcRmlLayoutTests, "ArcRmlUI.Layout")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenLayout(const TCHAR* RelativePath)
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) && View.Flush() && View.Flush()
			&& View.GetProbe().bHasDocument && View.ParseLayout(Elements);
	}

	const FArcRmlTestLayoutElement* Find(const TCHAR* DomId)
	{
		return ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
	}

	void ExpectX(const TCHAR* DomId, float Expected, const TCHAR* Reason)
	{
		const FArcRmlTestLayoutElement* Element = Find(DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(IsNear(Expected, Element->Rect.Position.X, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("%s: %s"), Reason, *ArcRmlLayoutTestsPrivate::DescribeBox(Element))));
	}

	void ExpectY(const TCHAR* DomId, float Expected, const TCHAR* Reason)
	{
		const FArcRmlTestLayoutElement* Element = Find(DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(IsNear(Expected, Element->Rect.Position.Y, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("%s: %s"), Reason, *ArcRmlLayoutTestsPrivate::DescribeBox(Element))));
	}

	void ExpectWidth(const TCHAR* DomId, float Expected, const TCHAR* Reason)
	{
		const FArcRmlTestLayoutElement* Element = Find(DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(IsNear(Expected, Element->Rect.Size.X, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("%s: %s"), Reason, *ArcRmlLayoutTestsPrivate::DescribeBox(Element))));
	}

	void ExpectSize(const TCHAR* DomId, float ExpectedWidth, float ExpectedHeight, const TCHAR* Reason)
	{
		const FArcRmlTestLayoutElement* Element = Find(DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		const FString Context = FString::Printf(TEXT("%s: %s"), Reason, *ArcRmlLayoutTestsPrivate::DescribeBox(Element));
		ASSERT_THAT(IsNear(ExpectedWidth, Element->Rect.Size.X, ArcRmlLayoutTestsPrivate::Tolerance, Context));
		ASSERT_THAT(IsNear(ExpectedHeight, Element->Rect.Size.Y, ArcRmlLayoutTestsPrivate::Tolerance, Context));
	}

	TEST_METHOD(GridItemWithCssWideMinWidth_KeepsTheAutomaticMinimum)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/GridMinWidthTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("g-auto-a"), 84.5f, TEXT("an item without min-width takes the automatic minimum (Chrome)"));
		ExpectWidth(TEXT("g-auto-b"), 115.5f, TEXT("an item without min-width takes the automatic minimum (Chrome)"));
		ExpectWidth(TEXT("g-initial-a"), 84.5f, TEXT("min-width: initial is auto"));
		ExpectWidth(TEXT("g-initial-b"), 115.5f, TEXT("min-width: initial is auto"));
		ExpectWidth(TEXT("g-inline-a"), 84.5f, TEXT("an inline min-width: initial is auto"));
		ExpectWidth(TEXT("g-inline-b"), 115.5f, TEXT("an inline min-width: initial is auto"));
		ExpectWidth(TEXT("g-inherit-a"), 84.5f, TEXT("min-width: inherit from a container without min-width is auto"));
		ExpectWidth(TEXT("g-inherit-b"), 115.5f, TEXT("min-width: inherit from a container without min-width is auto"));
		ExpectWidth(TEXT("g-zero-a"), 100.0f, TEXT("a stylesheet min-width: 0 removes the automatic minimum"));
		ExpectWidth(TEXT("g-zero-b"), 100.0f, TEXT("a stylesheet min-width: 0 removes the automatic minimum"));
		ExpectWidth(TEXT("g-inherit-px-a"), 100.0f, TEXT("min-width: inherit of 30px is a definite minimum"));
		ExpectWidth(TEXT("g-inherit-px-b"), 100.0f, TEXT("min-width: inherit of 30px is a definite minimum"));
	}

	TEST_METHOD(OverConstrainedHorizontalAutoMargins_SetTheLeftMarginToZero)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/AutoMarginTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectX(TEXT("abs-both"), 100.0f, TEXT("an absolute box with both margins auto and no room keeps margin-left 0 (CSS 2.1 10.3.7)"));
		ExpectX(TEXT("flow-both"), 300.0f, TEXT("an in-flow box wider than its containing block treats auto margins as 0 (CSS 2.1 10.3.3)"));
		ExpectX(TEXT("flow-left"), 300.0f, TEXT("an in-flow box wider than its containing block treats a lone auto margin-left as 0"));
		ExpectX(TEXT("flow-min"), 300.0f, TEXT("a min-width clamp that overflows the containing block keeps margin-left 0"));
		ExpectX(TEXT("abs-left"), 36.0f, TEXT("an absolute box with one auto margin solves for it, negative included"));
		ExpectX(TEXT("abs-right"), 104.0f, TEXT("an absolute box with an auto margin-right keeps its margin-left"));
		ExpectY(TEXT("abs-vert"), 210.0f, TEXT("vertical auto margins of an absolute box stay equal when negative (CSS 2.1 10.6.4)"));
		ExpectX(TEXT("flow-right"), 304.0f, TEXT("an in-flow box with an auto margin-right keeps its margin-left"));
		ExpectX(TEXT("flow-fit"), 320.0f, TEXT("auto margins with room left centre the box"));
		ExpectWidth(TEXT("stf-max"), 60.0f, TEXT("a shrink-to-fit box around a centred block is the block's width"));
		ExpectX(TEXT("stf-max-child"), 500.0f, TEXT("a centred block in its shrink-to-fit parent has no offset"));
		ExpectWidth(TEXT("stf-min"), 60.0f, TEXT("auto margins contribute nothing to the min-content width"));
		ExpectX(TEXT("stf-min-child"), 100.0f, TEXT("a centred block in a min-content parent has no offset"));
		ExpectX(TEXT("float-both"), 300.0f, TEXT("auto margins of a float are 0 (CSS 2.1 10.3.5)"));
		ExpectX(TEXT("ib-both"), 300.0f, TEXT("auto margins of an inline-block are 0 (CSS 2.1 10.3.9)"));
	}

	TEST_METHOD(FlexMinContent_HonoursTheFlexBasis)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FlexBasisTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("fx-cap"), 40.0f, TEXT("flex: 0 1 40px caps a 50px min-content contribution at 40"));
		ExpectWidth(TEXT("fx-raise"), 60.0f, TEXT("flex: 0 0 60px raises a 50px min-content contribution to 60"));
		ExpectWidth(TEXT("fx-cap-mix"), 50.0f, TEXT("a nowrap row sums the 40px capped item and a 10px min-content item"));
		ExpectWidth(TEXT("fx-raise-mix"), 70.0f, TEXT("a nowrap row sums the 60px raised item and a 10px min-content item"));
		ExpectWidth(TEXT("fx-border"), 40.0f, TEXT("a border-box flex-basis caps the outer contribution at 40"));
		ExpectWidth(TEXT("fx-wrap"), 50.0f, TEXT("a wrapping row takes the largest plain min-content contribution"));
		ExpectWidth(TEXT("fx-wrap-cap"), 50.0f, TEXT("a wrapping row does not cap a min-content contribution at the flex basis"));
		ExpectWidth(TEXT("fx-grow"), 50.0f, TEXT("a growable, shrinkable item keeps its min-content contribution"));
		ExpectWidth(TEXT("fx-free"), 50.0f, TEXT("a growable item that cannot shrink below 30px keeps its 50px contribution"));
		ExpectWidth(TEXT("fx-sum"), 100.0f, TEXT("a nowrap row sums the capped and raised contributions"));
		ExpectWidth(TEXT("fx-column"), 50.0f, TEXT("a column container's width ignores the main-axis flex-basis"));
	}

	TEST_METHOD(FlexIntrinsicWidth_KeepsTheAutomaticMinimumOfAnItemCappedAtItsFlexBasis)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FlexAutoMinTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("fx-cap"), 50.0f, TEXT("flex: 0 1 40px with min-width auto contributes its 50px min-content (Chrome)"));
		ExpectWidth(TEXT("it-cap"), 50.0f, TEXT("the item keeps its automatic minimum"));
		ExpectWidth(TEXT("fx-border"), 60.0f, TEXT("a border-box item contributes 50px content plus 8px padding and 2px border (Chrome)"));
		ExpectWidth(TEXT("fx-cap-mix"), 60.0f, TEXT("a nowrap row sums the 50px automatic minimum and a 10px min-content item (Chrome)"));
		ExpectWidth(TEXT("fx-sum"), 110.0f, TEXT("a nowrap row sums the automatic minimum and the raised 60px item (Chrome)"));
		ExpectWidth(TEXT("fx-hidden"), 40.0f, TEXT("an overflow: hidden item has no automatic minimum, so the flex basis caps it (Chrome)"));
		ExpectWidth(TEXT("fx-max"), 45.0f, TEXT("max-width: 45px limits the automatic minimum (Chrome)"));
		ExpectWidth(TEXT("fx-wide"), 50.0f, TEXT("the max-content width keeps the automatic minimum too (Chrome)"));
		ExpectSize(TEXT("fx-wide-wrap"), 110.0f, 20.0f, TEXT("a wrapping row's max-content width keeps the automatic minimum, so the items share one line (Chrome)"));
		ExpectX(TEXT("it-wide-wrap-b"), 650.0f, TEXT("the second item sits after the 50px first item on the same line (Chrome)"));
		ExpectWidth(TEXT("tw-menu"), 190.09f, TEXT("an absolute basis-10 menu holds its two labels, gap and padding (Chrome)"));
		ExpectWidth(TEXT("tw-row"), 172.44f, TEXT("an inline-flex basis-16 button row holds its labels (Chrome)"));
	}

	TEST_METHOD(AspectRatioItemStretchedToTheContainer_SizesTheShrinkToFitContainer)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FlexAspectTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("fa-stretch"), 200.0f, TEXT("a stretched aspect-ratio item contributes its ratio width"));
		ExpectSize(TEXT("fa-stretch-item"), 200.0f, 100.0f, TEXT("a stretched aspect-ratio item takes the container height"));
		ExpectWidth(TEXT("fa-two"), 250.0f, TEXT("two stretched aspect-ratio items contribute their ratio widths"));
		ExpectWidth(TEXT("fa-padded"), 200.0f, TEXT("the stretched content height excludes the item's edges"));
		ExpectSize(TEXT("fa-padded-item"), 188.0f, 100.0f, TEXT("the stretched content height excludes the item's edges"));
		ExpectWidth(TEXT("fa-inline"), 100.0f, TEXT("an inline-flex container sizes from the stretched aspect-ratio item"));
		ExpectWidth(TEXT("fa-min"), 200.0f, TEXT("the min-content width of a single-line row includes the stretched ratio width"));
		ExpectSize(TEXT("fa-min-item"), 200.0f, 100.0f, TEXT("the min-content width of a single-line row includes the stretched ratio width"));
		ExpectSize(TEXT("fa-start-item"), 0.0f, 0.0f, TEXT("an aspect-ratio item that is not stretched has no definite size"));
		ExpectSize(TEXT("fa-wrap-item"), 0.0f, 100.0f, TEXT("a multi-line container does not stretch to a definite cross size first"));
	}

	TEST_METHOD(GridItemMinWidthFromVar_KeepsTheAutomaticMinimum)
	{
		TestRunner->AddExpectedErrorPlain(TEXT("variable '--nope' not defined"), EAutomationExpectedErrorFlags::Contains, 0);
		TestRunner->AddExpectedErrorPlain(TEXT("variable '--mw' not defined"), EAutomationExpectedErrorFlags::Contains, 0);
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/GridVarTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("v-initial-a"), 84.5f, TEXT("a custom property set to initial makes min-width auto"));
		ExpectWidth(TEXT("v-initial-b"), 115.5f, TEXT("a custom property set to initial makes min-width auto"));
		ExpectWidth(TEXT("v-fb-initial-a"), 84.5f, TEXT("a var() fallback of initial makes min-width auto"));
		ExpectWidth(TEXT("v-fb-initial-b"), 115.5f, TEXT("a var() fallback of initial makes min-width auto"));
		ExpectWidth(TEXT("v-fb-inherit-a"), 84.5f, TEXT("a var() fallback of inherit from a container without min-width is auto"));
		ExpectWidth(TEXT("v-fb-inherit-b"), 115.5f, TEXT("a var() fallback of inherit from a container without min-width is auto"));
		ExpectWidth(TEXT("v-cinherit-a"), 84.5f, TEXT("a custom property inherited from nowhere makes min-width auto"));
		ExpectWidth(TEXT("v-cinherit-b"), 115.5f, TEXT("a custom property inherited from nowhere makes min-width auto"));
		ExpectWidth(TEXT("v-zero-a"), 100.0f, TEXT("a var() of 0px is a definite minimum"));
		ExpectWidth(TEXT("v-zero-b"), 100.0f, TEXT("a var() of 0px is a definite minimum"));
		ExpectWidth(TEXT("v-undef-a"), 84.5f, TEXT("an unresolved var() makes min-width unset, so auto"));
		ExpectWidth(TEXT("v-undef-b"), 115.5f, TEXT("an unresolved var() makes min-width unset, so auto"));
	}

	TEST_METHOD(AspectRatioGridItemWithNormalAlignment_TakesTheStretchedRowThroughTheRatioAsItsMinimum)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/GridRatioTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		struct FCase
		{
			const TCHAR* DomId;
			float Width;
			float Height;
		};
		const FCase Cases[] = {
			{TEXT("a-auto"), 240.0f, 120.0f},
			{TEXT("a-start"), 150.0f, 75.0f},
			{TEXT("a-zero"), 150.0f, 75.0f},
			{TEXT("col-auto"), 240.0f, 120.0f},
			{TEXT("col-fixed"), 100.0f, 50.0f},
			{TEXT("margin10"), 220.0f, 110.0f},
			{TEXT("margin-left10"), 240.0f, 120.0f},
			{TEXT("padded"), 226.0f, 120.0f},
			{TEXT("border-box"), 240.0f, 120.0f},
			{TEXT("content"), 240.0f, 120.0f},
			{TEXT("span2"), 240.0f, 120.0f},
			{TEXT("row-minmax"), 240.0f, 120.0f},
			{TEXT("stretch-zero"), 240.0f, 120.0f},
			{TEXT("min-h130"), 260.0f, 130.0f},
			{TEXT("min-h50"), 240.0f, 120.0f},
			{TEXT("max-h100"), 200.0f, 100.0f},
			{TEXT("max-w200"), 200.0f, 100.0f},
			{TEXT("s-margin10"), 220.0f, 110.0f},
			{TEXT("s-padded"), 230.0f, 120.0f},
			{TEXT("s-max-h100"), 200.0f, 100.0f},
			{TEXT("center"), 150.0f, 75.0f},
			{TEXT("items-start"), 150.0f, 75.0f},
		};
		for (const FCase& Case : Cases)
		{
			ExpectSize(Case.DomId, Case.Width, Case.Height, TEXT("Chrome size of the aspect-ratio grid item"));
		}
	}

	TEST_METHOD(AspectRatioItemInAFlexBoxSizedByInsetsOrPercent_SizesTheShrinkToFitContainer)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FlexAspectTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("fa-inset"), 200.0f, TEXT("top: 0; bottom: 100px in a 200px containing block gives a definite 100px height"));
		ExpectSize(TEXT("fa-inset-item"), 200.0f, 100.0f, TEXT("the item stretches to the inset height"));
		ExpectWidth(TEXT("fa-pct"), 200.0f, TEXT("height: 50% of a 200px containing block is a definite 100px"));
		ExpectSize(TEXT("fa-pct-item"), 200.0f, 100.0f, TEXT("the item stretches to the percentage height"));
		ExpectWidth(TEXT("fa-inset-min"), 110.0f, TEXT("the min-content width of an inset-sized row includes the stretched ratio width"));
	}

	TEST_METHOD(ElementWithoutFontFamily_UsesTheFallbackFont)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FontFallbackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Wrap = Find(TEXT("ib-wrap"));
		ASSERT_THAT(IsNotNull(Wrap));
		ASSERT_THAT(IsNear(24.0f, Wrap->Rect.Size.Y, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("the line box of a 20px inline-block takes the fallback font's descent: %s"), *ArcRmlLayoutTestsPrivate::DescribeBox(Wrap))));
		const FArcRmlTestLayoutElement* Wrap16 = Find(TEXT("ib-wrap16"));
		ASSERT_THAT(IsNotNull(Wrap16));
		ASSERT_THAT(IsNear(25.0f, Wrap16->Rect.Size.Y, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("the line box at 16px is 20 + round(4.672) = 25 with the fallback family Space Grotesk (Chrome 25): %s"), *ArcRmlLayoutTestsPrivate::DescribeBox(Wrap16))));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("text-wrap"));
		ASSERT_THAT(AreEqual(1, Lines.Num(), TEXT("text without a font-family is laid out in the fallback font")));
		ASSERT_THAT(IsTrue(Lines[0].Width > 0, TEXT("text without a font-family has a width")));
	}

	TEST_METHOD(AbsoluteBoxWithAnAutoInset_KeepsAutoMarginsAtZero)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/AutoMarginTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectX(TEXT("abs-left-inset-auto"), 840.0f, TEXT("left: auto with right: 0 puts the box at the right edge (CSS 2.1 10.3.7)"));
		ExpectX(TEXT("abs-right-inset-auto"), 700.0f, TEXT("right: auto with left: 0 keeps margin-left 0 (CSS 2.1 10.3.7)"));
		ExpectX(TEXT("abs-static"), 700.0f, TEXT("both insets auto keep the static position with margins 0 (CSS 2.1 10.3.7)"));
	}

	void ExpectCentredFill(const TCHAR* BarId, const TCHAR* Reason)
	{
		const FArcRmlTestLayoutElement* Bar = Find(BarId);
		ASSERT_THAT(IsNotNull(Bar, BarId));
		const FArcRmlTestLayoutElement* Fill = Elements.FindByPredicate([Bar](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Parent == Bar->Path && Element.Tag == TEXT("fill");
		});
		ASSERT_THAT(IsNotNull(Fill, TEXT("the progress bar has no fill element")));
		ASSERT_THAT(IsNear(70.0f, Fill->Rect.Position.X, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("%s: %s"), Reason, *ArcRmlLayoutTestsPrivate::DescribeBox(Fill))));
		ASSERT_THAT(IsNear(100.0f, Fill->Rect.Size.X, ArcRmlLayoutTestsPrivate::Tolerance, ArcRmlLayoutTestsPrivate::DescribeBox(Fill)));
	}

	TEST_METHOD(WidgetPartBuiltAsABlock_CentresWithAutoMargins)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/WidgetMarginTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectCentredFill(TEXT("bar"), TEXT("a 100px fill with display inline is built as a block, so its auto margins centre it in the 200px bar"));
		ExpectCentredFill(TEXT("bar-ib"), TEXT("a 100px fill with display inline-block is built as a block too, so its auto margins centre it"));
	}

	TEST_METHOD(RootHtmlWithInlineBlockDisplay_CentresWithAutoMargins)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/BodyInlineBlockTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectX(TEXT("root-ib"), 490.0f, TEXT("the html root is laid out as a block whatever its display, so auto margins centre it in the 1280px view"));
	}

	TEST_METHOD(InlineAndFloatedTables_KeepAutoMarginsAtZeroAfterTheTableIsSized)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/TableRoleTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectX(TEXT("t-inline"), 0.0f, TEXT("an inline-table resolves margin: auto to 0 also when the table resizes its box (Chrome 0)"));
		ExpectWidth(TEXT("t-inline"), 100.0f, TEXT("the inline-table shrinks to its cell (Chrome 100)"));
		ExpectX(TEXT("t-float"), 0.0f, TEXT("a floated table resolves margin: auto to 0 also when the table resizes its box (Chrome 0)"));
		ExpectX(TEXT("t-block"), 250.0f, TEXT("a block-level table is centred by margin: auto in its 600px wrapper (Chrome 250)"));
	}

	TEST_METHOD(RootBodyWithTableDisplay_CentresWithAutoMarginsAfterTheTableIsSized)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/BodyTableTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectX(TEXT("root-table"), 490.0f, TEXT("a body with display: table keeps its auto margins centring it in the 1280px html (Chrome 490)"));
		ExpectWidth(TEXT("root-table"), 300.0f, TEXT("the root table shrinks to its 300px cell (Chrome 300)"));
	}

#if WITH_EDITOR
	TEST_METHOD(FontFileReload_GivesFallbackAndNamedTextTheirFacesAgain)
	{
		ASSERT_THAT(IsTrue(OpenLayout(TEXT("Layout/FontFallbackTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlTestTextLine> FallbackBefore = ArcRmlUITestHelpers::GetLines(Elements, TEXT("text-wrap"));
		const TArray<FArcRmlTestTextLine> NamedBefore = ArcRmlUITestHelpers::GetLines(Elements, TEXT("named-wrap"));
		ASSERT_THAT(AreEqual(1, FallbackBefore.Num()));
		ASSERT_THAT(AreEqual(1, NamedBefore.Num()));

		FArcRmlTestView::ReloadFonts({TEXT("/ArcRmlUITest/UI/Rml/Fonts/SpaceGrotesk-Regular.ttf"), TEXT("/ArcRmlUITest/UI/Rml/Fonts/BarlowCondensed-Medium.ttf")});
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));

		const TArray<FArcRmlTestTextLine> FallbackAfter = ArcRmlUITestHelpers::GetLines(Elements, TEXT("text-wrap"));
		const TArray<FArcRmlTestTextLine> NamedAfter = ArcRmlUITestHelpers::GetLines(Elements, TEXT("named-wrap"));
		ASSERT_THAT(AreEqual(1, FallbackAfter.Num(), TEXT("text without a font-family is laid out again in the reloaded fallback face")));
		ASSERT_THAT(AreEqual(1, NamedAfter.Num(), TEXT("text in a named family is laid out again in its reloaded face")));
		ASSERT_THAT(IsTrue(FallbackBefore[0].Width > 0 && FallbackAfter[0].Width == FallbackBefore[0].Width, TEXT("the fallback text keeps its width after the reload")));
		ASSERT_THAT(IsTrue(NamedBefore[0].Width > 0 && NamedAfter[0].Width == NamedBefore[0].Width, TEXT("the named-family text keeps its width after the reload")));
		const FArcRmlTestLayoutElement* Wrap = Find(TEXT("ib-wrap"));
		ASSERT_THAT(IsNotNull(Wrap));
		ASSERT_THAT(IsNear(24.0f, Wrap->Rect.Size.Y, ArcRmlLayoutTestsPrivate::Tolerance,
			FString::Printf(TEXT("the line box takes the reloaded fallback face's descent again: %s"), *ArcRmlLayoutTestsPrivate::DescribeBox(Wrap))));
	}
#endif
};
