// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityMarginCollapseTestsPrivate
{
	struct FBoxCase
	{
		const TCHAR* Id;
		float X;
		float Y;
		float Width;
		float Height;
	};

	const TCHAR* const HtmlPath = TEXT("0");
	constexpr float Tolerance = 1.0f;

	const FBoxCase ChromeHeadingInBody[] = {
		{TEXT("mc-body"), 8.0f, 21.4375f, 1264.0f, 539.4375f},
		{TEXT("mc-h1"), 8.0f, 21.4375f, 1264.0f, 40.0f},
	};

	const FBoxCase ChromeFirstParagraphAndList[] = {
		{TEXT("mc-p-parent"), 8.0f, 82.875f, 1264.0f, 21.0f},
		{TEXT("mc-p"), 8.0f, 82.875f, 1264.0f, 21.0f},
		{TEXT("mc-ul-parent"), 8.0f, 119.875f, 1264.0f, 21.0f},
		{TEXT("mc-ul"), 8.0f, 119.875f, 1264.0f, 21.0f},
		{TEXT("mc-li"), 48.0f, 119.875f, 1224.0f, 21.0f},
	};

	const FBoxCase ChromeBorderAndPaddingOnTop[] = {
		{TEXT("mc-border-parent"), 8.0f, 156.875f, 1264.0f, 39.0f},
		{TEXT("mc-border-p"), 8.0f, 174.875f, 1264.0f, 21.0f},
		{TEXT("mc-padding-parent"), 8.0f, 211.875f, 1264.0f, 41.0f},
		{TEXT("mc-padding-p"), 8.0f, 231.875f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeFlowRoot[] = {
		{TEXT("mc-flow-root"), 8.0f, 268.875f, 1264.0f, 53.0f},
		{TEXT("mc-flow-root-p"), 8.0f, 284.875f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeNestedChain[] = {
		{TEXT("mc-outer"), 8.0f, 351.875f, 1264.0f, 21.0f},
		{TEXT("mc-middle"), 8.0f, 351.875f, 1264.0f, 21.0f},
		{TEXT("mc-inner-p"), 8.0f, 351.875f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeLastChild[] = {
		{TEXT("mc-last-parent"), 8.0f, 402.875f, 1264.0f, 58.0f},
		{TEXT("mc-last-p"), 8.0f, 439.875f, 1264.0f, 21.0f},
		{TEXT("mc-bottom-parent"), 8.0f, 484.875f, 1264.0f, 39.0f},
		{TEXT("mc-bottom-p"), 8.0f, 484.875f, 1264.0f, 21.0f},
		{TEXT("mc-tail"), 8.0f, 539.875f, 1264.0f, 21.0f},
	};

	constexpr float ChromeHtmlHeight = 576.875f;

	const FBoxCase ChromeParagraphFirstListLast[] = {
		{TEXT("mcp-body"), 8.0f, 16.0f, 1264.0f, 58.0f},
		{TEXT("mcp-p"), 8.0f, 16.0f, 1264.0f, 21.0f},
		{TEXT("mcp-ul"), 8.0f, 53.0f, 1264.0f, 21.0f},
		{TEXT("mcp-li"), 48.0f, 53.0f, 1224.0f, 21.0f},
	};

	constexpr float ChromeParagraphFirstHtmlHeight = 90.0f;

	const FBoxCase ChromeNegativeMargins[] = {
		{TEXT("mcc-mixed-parent"), 8.0f, 48.0f, 1264.0f, 21.0f},
		{TEXT("mcc-mixed-p"), 8.0f, 48.0f, 1264.0f, 21.0f},
		{TEXT("mcc-neg-parent"), 8.0f, 103.0f, 1264.0f, 21.0f},
		{TEXT("mcc-neg-p"), 8.0f, 103.0f, 1264.0f, 21.0f},
		{TEXT("mcc-negbottom-parent"), 8.0f, 140.0f, 1264.0f, 21.0f},
		{TEXT("mcc-negbottom-p"), 8.0f, 140.0f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeOverflowHidden[] = {
		{TEXT("mcc-overflow"), 8.0f, 203.0f, 1264.0f, 53.0f},
		{TEXT("mcc-overflow-p"), 8.0f, 219.0f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeFixedHeight[] = {
		{TEXT("mcc-height"), 8.0f, 272.0f, 1264.0f, 60.0f},
		{TEXT("mcc-height-p"), 8.0f, 272.0f, 1264.0f, 21.0f},
		{TEXT("mcc-padbottom"), 8.0f, 348.0f, 1264.0f, 41.0f},
	};

	const FBoxCase ChromeBottomPadding[] = {
		{TEXT("mcc-padbottom"), 8.0f, 348.0f, 1264.0f, 41.0f},
		{TEXT("mcc-padbottom-p"), 8.0f, 348.0f, 1264.0f, 21.0f},
		{TEXT("mcc-flex-parent"), 8.0f, 413.0f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeFlexFirstChild[] = {
		{TEXT("mcc-flex-parent"), 8.0f, 413.0f, 1264.0f, 21.0f},
		{TEXT("mcc-flex"), 8.0f, 413.0f, 1264.0f, 21.0f},
	};

	const FBoxCase ChromeFlexItem[] = {
		{TEXT("mcc-flexrow"), 8.0f, 434.0f, 1264.0f, 53.0f},
		{TEXT("mcc-item"), 8.0f, 434.0f, 69.3281f, 53.0f},
		{TEXT("mcc-item-p"), 8.0f, 450.0f, 69.3281f, 21.0f},
		{TEXT("mcc-end"), 8.0f, 487.0f, 1264.0f, 30.0f},
		{TEXT("mcc-body"), 8.0f, 8.0f, 1264.0f, 509.0f},
	};

	FString DescribeRect(const FArcRmlTestLayoutElement* Element, const FBoxCase& Case)
	{
		if (!Element)
		{
			return FString::Printf(TEXT("[%s is not in the layout] "), Case.Id);
		}
		const FArcRmlInspectorRect& Rect = Element->Rect;
		if (FMath::Abs(Rect.Position.X - Case.X) > Tolerance || FMath::Abs(Rect.Position.Y - Case.Y) > Tolerance
			|| FMath::Abs(Rect.Size.X - Case.Width) > Tolerance || FMath::Abs(Rect.Size.Y - Case.Height) > Tolerance)
		{
			return FString::Printf(TEXT("[%s is %.2f,%.2f %.2fx%.2f, Chrome %.2f,%.2f %.2fx%.2f] "), Case.Id, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
				Rect.Size.Y, Case.X, Case.Y, Case.Width, Case.Height);
		}
		return FString();
	}

	template <int32 N>
	FString DescribeMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, const FBoxCase (&Cases)[N])
	{
		FString Mismatches;
		for (const FBoxCase& Case : Cases)
		{
			Mismatches += DescribeRect(ArcRmlUITestHelpers::FindByDomId(Layout, Case.Id), Case);
		}
		return Mismatches;
	}

	FString DescribeHtml(const TArray<FArcRmlTestLayoutElement>& Layout, float ChromeHeight)
	{
		return DescribeRect(ArcRmlUITestHelpers::FindByPath(Layout, HtmlPath), {TEXT("html"), 0.0f, 0.0f, 1280.0f, ChromeHeight});
	}
}

TEST_CLASS(ArcRmlCssParityMarginCollapseTests, "ArcRmlUI.CssParity.MarginCollapse")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* Relative)
	{
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(Relative)) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout);
	}

	TEST_METHOD(FirstChildHeading_CollapsesItsTopMarginThroughBodyAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeHeadingInBody)));
	}

	TEST_METHOD(FirstChildParagraphAndList_CollapseTheirTopMarginsThroughTheParentAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeFirstParagraphAndList)));
	}

	TEST_METHOD(ParentWithTopBorderOrPadding_KeepsTheChildTopMarginInsideAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeBorderAndPaddingOnTop)));
	}

	TEST_METHOD(FlowRootParent_KeepsTheChildMarginsInsideAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeFlowRoot)));
	}

	TEST_METHOD(NestedBlocks_CollapseEveryTopAndBottomMarginIntoOneAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeNestedChain)));
	}

	TEST_METHOD(LastChildParagraph_CollapsesItsBottomMarginThroughTheParentUnlessABorderSeparatesThemAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeLastChild)));
		ASSERT_THAT(AreEqual(FString(), DescribeHtml(Layout, ChromeHtmlHeight), TEXT("the html root does not hold body's collapsed margins as Chrome")));
	}

	TEST_METHOD(BodyWithAFirstParagraphAndALastList_CollapsesBothEndsButNotThroughHtmlAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapsePFirstTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeParagraphFirstListLast)));
		ASSERT_THAT(AreEqual(FString(), DescribeHtml(Layout, ChromeParagraphFirstHtmlHeight), TEXT("the html root does not hold body's collapsed margins as Chrome")));
	}

	TEST_METHOD(NegativeAndMixedSignMargins_CollapseThroughTheParentAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeNegativeMargins)));
	}

	TEST_METHOD(OverflowHiddenParent_KeepsTheChildMarginsInsideAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeOverflowHidden)));
	}

	TEST_METHOD(FixedHeightParent_CollapsesItsTopButNotItsBottomAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeFixedHeight)));
	}

	TEST_METHOD(ParentWithBottomPadding_KeepsTheLastChildBottomMarginInsideAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeBottomPadding)));
	}

	TEST_METHOD(FlexContainerFirstChild_CollapsesItsOwnMarginThroughTheParentAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeFlexFirstChild)));
	}

	TEST_METHOD(FlexItem_KeepsItsFirstChildMarginInsideAsChrome)
	{
		using namespace ArcRmlCssParityMarginCollapseTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/MarginCollapse/MarginCollapseCasesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(Layout, ChromeFlexItem)));
	}
};
