// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityLayoutTestsPrivate
{
	struct FChromeBox
	{
		const TCHAR* DomId;
		float X;
		float Y;
		float Width;
		float Height;
	};

	struct FChromeOrigin
	{
		const TCHAR* DomId;
		float X;
		float Y;
	};

	int32 FindDraw(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (Draws[Index].Op == EArcRmlTestDrawOp::Geometry && !Draws[Index].bCallbackTexture && Draws[Index].FirstColourPremultiplied == Colour)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool IsNear(float A, float B, float Tolerance = 0.5f)
	{
		return FMath::IsNearlyEqual(A, B, Tolerance);
	}

	FString Describe(const FArcRmlTestLayoutElement& Element)
	{
		return FString::Printf(TEXT("%s is %g,%g,%g,%g"), *Element.DomId, Element.Rect.Position.X, Element.Rect.Position.Y, Element.Rect.Size.X, Element.Rect.Size.Y);
	}
}

TEST_CLASS(ArcRmlCssParityLayoutTests, "ArcRmlUI.CssParity.Layout")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Layout/OrderTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		return View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork;
	}

	void ExpectBoxes(const ArcRmlCssParityLayoutTestsPrivate::FChromeBox* Boxes, int32 Count)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FChromeBox& Box = Boxes[Index];
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Box.DomId);
			ASSERT_THAT(IsNotNull(Element, Box.DomId));
			ASSERT_THAT(IsTrue(IsNear(Element->Rect.Position.X, Box.X) && IsNear(Element->Rect.Position.Y, Box.Y) && IsNear(Element->Rect.Size.X, Box.Width)
					&& IsNear(Element->Rect.Size.Y, Box.Height),
				FString::Printf(TEXT("%s, Chrome %g,%g,%g,%g"), *Describe(*Element), Box.X, Box.Y, Box.Width, Box.Height)));
		}
	}

	void ExpectDrawOrder(const FColor& Below, const FColor& Above)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const int32 BelowIndex = FindDraw(View.GetDraws(), Below);
		const int32 AboveIndex = FindDraw(View.GetDraws(), Above);
		ASSERT_THAT(IsTrue(BelowIndex != INDEX_NONE, TEXT("the lower box has no draw")));
		ASSERT_THAT(IsTrue(AboveIndex != INDEX_NONE, TEXT("the upper box has no draw")));
		ASSERT_THAT(IsTrue(BelowIndex < AboveIndex, FString::Printf(TEXT("lower box draws at %d, upper box at %d"), BelowIndex, AboveIndex)));
	}

	void ExpectHit(const FVector2f& Point, const TCHAR* HitId, const TCHAR* HoverColour)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.MouseMove(Point);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, HitId);
		ASSERT_THAT(IsNotNull(Element, HitId));
		ASSERT_THAT(IsTrue(Element->BackgroundColor == HoverColour, FString::Printf(TEXT("%s background is %s, expected hover %s"), HitId, *Element->BackgroundColor, HoverColour)));
	}

	TEST_METHOD(FlexRow_PlacesItemsByOrderThenDomOrder)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("row-a"), 90, 0, 10, 20 },
			{ TEXT("row-b"), 0, 0, 20, 20 },
			{ TEXT("row-c"), 20, 0, 30, 20 },
			{ TEXT("row-d"), 50, 0, 40, 20 },
			{ TEXT("row-e"), 100, 0, 50, 20 },
			{ TEXT("row-abs"), 300, 0, 10, 10 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(FlexWrap_PlacesItemsByOrderAcrossLines)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("wrap"), 0, 40, 120, 40 },
			{ TEXT("wrap-b"), 0, 40, 20, 20 },
			{ TEXT("wrap-c"), 20, 40, 30, 20 },
			{ TEXT("wrap-d"), 50, 40, 40, 20 },
			{ TEXT("wrap-a"), 90, 40, 10, 20 },
			{ TEXT("wrap-e"), 0, 60, 50, 20 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(ColumnReverse_PlacesItemsByOrderFromTheEnd)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("colrev"), 0, 90, 50, 120 },
			{ TEXT("colrev-a"), 0, 150, 50, 10 },
			{ TEXT("colrev-b"), 0, 190, 50, 20 },
			{ TEXT("colrev-c"), 0, 160, 50, 30 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(DisplayContents_OrdersTheChildrenAsItemsOfTheContainer)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("contents-c"), 0, 220, 35, 20 },
			{ TEXT("contents-a"), 35, 220, 15, 20 },
			{ TEXT("contents-b"), 50, 220, 25, 20 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(Grid_AutoPlacesItemsByOrderAroundAnExplicitlyPlacedOne)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("grid-placed"), 40, 260, 40, 20 },
			{ TEXT("grid-b"), 0, 260, 40, 20 },
			{ TEXT("grid-c"), 80, 260, 40, 20 },
			{ TEXT("grid-d"), 0, 280, 40, 20 },
			{ TEXT("grid-a"), 40, 280, 40, 20 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(StaticOverlap_HigherOrderPaintsOver)
	{
		ExpectDrawOrder(FColor(0, 0, 255, 255), FColor(255, 0, 0, 255));
	}

	TEST_METHOD(StaticOverlap_HigherOrderIsHit)
	{
		ExpectHit(FVector2f(45.0f, 330.0f), TEXT("overlap-a"), TEXT("#00ff00ff"));
	}

	TEST_METHOD(RelativeOverlap_HigherOrderPaintsOver)
	{
		ExpectDrawOrder(FColor(128, 0, 255, 255), FColor(255, 128, 0, 255));
	}

	TEST_METHOD(RelativeOverlap_HigherOrderIsHit)
	{
		ExpectHit(FVector2f(45.0f, 380.0f), TEXT("overlap-rel-a"), TEXT("#00ffffff"));
	}

	TEST_METHOD(MixedDisplayGridCell_OrderDecidesPaint)
	{
		ExpectDrawOrder(FColor(0, 0, 192, 255), FColor(192, 0, 0, 255));
	}

	TEST_METHOD(MixedDisplayGridCell_OrderDecidesHit)
	{
		ExpectHit(FVector2f(30.0f, 430.0f), TEXT("mix-a"), TEXT("#00c000ff"));
	}

	TEST_METHOD(FloatInFlex_OrderDecidesPaint)
	{
		ExpectDrawOrder(FColor(192, 0, 192, 255), FColor(128, 64, 0, 255));
	}

	TEST_METHOD(FloatInFlex_OrderDecidesHit)
	{
		ExpectHit(FVector2f(45.0f, 480.0f), TEXT("flt-b"), TEXT("#408000ff"));
	}
};

// Expected numbers are Chrome's: the fixtures were dumped with ArcRml.LayoutDump and checked with
// chrome_layout_compare.py (flex and grid items carry the zero-specificity min-size auto rule), and every
// asserted box matched Chrome within 0.4 px.
TEST_CLASS(ArcRmlCssParityFlexMinAutoTests, "ArcRmlUI.CssParity.FlexMinAuto")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		FArcRmlTestView::DropCompiledDocument(Path);
		return View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork;
	}

	void ExpectBoxes(const TCHAR* RelativePath, const ArcRmlCssParityLayoutTestsPrivate::FChromeBox* Boxes, int32 Count, const TCHAR* DumpName = nullptr)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(RelativePath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		if (DumpName != nullptr)
		{
			View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), DumpName)));
			ASSERT_THAT(IsTrue(View.Flush() && View.Flush()));
		}
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FChromeBox& Box = Boxes[Index];
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Box.DomId);
			ASSERT_THAT(IsNotNull(Element, Box.DomId));
			ASSERT_THAT(IsTrue(IsNear(Element->Rect.Position.X, Box.X) && IsNear(Element->Rect.Position.Y, Box.Y) && IsNear(Element->Rect.Size.X, Box.Width)
					&& IsNear(Element->Rect.Size.Y, Box.Height),
				FString::Printf(TEXT("%s, Chrome %g,%g,%g,%g"), *Describe(*Element), Box.X, Box.Y, Box.Width, Box.Height)));
		}
	}

	void ExpectFixtureBoxes(const ArcRmlCssParityLayoutTestsPrivate::FChromeBox* Boxes, int32 Count)
	{
		ExpectBoxes(TEXT("CssParity/Layout/FlexMinAutoTest.rml"), Boxes, Count);
	}

	// The baseline of the first text run of a line: where the atomic inline on that line sits relative to the text.
	void ExpectLineBaseline(const TCHAR* LineDomId, float Baseline)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, LineDomId);
		ASSERT_THAT(IsTrue(Lines.Num() > 0, LineDomId));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(Lines[0].Baseline, Baseline),
			FString::Printf(TEXT("%s text baseline is %g, Chrome %g"), LineDomId, Lines[0].Baseline, Baseline)));
	}

	TEST_METHOD(InventoryActionRow_ItemsKeepTheirContentWidthAndOverflow)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("actions"), 20, 20, 370, 38 },
			{ TEXT("act-use"), 20, 20, 119.82f, 38 },
			{ TEXT("act-equip"), 147.82f, 20, 128.23f, 38 },
			{ TEXT("act-drop"), 284.05f, 20, 122.77f, 38 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(LongWordRow_ItemDoesNotShrinkBelowTheWord)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("word-row"), 20, 80, 100, 21 },
			{ TEXT("word"), 20, 80, 113.12f, 21 },
			{ TEXT("word-b"), 133.12f, 80, 19.5f, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(Column_ItemKeepsItsContentHeightOverTheShare)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("col"), 420, 20, 120, 60 },
			{ TEXT("col-tall"), 420, 20, 120, 105 },
			{ TEXT("col-b"), 420, 125, 120, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(OverflowHiddenItem_ShrinksBelowItsContent)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("hid"), 20, 120, 80.5f, 21 },
			{ TEXT("hid-b"), 100.5f, 120, 19.5f, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(MinWidthZeroItem_ShrinksBelowItsContent)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("zero"), 20, 160, 80.5f, 21 },
			{ TEXT("zero-b"), 100.5f, 160, 19.5f, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(ExplicitMinWidthAutoItem_KeepsTheAutomaticMinimum)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("auto-row"), 20, 20, 100, 21 },
			{ TEXT("auto"), 20, 20, 113.125f, 21 },
			{ TEXT("auto-b"), 133.125f, 20, 19.52f, 21 },
			{ TEXT("none"), 20, 60, 113.125f, 21 },
			{ TEXT("none-b"), 133.125f, 60, 19.52f, 21 },
		};
		ExpectBoxes(TEXT("CssParity/Layout/MinSizeAutoTest.rml"), Boxes, UE_ARRAY_COUNT(Boxes), TEXT("MinSizeAuto"));
	}

	TEST_METHOD(ExplicitMinSizeAutoBlock_IsZero)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("blk"), 20, 100, 30, 10 },
		};
		ExpectBoxes(TEXT("CssParity/Layout/MinSizeAutoTest.rml"), Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(SpecifiedWidthItem_MinimumIsTheSmallerSuggestion)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("spec"), 20, 200, 40, 21 },
			{ TEXT("spec-b"), 60, 200, 60, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(AspectRatioItem_MinimumIsTheTransferredSize)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("ar"), 420, 120, 100, 50 },
			{ TEXT("ar-b"), 520, 120, 19.5f, 50 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(NestedInlineFlex_SitsOnTheBaselineOfItsInnerContainer)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("nest-line"), 20, 260, 119.14f, 47 },
			{ TEXT("outer-if"), 57.71f, 260, 39.26f, 47 },
			{ TEXT("inner-if"), 60.71f, 263, 33.26f, 41 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectLineBaseline(TEXT("nest-line"), 292);
	}

	TEST_METHOD(NestedGridAndFlexInBaselineRow_AlignByTheirItemBaselines)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("base-row"), 20, 320, 151.98f, 57 },
			{ TEXT("base-text"), 20, 334, 34.97f, 36 },
			{ TEXT("base-grid"), 60.97f, 343, 43.69f, 30 },
			{ TEXT("base-flex"), 110.66f, 333, 25.22f, 44 },
			{ TEXT("base-block"), 141.88f, 320, 30.1f, 57 },
			{ TEXT("base-block-flex"), 141.88f, 333, 30.1f, 44 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(ColumnBaselineAlignedItem_ActsAsFlexStartAndTakesTheBaselineFromTheFirstItem)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("icol"), 57.71f, 420, 23.83f, 64 },
			{ TEXT("icol-a"), 59.71f, 422, 19.83f, 21 },
			{ TEXT("icol-b"), 59.71f, 443, 19.83f, 39 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectLineBaseline(TEXT("icol-line"), 438);
	}

	TEST_METHOD(CalcMainSizeInIndefiniteColumn_IsDefinite)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("calccol"), 20, 480, 80, 20 },
			{ TEXT("calc20"), 20, 480, 80, 20 },
			{ TEXT("calc50"), 20, 560, 50, 21 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(InlineBlockWithScrollContainerLastChild_SitsOnItsBottomMarginEdge)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("ib-line"), 20, 620, 111.01f, 62 },
			{ TEXT("ib"), 57.71f, 620, 31.12f, 59 },
			{ TEXT("ib-ovh"), 59.71f, 643, 27.12f, 30 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectLineBaseline(TEXT("ib-line"), 677);
	}

	TEST_METHOD(InlineTable_TakesItsBaselineFromTheFirstRow)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("itab-line"), 20, 660, 205.89f, 77 },
			{ TEXT("itab"), 57.71f, 660, 126, 77 },
			{ TEXT("tc-a"), 60.71f, 663, 24, 46 },
			{ TEXT("tc-c"), 60.71f, 709, 24, 25 },
		};
		ExpectFixtureBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectLineBaseline(TEXT("itab-line"), 681);
	}

	TEST_METHOD(FixedLineHeightLineWithFallbackGlyph_ScrollHeightIncludesTheContentArea)
	{
		ASSERT_THAT(IsTrue(OpenFixture(TEXT("CssParity/Text/LineEdgeProbe.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		FVector2f Fixed = FVector2f::ZeroVector;
		FVector2f Plain = FVector2f::ZeroVector;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadScrollSize(View, TEXT("mix-fixed"), Fixed)));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadScrollSize(View, TEXT("plain"), Plain)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Fixed.Y, 22.0f, 0.5f), FString::Printf(TEXT("mix-fixed scroll height is %g, Chrome 22"), Fixed.Y)));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Plain.Y, 24.0f, 0.5f), FString::Printf(TEXT("plain scroll height is %g, Chrome 24"), Plain.Y)));
	}
};

TEST_CLASS(ArcRmlCssParityIndefiniteAndTableTests, "ArcRmlUI.CssParity.IndefiniteAndTable")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Layout/Step13Test.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork))
		{
			return false;
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("Step13"))));
		return View.Flush() && View.Flush();
	}

	void ExpectBoxes(const ArcRmlCssParityLayoutTestsPrivate::FChromeBox* Boxes, int32 Count)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FChromeBox& Box = Boxes[Index];
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Box.DomId);
			ASSERT_THAT(IsNotNull(Element, Box.DomId));
			ASSERT_THAT(IsTrue(IsNear(Element->Rect.Position.X, Box.X) && IsNear(Element->Rect.Position.Y, Box.Y) && IsNear(Element->Rect.Size.X, Box.Width)
					&& IsNear(Element->Rect.Size.Y, Box.Height),
				FString::Printf(TEXT("%s, Chrome %g,%g,%g,%g"), *Describe(*Element), Box.X, Box.Y, Box.Width, Box.Height)));
		}
	}

	void ExpectContentSize(const TCHAR* DomId, float Width, float Height)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(Element->ContentSize.X, Width) && ArcRmlCssParityLayoutTestsPrivate::IsNear(Element->ContentSize.Y, Height),
			FString::Printf(TEXT("%s content is %g,%g, Chrome %g,%g"), DomId, Element->ContentSize.X, Element->ContentSize.Y, Width, Height)));
	}

	void ExpectScrollWidth(const TCHAR* DomId, float Width)
	{
		FVector2f ScrollSize = FVector2f::ZeroVector;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadScrollSize(View, DomId, ScrollSize), DomId));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(ScrollSize.X, Width), FString::Printf(TEXT("%s scroll width is %g, Chrome %g"), DomId, ScrollSize.X, Width)));
	}

	void ExpectTextBaseline(const TCHAR* CellDomId, float Baseline)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const TArray<FArcRmlTestTextLine> Lines = ArcRmlUITestHelpers::GetLines(Elements, CellDomId);
		ASSERT_THAT(IsTrue(Lines.Num() > 0, CellDomId));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(Lines[0].Baseline, Baseline),
			FString::Printf(TEXT("%s text baseline is %g, Chrome %g"), CellDomId, Lines[0].Baseline, Baseline)));
	}

	TEST_METHOD(FlexItemPercentageHeight_InAutoHeightContainerActsAsAuto)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("img-row"), 700, 640, 5, 12 },
			{ TEXT("img-pct"), 700, 640, 12, 12 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(StretchedRowItem_PercentageMaxHeightIsNone)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("fp1"), 1100, 760, 300, 40 },
			{ TEXT("fp1-a"), 1100, 760, 113.3f, 40 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(RowItem_PercentageMinHeightCountsAsZero)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("fp1-b"), 1213.3f, 760, 27.49f, 40 },
			{ TEXT("fp1-c"), 1240.78f, 760, 50, 40 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(AutoColumn_PercentageHeightWithPercentageMaxHeightIsAuto)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("fp2"), 1100, 840, 300, 42 },
			{ TEXT("fp2-a"), 1100, 840, 300, 21 },
			{ TEXT("fp2-b"), 1100, 861, 300, 21 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(AutoColumn_PercentageFlexBasisWithPercentageMinHeightIsAuto)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("fp3"), 1100, 940, 300, 42 },
			{ TEXT("fp3-a"), 1100, 940, 300, 21 },
			{ TEXT("fp3-b"), 1100, 961, 300, 21 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(AtomicInlineBox_OverflowWidensTheParentScrollWidth)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectScrollWidth(TEXT("calcrow"), 113.0f);
	}

	TEST_METHOD(InlineBlock_OverflowHiddenDoesNotPassItsContentOverflowOn)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectScrollWidth(TEXT("ovx-vis-line"), 100.0f);
		ExpectScrollWidth(TEXT("ovx-hid-line"), 50.0f);
	}

	TEST_METHOD(TableCell_AlignmentOffsetIsNotReportedAsPadding)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tc-a"), 60.71f, 923, 24.37f, 46 },
			{ TEXT("tc-b"), 85.08f, 923, 95.63f, 46 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectContentSize(TEXT("tc-b"), 91.63f, 42.0f);
	}

	TEST_METHOD(InlineTable_ShrinksToFitItsColumns)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tl1"), 1100, 20, 229.15f, 27 },
			{ TEXT("ita"), 1137.71f, 20, 149.26f, 27 },
			{ TEXT("ta1"), 1137.71f, 20, 48.9f, 27 },
			{ TEXT("ta2"), 1186.61f, 20, 100.37f, 27 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(TableAsFlexItem_TakesItsContentWidth)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tl2"), 1100, 120, 119.9f, 27 },
			{ TEXT("tfa"), 1100, 120, 119.9f, 27 },
			{ TEXT("tfa1"), 1100, 120, 70.0f, 27 },
			{ TEXT("tfa2"), 1170.0f, 120, 49.9f, 27 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(BlockTable_SharesItsWidthBetweenWrappingColumns)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tba"), 1100, 220, 400, 96 },
			{ TEXT("tba1"), 1100, 220, 147.69f, 48 },
			{ TEXT("tba2"), 1247.69f, 220, 252.31f, 48 },
			{ TEXT("tba3"), 1100, 268, 147.69f, 48 },
			{ TEXT("tba4"), 1247.69f, 268, 252.31f, 48 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(NarrowBlockTable_KeepsTheMinContentWidthOfItsColumns)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tnr"), 1100, 360, 143.23f, 90 },
			{ TEXT("tnr1"), 1100, 360, 78.38f, 90 },
			{ TEXT("tnr2"), 1178.38f, 360, 64.85f, 90 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(FixedWidthTable_GivesTheExtraWidthToTheAutoColumnsByMaxContent)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("twd"), 1100, 480, 300, 27 },
			{ TEXT("twd1"), 1100, 480, 48.83f, 27 },
			{ TEXT("twd2"), 1148.83f, 480, 251.17f, 27 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
	}

	TEST_METHOD(TableCells_BaselineAlignedShareTheRowBaselineTopAndMiddleDoNot)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tbl"), 1100, 620, 161.82f, 74 },
			{ TEXT("tbl1"), 1100, 620, 43.15f, 37 },
			{ TEXT("tbl2"), 1143.15f, 620, 35.42f, 37 },
			{ TEXT("tbl3"), 1178.58f, 620, 33.26f, 37 },
			{ TEXT("tbl4"), 1211.84f, 620, 33.98f, 37 },
			{ TEXT("tbl5"), 1245.82f, 620, 16, 37 },
			{ TEXT("tbl6"), 1100, 657, 43.15f, 37 },
			{ TEXT("tbl7"), 1143.15f, 657, 35.42f, 37 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectTextBaseline(TEXT("tbl1"), 647.0f);
		ExpectTextBaseline(TEXT("tbl2"), 647.0f);
		ExpectTextBaseline(TEXT("tbl3"), 639.0f);
		ExpectTextBaseline(TEXT("tbl4"), 644.0f);
		ExpectTextBaseline(TEXT("tbl6"), 684.0f);
		ExpectTextBaseline(TEXT("tbl7"), 684.0f);
		ExpectTextBaseline(TEXT("tbl8"), 684.0f);
		ExpectTextBaseline(TEXT("tbl9"), 684.0f);
	}

	TEST_METHOD(TableCell_BaselineShiftKeepsItsOwnPadding)
	{
		const ArcRmlCssParityLayoutTestsPrivate::FChromeBox Boxes[] = {
			{ TEXT("tbl2"), 1143.15f, 620, 35.42f, 37 },
			{ TEXT("tbl6"), 1100, 657, 43.15f, 37 },
		};
		ExpectBoxes(Boxes, UE_ARRAY_COUNT(Boxes));
		ExpectContentSize(TEXT("tbl2"), 29.42f, 28.0f);
		ExpectContentSize(TEXT("tbl6"), 37.15f, 28.0f);
	}
};

namespace ArcRmlCssParityScrollbarSpaceTestsPrivate
{
	const TCHAR* const FixturePath = TEXT("CssParity/Scroll/ScrollbarSpaceTest.rml");

	const FArcRmlTestLayoutElement* FindBar(const TArray<FArcRmlTestLayoutElement>& Elements, const FArcRmlTestLayoutElement& Owner, const TCHAR* Tag)
	{
		for (const FArcRmlTestLayoutElement& Element : Elements)
		{
			if (Element.Kind == TEXT("scrollbar") && Element.Parent == Owner.Path && Element.Tag == Tag)
			{
				return &Element;
			}
		}
		return nullptr;
	}

	FBox2f ToBox(const FArcRmlTestLayoutElement& Element)
	{
		return FBox2f(FVector2f(Element.Rect.Position.X, Element.Rect.Position.Y),
			FVector2f(Element.Rect.Position.X + Element.Rect.Size.X, Element.Rect.Position.Y + Element.Rect.Size.Y));
	}
}

TEST_CLASS(ArcRmlCssParityScrollbarSpaceTests, "ArcRmlUI.CssParity.ScrollbarSpace")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(ArcRmlCssParityScrollbarSpaceTestsPrivate::FixturePath);
		FArcRmlTestView::DropCompiledDocument(Path);
		const FString Sheet = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FPaths::GetPath(FString(ArcRmlCssParityScrollbarSpaceTestsPrivate::FixturePath)),
			FPaths::GetBaseFilename(FString(ArcRmlCssParityScrollbarSpaceTestsPrivate::FixturePath)) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
		return View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0 && !View.GetProbe().bPendingAsyncWork
			&& View.ParseLayout(Elements);
	}

	bool ReadFixture()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	void ExpectClientWidth(const TCHAR* Id, float Width)
	{
		FVector2f ScrollSize = FVector2f::ZeroVector;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadScrollSize(View, Id, ScrollSize), Id));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(ScrollSize.X, Width), FString::Printf(TEXT("%s client width is %g, Chrome %g"), Id, ScrollSize.X, Width)));
	}

	void ExpectBoxWidth(const TCHAR* Id, float Width)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		ASSERT_THAT(IsNotNull(Element, Id));
		ASSERT_THAT(IsTrue(ArcRmlCssParityLayoutTestsPrivate::IsNear(Element->Rect.Size.X, Width), FString::Printf(TEXT("%s width is %g, Chrome %g"), Id, Element->Rect.Size.X, Width)));
	}

	void ExpectNoHorizontalBar(const TCHAR* Id)
	{
		using namespace ArcRmlCssParityScrollbarSpaceTestsPrivate;
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		ASSERT_THAT(IsNotNull(Owner, Id));
		const FArcRmlTestLayoutElement* Horizontal = FindBar(Elements, *Owner, TEXT("scrollbarhorizontal"));
		ASSERT_THAT(IsTrue(Horizontal == nullptr || !Horizontal->bVisible, FString::Printf(TEXT("%s has a horizontal bar, Chrome has none"), Id)));
	}

	void ExpectVerticalBarOnly(const TCHAR* Id, float Thickness)
	{
		using namespace ArcRmlCssParityScrollbarSpaceTestsPrivate;
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		ASSERT_THAT(IsNotNull(Owner, Id));
		const FArcRmlTestLayoutElement* Vertical = FindBar(Elements, *Owner, TEXT("scrollbarvertical"));
		ASSERT_THAT(IsNotNull(Vertical, FString::Printf(TEXT("%s has no vertical bar"), Id)));
		ASSERT_THAT(IsTrue(Vertical->bVisible && ArcRmlCssParityLayoutTestsPrivate::IsNear(Vertical->Rect.Size.X, Thickness),
			FString::Printf(TEXT("%s vertical bar is %g px wide, Chrome %g"), Id, Vertical->Rect.Size.X, Thickness)));
		ExpectNoHorizontalBar(Id);
	}

	void ExpectLocalTile(const TCHAR* Id, float InkMinX, float InkMaxX)
	{
		using namespace ArcRmlCssParityScrollbarSpaceTestsPrivate;
		const FArcRmlTestLayoutElement* Owner = ArcRmlUITestHelpers::FindByDomId(Elements, Id);
		ASSERT_THAT(IsNotNull(Owner, Id));
		const FBox2f Region = ToBox(*Owner);
		int32 Count = 0;
		FBox2f Ink(ForceInit);
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			const bool bTile = Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.bShader && FMath::IsNearlyEqual(Draw.TileSize.X, 20.0f, 0.01f)
				&& FMath::IsNearlyEqual(Draw.TileSize.Y, 20.0f, 0.01f) && Draw.Bounds.Min.X >= Region.Min.X - 0.5f && Draw.Bounds.Max.X <= Region.Max.X + 0.5f
				&& Draw.Bounds.Min.Y >= Region.Min.Y - 0.5f && Draw.Bounds.Max.Y <= Region.Max.Y + 0.5f;
			if (bTile)
			{
				Ink = Draw.Bounds;
				++Count;
			}
		}
		ASSERT_THAT(AreEqual(1, Count, FString::Printf(TEXT("%s: 20x20 tile draws"), Id)));
		const float Left = Region.Min.X + InkMinX;
		const float Right = Region.Min.X + InkMaxX + 1.0f;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Ink.Min.X, Left, 0.5f) && FMath::IsNearlyEqual(Ink.Max.X, Right, 0.5f)
				&& FMath::IsNearlyEqual(Ink.Min.Y, Region.Min.Y, 0.5f) && FMath::IsNearlyEqual(Ink.Max.Y, Region.Min.Y + 20.0f, 0.5f),
			FString::Printf(TEXT("%s: tile ink x %g-%g y %g-%g, Chrome x %g-%g y %g-%g"), Id, Ink.Min.X, Ink.Max.X, Ink.Min.Y, Ink.Max.Y, Left, Right, Region.Min.Y, Region.Min.Y + 20.0f)));
	}

	bool SetOn(bool bOn)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("on"), bOn);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before && View.FlushUntilSettled() && ReadFixture();
	}

	TEST_METHOD(Unstyled_OverflowingBoxTakesTheNative15pxVerticalBar)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClientWidth(TEXT("c1"), 105.0f);
		ExpectVerticalBarOnly(TEXT("c1"), 15.0f);
	}

	TEST_METHOD(Unstyled_OverflowYScrollWithoutOverflowStillTakesTheBar)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClientWidth(TEXT("c2"), 105.0f);
		ExpectVerticalBarOnly(TEXT("c2"), 15.0f);
	}

	TEST_METHOD(WebkitThumbRuleAlone_KeepsTheNative15pxBar)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClientWidth(TEXT("c3"), 105.0f);
		ExpectVerticalBarOnly(TEXT("c3"), 15.0f);
	}

	TEST_METHOD(WebkitScrollbarWidth6_TakesSixPixels)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClientWidth(TEXT("c4"), 114.0f);
		ExpectVerticalBarOnly(TEXT("c4"), 6.0f);
	}

	TEST_METHOD(ScrollbarWidthThin_WinsOverTheWebkitRule)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectClientWidth(TEXT("c5"), 110.0f);
		ExpectVerticalBarOnly(TEXT("c5"), 10.0f);
	}

	TEST_METHOD(AbsoluteChildLeftRightZero_FillsTheScrollport)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxWidth(TEXT("c6abs"), 105.0f);
	}

	TEST_METHOD(AutoWidthAbsoluteScrollContainer_AddsTheBarToItsContent)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxWidth(TEXT("c7"), 115.0f);
		ExpectNoHorizontalBar(TEXT("c7"));
	}

	TEST_METHOD(InlineBlockAndFlexScrollContainers_AddTheBarToTheirContent)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxWidth(TEXT("c8a"), 115.0f);
		ExpectNoHorizontalBar(TEXT("c8a"));
		ExpectBoxWidth(TEXT("c8b"), 115.0f);
		ExpectNoHorizontalBar(TEXT("c8b"));
	}

	TEST_METHOD(ColumnFlexScrollContainer_ItemSpansTheScrollport)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxWidth(TEXT("c9item"), 105.0f);
	}

	TEST_METHOD(LocalBackgroundTile_StopsBeforeTheBar)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectLocalTile(TEXT("c10"), 85.0f, 104.0f);
		ExpectLocalTile(TEXT("c10b"), 94.0f, 113.0f);
	}

	TEST_METHOD(WebkitRuleToggledByAClass_SwitchesTheBarAndTheLayout)
	{
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectBoxWidth(TEXT("c11child"), 105.0f);
		ExpectVerticalBarOnly(TEXT("c11"), 15.0f);
		ASSERT_THAT(IsTrue(SetOn(true)));
		ExpectBoxWidth(TEXT("c11child"), 114.0f);
		ExpectVerticalBarOnly(TEXT("c11"), 6.0f);
		ASSERT_THAT(IsTrue(SetOn(false)));
		ExpectBoxWidth(TEXT("c11child"), 105.0f);
		ExpectVerticalBarOnly(TEXT("c11"), 15.0f);
	}
};

// "-w" parents sit in a 150 px containing block (max-content width), "-n" parents in a 0 px one (min-content width).
TEST_CLASS(ArcRmlCssParityReplacedIntrinsicTests, "ArcRmlUI.CssParity.ReplacedIntrinsic")
{
	struct FExpectedWidth
	{
		FString DomId;
		float Width;
	};

	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	FString DescribeMismatches(const TCHAR* Fixture, const TArray<FExpectedWidth>& Expected)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(FString(TEXT("CssParity/Replaced/")) + Fixture + TEXT(".rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork))
		{
			return FString::Printf(TEXT("%s did not open: %s"), Fixture, *ArcRmlUITestHelpers::DescribeDiagnostics(View));
		}
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("ReplacedIntrinsic")));
		View.DumpLayout(Dir);
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!(View.Flush() && View.Flush() && View.ParseLayout(Elements)))
		{
			return FString::Printf(TEXT("%s has no layout"), Fixture);
		}
		FString Mismatches;
		for (const FExpectedWidth& Entry : Expected)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, *Entry.DomId);
			if (Element == nullptr)
			{
				Mismatches += FString::Printf(TEXT("%s missing; "), *Entry.DomId);
			}
			else if (!IsNear(Element->Rect.Size.X, Entry.Width))
			{
				Mismatches += FString::Printf(TEXT("%s, Chrome width %g; "), *Describe(*Element), Entry.Width);
			}
		}
		return Mismatches;
	}

	void ExpectParentWidths(const TCHAR* Case, float MaxContentWidth, float WideChildWidth, float MinContentWidth, float NarrowChildWidth)
	{
		TArray<FExpectedWidth> Expected;
		const TCHAR* Kinds[] = { TEXT("fi"), TEXT("ib"), TEXT("fl"), TEXT("ab") };
		for (const TCHAR* Kind : Kinds)
		{
			Expected.Add({ FString::Printf(TEXT("%s-%s-w"), Kind, Case), MaxContentWidth });
			Expected.Add({ FString::Printf(TEXT("%s-%s-w-r"), Kind, Case), WideChildWidth });
			Expected.Add({ FString::Printf(TEXT("%s-%s-n"), Kind, Case), MinContentWidth });
			Expected.Add({ FString::Printf(TEXT("%s-%s-n-r"), Kind, Case), NarrowChildWidth });
		}
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(TEXT("ReplacedIntrinsicTest"), Expected)));
	}

	void ExpectGridWidths(const TArray<FExpectedWidth>& Expected)
	{
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(TEXT("ReplacedIntrinsicGridTest"), Expected)));
	}

	TEST_METHOD(FixedWidthImage_AddsItsWidthToBothContributions)
	{
		ExpectParentWidths(TEXT("px"), 72.0f, 72.0f, 72.0f, 72.0f);
	}

	TEST_METHOD(CalcWidthImage_AddsItsWidthToBothContributions)
	{
		ExpectParentWidths(TEXT("calc"), 72.0f, 72.0f, 72.0f, 72.0f);
	}

	TEST_METHOD(PercentageWidthImage_AddsItsNaturalWidthToTheMaxContentAndMinWidthToTheMinContent)
	{
		ExpectParentWidths(TEXT("pct"), 120.0f, 120.0f, 0.0f, 0.0f);
	}

	TEST_METHOD(PercentageMaxWidthImage_AddsItsNaturalWidthToTheMaxContentAndZeroToTheMinContent)
	{
		ExpectParentWidths(TEXT("maxpct"), 120.0f, 120.0f, 0.0f, 0.0f);
	}

	TEST_METHOD(SvgSizedByHeightAndViewBox_AddsTheRatioWidthToBothContributions)
	{
		ExpectParentWidths(TEXT("svg"), 37.33f, 37.33f, 37.33f, 37.33f);
	}

	TEST_METHOD(FixedWidthImageUnderALargerMinWidth_AddsTheMinWidthToBothContributions)
	{
		ExpectParentWidths(TEXT("min"), 100.0f, 100.0f, 100.0f, 100.0f);
	}

	TEST_METHOD(PercentageWidthImageWithAMinWidth_AddsTheMinWidthToTheMinContent)
	{
		ExpectParentWidths(TEXT("pctmin"), 120.0f, 120.0f, 30.0f, 30.0f);
	}

	TEST_METHOD(PercentageCalcWidthImage_AddsItsNaturalWidthToTheMaxContentAndMinWidthToTheMinContent)
	{
		ExpectParentWidths(TEXT("calcpct"), 120.0f, 120.0f, 0.0f, 0.0f);
	}

	TEST_METHOD(GridAutoTrack_TakesTheImageContributions)
	{
		ExpectGridWidths({
			{ TEXT("ga-px-w"), 72.0f }, { TEXT("ga-px-n"), 72.0f },
			{ TEXT("ga-pct-w"), 120.0f }, { TEXT("ga-pct-w-r"), 120.0f }, { TEXT("ga-pct-n"), 0.0f }, { TEXT("ga-pct-n-r"), 0.0f },
		});
	}

	TEST_METHOD(GridMinContentTrack_TakesTheImageMinContentContributions)
	{
		ExpectGridWidths({
			{ TEXT("gm-px-w"), 72.0f }, { TEXT("gm-px-n"), 72.0f },
			{ TEXT("gm-pct-w"), 0.0f }, { TEXT("gm-pct-n"), 0.0f },
		});
	}

	TEST_METHOD(GridItemInANarrowerFixedTrack_FitsThePercentageImageToTheTrack)
	{
		ExpectGridWidths({
			{ TEXT("gs-pct"), 50.0f }, { TEXT("gs-pct-r"), 50.0f },
			{ TEXT("gs-maxpct"), 50.0f }, { TEXT("gs-maxpct-r"), 50.0f },
		});
	}

	TEST_METHOD(InlineFlexAroundAnImageWithALengthFlexBasis_KeepsTheNaturalWidthAsTheAutomaticMinimum)
	{
		ExpectGridWidths({
			{ TEXT("fb-w"), 120.0f }, { TEXT("fb-w-r"), 120.0f },
			{ TEXT("fb-n"), 120.0f }, { TEXT("fb-n-r"), 120.0f },
		});
	}
};

// The layout dump's Chrome page gives each img the natural size RmlUi uses without width / height attributes.
TEST_CLASS(ArcRmlCssParityDumpImageTests, "ArcRmlUI.CssParity.DumpImage")
{
	struct FExpectedImage
	{
		const TCHAR* DomId;
		float Width;
		float Height;
		const TCHAR* Source;
	};

	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	static FString FindImageTag(const FString& Html, const TCHAR* DomId)
	{
		const int32 IdAt = Html.Find(FString::Printf(TEXT(" id=\"%s\""), DomId));
		if (IdAt == INDEX_NONE)
		{
			return FString();
		}
		const int32 Start = Html.Find(TEXT("<img"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, IdAt);
		const int32 End = Html.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, IdAt);
		return Start == INDEX_NONE || End == INDEX_NONE ? FString() : Html.Mid(Start, End - Start + 1);
	}

	FString DescribeMismatches(float DpRatio, const TArray<FExpectedImage>& Expected)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Replaced/DumpImageTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(View.Open(Path, ArcRmlUITestHelpers::ViewSize, DpRatio) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork))
		{
			return FString::Printf(TEXT("DumpImageTest did not open: %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View));
		}
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("DumpImage"),
			FString::Printf(TEXT("Dp%g"), DpRatio)));
		View.DumpLayout(Dir);
		TArray<FArcRmlTestLayoutElement> Elements;
		FString Html;
		if (!(View.Flush() && View.Flush() && View.ParseLayout(Elements)
			&& FFileHelper::LoadFileToString(Html, *FPaths::Combine(Dir, TEXT("DumpImageTest.html")))))
		{
			return TEXT("DumpImageTest has no layout dump");
		}
		FString Mismatches;
		for (const FExpectedImage& Entry : Expected)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Entry.DomId);
			const FString Tag = FindImageTag(Html, Entry.DomId);
			if (Element == nullptr || Tag.IsEmpty())
			{
				Mismatches += FString::Printf(TEXT("%s missing; "), Entry.DomId);
				continue;
			}
			if (!IsNear(Element->Rect.Size.X, Entry.Width) || !IsNear(Element->Rect.Size.Y, Entry.Height))
			{
				Mismatches += FString::Printf(TEXT("%s, Chrome %gx%g; "), *Describe(*Element), Entry.Width, Entry.Height);
			}
			// Chrome at device scale 1 picks a src over a srcset density below 1x, so a tag carries one of them.
			if (Tag.Contains(TEXT(" width=")) || Tag.Contains(TEXT(" height=")) || !Tag.Contains(Entry.Source)
				|| (Tag.Contains(TEXT(" src=\"")) && Tag.Contains(TEXT(" srcset=\""))))
			{
				Mismatches += FString::Printf(TEXT("%s page tag %s, expected %s, one image source and no width or height; "), Entry.DomId, *Tag, Entry.Source);
			}
		}
		return Mismatches;
	}

	TEST_METHOD(FileAndSpriteImagesAtDpRatioOne_GiveChromeTheirNaturalSize)
	{
		const TCHAR* File = TEXT(" src=\"file:///");
		const TCHAR* Sprite = TEXT(" src=\"data:image/svg+xml,%3Csvg%20xmlns='http://www.w3.org/2000/svg'%20width='60'%20height='40'/%3E\"");
		const TCHAR* Undecoded = TEXT(" src=\"data:image/svg+xml,%3Csvg%20xmlns='http://www.w3.org/2000/svg'%20width='0'%20height='0'/%3E\"");
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(1.0f, {
			{ TEXT("file-w"), 72.0f, 48.0f, File },
			{ TEXT("file-h"), 48.0f, 32.0f, File },
			{ TEXT("file-n"), 120.0f, 80.0f, File },
			{ TEXT("sprite-w"), 72.0f, 48.0f, Sprite },
			{ TEXT("sprite-n"), 60.0f, 40.0f, Sprite },
			{ TEXT("file-svg"), 0.0f, 0.0f, Undecoded },
		})));
	}

	TEST_METHOD(FileAndSpriteImagesAtDpRatioOneAndAHalf_GiveChromeTheScaledNaturalSize)
	{
		const TCHAR* File = TEXT("Image120x80.png 0.666666687x\"");
		const TCHAR* Sprite = TEXT(" src=\"data:image/svg+xml,%3Csvg%20xmlns='http://www.w3.org/2000/svg'%20width='90'%20height='60'/%3E\"");
		const TCHAR* Undecoded = TEXT(" src=\"data:image/svg+xml,%3Csvg%20xmlns='http://www.w3.org/2000/svg'%20width='0'%20height='0'/%3E\"");
		ASSERT_THAT(AreEqual(FString(), DescribeMismatches(1.5f, {
			{ TEXT("file-w"), 108.0f, 72.0f, File },
			{ TEXT("file-h"), 72.0f, 48.0f, File },
			{ TEXT("file-n"), 180.0f, 120.0f, File },
			{ TEXT("sprite-w"), 108.0f, 72.0f, Sprite },
			{ TEXT("sprite-n"), 90.0f, 60.0f, Sprite },
			{ TEXT("file-svg"), 0.0f, 0.0f, Undecoded },
		})));
	}
};

// Each table holds two columns in a 720 px block; the "-wide" tables' columns have a 450 px min-content and a 600 px
// max-content sum, the others 300 and 600. Expected widths come from Chrome on the TableWidthTest fixture.
TEST_CLASS(ArcRmlCssParityTableWidthTests, "ArcRmlUI.CssParity.TableWidth")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectWidths(const TCHAR* Table, float TableWidth, float FirstCellWidth, float SecondCellWidth)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Table/TableWidthTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), TEXT("TableWidth"))));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));
		const TPair<FString, float> Expected[] = {
			{ FString(Table), TableWidth },
			{ FString::Printf(TEXT("%s-a"), Table), FirstCellWidth },
			{ FString::Printf(TEXT("%s-b"), Table), SecondCellWidth },
		};
		FString Mismatches;
		for (const TPair<FString, float>& Entry : Expected)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, *Entry.Key);
			if (Element == nullptr)
			{
				Mismatches += FString::Printf(TEXT("%s missing; "), *Entry.Key);
			}
			else if (!IsNear(Element->Rect.Size.X, Entry.Value))
			{
				Mismatches += FString::Printf(TEXT("%s, Chrome width %g; "), *Describe(*Element), Entry.Value);
			}
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches));
	}

	TEST_METHOD(AutoWidthUnderMinWidthFull_FillsTheMinWidthAndScalesItsColumns)
	{
		ExpectWidths(TEXT("t-minfull"), 720.0f, 360.0f, 360.0f);
	}

	TEST_METHOD(AutoWidth_IsCappedAtTheMaxContentWidth)
	{
		ExpectWidths(TEXT("t-auto"), 600.0f, 300.0f, 300.0f);
	}

	TEST_METHOD(PercentageWidth_FillsTheContainer)
	{
		ExpectWidths(TEXT("t-full"), 720.0f, 360.0f, 360.0f);
	}

	TEST_METHOD(AutoWidthUnderMaxWidth_TakesTheMaxWidth)
	{
		ExpectWidths(TEXT("t-max400"), 400.0f, 200.0f, 200.0f);
	}

	TEST_METHOD(AutoWidthUnderAMaxWidthBelowItsColumns_KeepsTheColumnsMinContentWidth)
	{
		ExpectWidths(TEXT("t-max400-wide"), 450.0f, 300.0f, 150.0f);
	}

	TEST_METHOD(DefiniteWidthUnderMaxWidth_TakesTheMaxWidth)
	{
		ExpectWidths(TEXT("t-def-max400"), 400.0f, 200.0f, 200.0f);
	}

	TEST_METHOD(DefiniteWidthUnderAMaxWidthBelowItsColumns_KeepsTheColumnsMinContentWidth)
	{
		ExpectWidths(TEXT("t-def-max400-wide"), 450.0f, 300.0f, 150.0f);
	}
};

namespace ArcRmlCssParityLayoutTestsPrivate
{
	// A box with a negative Height is checked for its x, y and width only.
	// Opens the fixture in the test view size; a non-zero ResizeTo then resizes the open view, so the boxes are checked after a resize.
	// With bTransformed the boxes are the bounding boxes of the border boxes under their transforms (Chrome's getBoundingClientRect with
	// transforms on, chrome_layout_compare.py --animations-at), else the untransformed border boxes.
	FString CheckChromeBoxes(FArcRmlTestView& View, const TCHAR* Fixture, const TCHAR* DumpFolder, TConstArrayView<FChromeBox> Boxes,
		FIntPoint ResizeTo = FIntPoint::ZeroValue, float Tolerance = 0.5f, bool bTransformed = false)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork))
		{
			return ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		if (ResizeTo != FIntPoint::ZeroValue)
		{
			View.Resize(ResizeTo, 1.0f);
			if (!(View.FlushUntilSettled() && !View.GetProbe().bPendingAsyncWork))
			{
				return ArcRmlUITestHelpers::DescribeDiagnostics(View);
			}
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), DumpFolder)));
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!(View.Flush() && View.Flush() && View.ParseLayout(Elements)))
		{
			return TEXT("layout not parsed");
		}
		FString Mismatches;
		for (const FChromeBox& Box : Boxes)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Box.DomId);
			if (Element == nullptr)
			{
				Mismatches += FString::Printf(TEXT("%s missing; "), Box.DomId);
				continue;
			}
			const FArcRmlInspectorRect& Rect = bTransformed ? Element->TransformedRect : Element->Rect;
			if (!(IsNear(Rect.Position.X, Box.X, Tolerance) && IsNear(Rect.Position.Y, Box.Y, Tolerance)
				&& IsNear(Rect.Size.X, Box.Width, Tolerance) && (Box.Height < 0.0f || IsNear(Rect.Size.Y, Box.Height, Tolerance))))
			{
				Mismatches += bTransformed
					? FString::Printf(TEXT("%s transformed %g,%g,%g,%g, Chrome %g,%g,%g,%g; "), Box.DomId, Rect.Position.X, Rect.Position.Y, Rect.Size.X,
						Rect.Size.Y, Box.X, Box.Y, Box.Width, Box.Height)
					: FString::Printf(TEXT("%s, Chrome %g,%g,%g,%g; "), *Describe(*Element), Box.X, Box.Y, Box.Width, Box.Height);
			}
		}
		return Mismatches;
	}
}

// Each line ends in a collapsible space inside an inline box, followed by white space that collapses away, an empty inline box
// or a wrap before an atomic inline. Expected boxes come from Chrome on the TrailingSpaceTest fixture.
TEST_CLASS(ArcRmlCssParityTrailingSpaceTests, "ArcRmlUI.CssParity.TrailingSpace")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Text/TrailingSpaceTest.rml"),
			TEXT("TrailingSpace"), Boxes)));
	}

	TEST_METHOD(SpaceBeforeAClosedLinkAndCollapsedWhiteSpace_IsRemovedFromTheShrinkToFitWidth)
	{
		ExpectBoxes({ { TEXT("ts-a"), 0.0f, 0.0f, 9.5f, 20.0f }, { TEXT("ts-a-in"), 0.0f, -1.0f, 9.5f, 21.0f } });
	}

	TEST_METHOD(SpaceBeforeAClosedSpanAndANewline_IsRemovedFromTheShrinkToFitWidth)
	{
		ExpectBoxes({ { TEXT("ts-b"), 0.0f, 24.0f, 9.5f, 20.0f }, { TEXT("ts-b-in"), 0.0f, 23.0f, 9.5f, 21.0f } });
	}

	// Guard: the walk skipped a trailing empty inline box before the change too.
	TEST_METHOD(SpaceBeforeAClosedBoldAndAnEmptyInlineBox_IsRemovedFromTheShrinkToFitWidth)
	{
		ExpectBoxes({ { TEXT("ts-c"), 0.0f, 48.0f, 9.484375f, -1.0f }, { TEXT("ts-c-in"), 0.0f, 47.0f, 9.484375f, 21.0f } });
	}

	TEST_METHOD(SpaceBeforeAClosedSpanInACentredLine_DoesNotShiftTheLine)
	{
		ExpectBoxes({ { TEXT("ts-d"), 0.0f, 72.0f, 100.0f, 20.0f }, { TEXT("ts-d-in"), 45.25f, 71.0f, 9.5f, 21.0f } });
	}

	TEST_METHOD(SpaceBeforeAWrapToAnAtomicInline_IsRemovedFromTheRightAlignedLine)
	{
		ExpectBoxes({ { TEXT("ts-e"), 0.0f, 96.0f, 40.0f, 40.0f }, { TEXT("ts-e-in"), 30.5f, 95.0f, 9.5f, 21.0f },
			{ TEXT("ts-e-ib"), 0.0f, 121.0f, 40.0f, 10.0f } });
	}

	// Guard: preserved white space keeps the hanging path.
	TEST_METHOD(PreservedSpaceInPreWrap_StaysInTheShrinkToFitWidth)
	{
		ExpectBoxes({ { TEXT("ts-f"), 0.0f, 120.0f, 13.609375f, 20.0f }, { TEXT("ts-f-in"), 0.0f, 119.0f, 13.609375f, 21.0f } });
	}

	TEST_METHOD(TextWithoutASpace_IsAsWideAsItsText)
	{
		ExpectBoxes({ { TEXT("ts-g"), 0.0f, 144.0f, 9.5f, 20.0f } });
	}
};

// Absolutely positioned 10 x 10 children without insets in 100 x 40 flex containers with 8 px padding, 4 px apart. Expected boxes
// come from Chrome on the FlexAbsStaticTest fixture.
TEST_CLASS(ArcRmlCssParityFlexAbsStaticTests, "ArcRmlUI.CssParity.FlexAbsStatic")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/FlexAbsStaticTest.rml"),
			TEXT("FlexAbsStatic"), Boxes)));
	}

	TEST_METHOD(RowFlexStart_IsAtTheContentBoxStart)
	{
		ExpectBoxes({ { TEXT("fa-start-c"), 8.0f, 8.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(RowCentreAndAlignItemsCentre_IsCentredInTheContentBox)
	{
		ExpectBoxes({ { TEXT("fa-centre-c"), 53.0f, 83.0f, 10.0f, 10.0f }, { TEXT("fa-margin-c"), 53.0f, 683.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(RowFlexEnd_IsAtTheContentBoxEnd)
	{
		ExpectBoxes({ { TEXT("fa-end-c"), 98.0f, 128.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(SpaceDistribution_FallsBackToStartOrCentre)
	{
		ExpectBoxes({ { TEXT("fa-between-c"), 8.0f, 188.0f, 10.0f, 10.0f }, { TEXT("fa-around-c"), 53.0f, 248.0f, 10.0f, 10.0f },
			{ TEXT("fa-evenly-c"), 53.0f, 308.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(ReverseDirections_StartAtTheOtherEdge)
	{
		ExpectBoxes({ { TEXT("fa-rrev-c"), 98.0f, 368.0f, 10.0f, 10.0f }, { TEXT("fa-crev-c"), 8.0f, 518.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(ColumnCentre_IsCentredOnTheVerticalAxis)
	{
		ExpectBoxes({ { TEXT("fa-col-c"), 8.0f, 443.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(CrossAxis_FollowsAlignSelfAndWrapReverse)
	{
		ExpectBoxes({ { TEXT("fa-self-c"), 8.0f, 578.0f, 10.0f, 10.0f }, { TEXT("fa-wrev-c"), 8.0f, 638.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(StaticFlexContainer_PlacesTheChildInItsOwnContentBox)
	{
		ExpectBoxes({ { TEXT("fa-static-c"), 58.0f, 748.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(Insets_OverrideTheStaticPosition)
	{
		ExpectBoxes({ { TEXT("fa-inset-c"), 4.0f, 794.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(BesideAnInFlowItem_IsPlacedAsASoleItem)
	{
		ExpectBoxes({ { TEXT("fm-centre-c"), 53.0f, 873.0f, 10.0f, 10.0f }, { TEXT("fm-end-c"), 98.0f, 918.0f, 10.0f, 10.0f },
			{ TEXT("fm-col-c"), 8.0f, 993.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(AutoHeightContainer_CentresInTheHeightOfItsItems)
	{
		ExpectBoxes({ { TEXT("fm-auto-c"), 8.0f, 1048.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(AlignItemsFlexEnd_IsAtTheCrossEnd)
	{
		ExpectBoxes({ { TEXT("fm-aiend-c"), 8.0f, 1118.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(BaselineUnderWrapReverse_StaysAtTheCrossStart)
	{
		ExpectBoxes({ { TEXT("fm-wrev-base-c"), 8.0f, 1148.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(AutoHeightContainerWithoutItems_CentresInItsMinHeight)
	{
		ExpectBoxes({ { TEXT("fe-auto-c"), 8.0f, 1203.0f, 10.0f, 10.0f }, { TEXT("fe-min-c"), 8.0f, 1243.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(ContainerWithoutItems_TakesItsMinAndMaxSizes)
	{
		ExpectBoxes({ { TEXT("fe-min"), 0.0f, 1220.0f, 116.0f, 56.0f }, { TEXT("fe-max"), 0.0f, 1280.0f, 116.0f, 56.0f },
			{ TEXT("fe-max-c"), 8.0f, 1303.0f, 10.0f, 10.0f }, { TEXT("fe-maxonly"), 0.0f, 1340.0f, 116.0f, 36.0f },
			{ TEXT("fe-maxonly-c"), 8.0f, 1353.0f, 10.0f, 10.0f }, { TEXT("fe-col"), 0.0f, 1380.0f, 116.0f, 56.0f },
			{ TEXT("fe-col-c"), 8.0f, 1403.0f, 10.0f, 10.0f }, { TEXT("fe-inline"), 0.0f, 1440.0f, 66.0f, 16.0f },
			{ TEXT("fe-inline-c"), 28.0f, 1448.0f, 10.0f, 10.0f } });
	}
};

// Boxes sized by min-content, max-content and fit-content: a 60 px and a 40 px inline-block separated by a space, 400 px wide containers
// 4 px apart, 60 px tall content for the block axis. Expected boxes come from Chrome on the SizingKeywordsTest fixture.
TEST_CLASS(ArcRmlCssParitySizingKeywordsTests, "ArcRmlUI.CssParity.SizingKeywords")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/SizingKeywordsTest.rml"),
			TEXT("SizingKeywords"), Boxes)));
	}

	TEST_METHOD(WidthKeywords_GiveTheIntrinsicWidths)
	{
		ExpectBoxes({ { TEXT("sk-min"), 0.0f, 0.0f, 60.0f, 42.0f }, { TEXT("sk-max"), 0.0f, 46.0f, 104.125f, 21.0f },
			{ TEXT("sk-fit"), 0.0f, 71.0f, 104.125f, 21.0f }, { TEXT("sk-fit-narrow"), 0.0f, 96.0f, 80.0f, 42.0f },
			{ TEXT("sk-inline-size"), 0.0f, 284.0f, 60.0f, 42.0f } });
	}

	TEST_METHOD(MinAndMaxWidthKeywords_BoundTheWidth)
	{
		ExpectBoxes({ { TEXT("sk-minw"), 0.0f, 142.0f, 60.0f, 42.0f }, { TEXT("sk-maxw"), 0.0f, 188.0f, 104.125f, 21.0f },
			{ TEXT("sk-maxw-min"), 0.0f, 213.0f, 60.0f, 42.0f } });
	}

	TEST_METHOD(KeywordWidth_IsAContentWidthUnderBorderBox)
	{
		ExpectBoxes({ { TEXT("sk-border"), 0.0f, 259.0f, 124.125f, 21.0f } });
	}

	TEST_METHOD(FitContentBetweenInsets_IsCentredByAutoMargins)
	{
		ExpectBoxes({ { TEXT("sk-abs"), 147.9375f, 330.0f, 104.125f, 21.0f } });
	}

	TEST_METHOD(MinContentChild_GivesItsWidthToAShrinkToFitParent)
	{
		ExpectBoxes({ { TEXT("sk-parent"), 0.0f, 374.0f, 60.0f, 42.0f }, { TEXT("sk-child"), 0.0f, 374.0f, 60.0f, 42.0f } });
	}

	TEST_METHOD(BlockAxisKeywords_AreTheContentHeight)
	{
		ExpectBoxes({ { TEXT("sk-h-fit"), 0.0f, 420.0f, 50.0f, 60.0f }, { TEXT("sk-h-min"), 0.0f, 484.0f, 50.0f, 60.0f },
			{ TEXT("sk-h-max"), 0.0f, 548.0f, 50.0f, 60.0f }, { TEXT("sk-h-minfit"), 0.0f, 612.0f, 50.0f, 100.0f } });
	}

	TEST_METHOD(FlexItems_UseTheKeywordsAndAreNotStretched)
	{
		ExpectBoxes({ { TEXT("sk-flex-min"), 0.0f, 716.0f, 60.0f, 42.0f }, { TEXT("sk-flex-col-fit"), 0.0f, 762.0f, 104.125f, 21.0f },
			{ TEXT("sk-flex-h-fit"), 0.0f, 787.0f, 0.0f, 60.0f }, { TEXT("sk-flex-minw"), 0.0f, 891.0f, 104.125f, 21.0f } });
	}

	TEST_METHOD(GridItem_UsesTheKeywordAndIsNotStretched)
	{
		ExpectBoxes({ { TEXT("sk-grid-min"), 0.0f, 916.0f, 60.0f, 42.0f } });
	}
};

// Sizing keywords in intrinsic contributions, automatic minimums, abspos insets, fit-content bounds and the block-axis crossovers. Expected
// boxes come from Chrome on the SizingKeywordsEdgesTest fixture.
TEST_CLASS(ArcRmlCssParitySizingKeywordEdgesTests, "ArcRmlUI.CssParity.SizingKeywordEdges")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/SizingKeywordsEdgesTest.rml"),
			TEXT("SizingKeywordsEdges"), Boxes)));
	}

	TEST_METHOD(MaxWidthFitContent_IsTheMaxContentWidthInAShrinkToFitContribution)
	{
		ExpectBoxes({ { TEXT("ke-flex-maxw-fit"), 0.0f, 0.0f, 104.125f, 21.0f }, { TEXT("ke-ib-maxw-fit"), 0.0f, 25.0f, 104.125f, 21.0f } });
	}

	TEST_METHOD(RowFlexItemWithAKeywordWidth_KeepsItsMinContentWidthAsItsAutomaticMinimum)
	{
		ExpectBoxes({ { TEXT("ke-flex-w-min"), 0.0f, 50.0f, 60.0f, 42.0f } });
	}

	TEST_METHOD(HeightFitContentBetweenInsets_IsTheContentHeightCentredByAutoMargins)
	{
		ExpectBoxes({ { TEXT("ke-abs-h-fit"), 0.0f, 166.0f, 400.0f, 60.0f } });
	}

	TEST_METHOD(FitContent_IsNeverBelowTheMinContentWidthAndLeavesOutMarginsAndPadding)
	{
		ExpectBoxes({ { TEXT("ke-fit-lower"), 0.0f, 300.0f, 60.0f, 42.0f }, { TEXT("ke-fit-margin"), 150.0f, 346.0f, 100.0f, 42.0f },
			{ TEXT("ke-fit-padding"), 0.0f, 392.0f, 400.0f, 42.0f } });
	}

	TEST_METHOD(FitContentWrappingFlexContainerInAnInlineBlock_FillsTheNarrowContainer)
	{
		ExpectBoxes({ { TEXT("ke-ib-parent"), 0.0f, 444.0f, 80.0f, 20.0f }, { TEXT("ke-ib-flex-fit"), 0.0f, 444.0f, 80.0f, 20.0f } });
	}

	TEST_METHOD(MinWidthFitContentOnAWrappingFlexContainer_IsTheMinContentWidthInAShrinkToFitContribution)
	{
		ExpectBoxes({ { TEXT("ke-ib-minw-parent"), 0.0f, 474.0f, 80.0f, 20.0f }, { TEXT("ke-ib-flex-minw-fit"), 0.0f, 474.0f, 80.0f, 20.0f } });
	}

	TEST_METHOD(ColumnFlexItemWithMaxWidthMinContent_IsMeasuredAtItsMaximumWidth)
	{
		ExpectBoxes({ { TEXT("ke-col-maxw-min"), 0.0f, 498.0f, 60.0f, 42.0f } });
	}

	TEST_METHOD(GridItemWithAHeightKeyword_IsNotStretchedToItsRow)
	{
		ExpectBoxes({ { TEXT("ke-grid-h-fit"), 0.0f, 544.0f, 300.0f, 60.0f } });
	}

	TEST_METHOD(BlockAxisKeywords_KeepADefiniteHeightThatLiesBetweenThem)
	{
		ExpectBoxes({ { TEXT("ke-h-short-minh-min"), 0.0f, 648.0f, 50.0f, 100.0f }, { TEXT("ke-h-tall-maxh-max"), 0.0f, 752.0f, 50.0f, 20.0f } });
	}
};

// The user-agent dialog, fieldset and legend rules. Expected boxes and colours come from Chrome on the UserAgentDialogTest (no preflight)
// and UserAgentFieldsetPreflightTest fixtures.
TEST_CLASS(ArcRmlCssParityUserAgentDialogTests, "ArcRmlUI.CssParity.UserAgentDialog")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(const TCHAR* Fixture, const TCHAR* DumpFolder, TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, Fixture, DumpFolder, Boxes)));
	}

	void ExpectColours(const TCHAR* DomId, const TCHAR* Color, const TCHAR* BackgroundColor, const TCHAR* BorderTopColor)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(AreEqual(FString(Color), Element->Color));
		ASSERT_THAT(AreEqual(FString(BackgroundColor), Element->BackgroundColor));
		ASSERT_THAT(AreEqual(FString(BorderTopColor), Element->BorderTopColor));
	}

	TEST_METHOD(OpenDialogWithMaxWidth_IsAFitContentBoxCentredBetweenItsInsets)
	{
		ExpectBoxes(TEXT("CssParity/UserAgent/UserAgentDialogTest.rml"), TEXT("UserAgentDialog"),
			{ { TEXT("ud-open"), 333.0f, 18.0f, 614.0f, 101.0f }, { TEXT("ud-short"), 599.5469f, 200.0f, 80.9062f, 59.0f },
				{ TEXT("ud-short-text"), 618.5469f, 219.0f, 42.9062f, 21.0f } });
		ExpectColours(TEXT("ud-open"), TEXT("#000000ff"), TEXT("#ffffffff"), TEXT("#000000ff"));
	}

	TEST_METHOD(DialogWithoutOpen_IsNotRendered)
	{
		ExpectBoxes(TEXT("CssParity/UserAgent/UserAgentDialogTest.rml"), TEXT("UserAgentDialog"), {});
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("ud-closed"));
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(IsFalse(Element->bVisible));
	}

	TEST_METHOD(FieldsetWithoutPreflight_HasTheUserAgentEdgesAndMinContentWidth)
	{
		ExpectBoxes(TEXT("CssParity/UserAgent/UserAgentDialogTest.rml"), TEXT("UserAgentDialog"),
			{ { TEXT("ud-fieldset"), 10.0f, 18.0f, 1260.0f, 39.5938f }, { TEXT("ud-fieldset-block"), 24.0f, 25.5938f, 300.0f, 20.0f },
				{ TEXT("ud-fieldset-min"), 10.0f, 57.5938f, 328.0f, 39.5938f }, { TEXT("ud-fieldset-min-block"), 24.0f, 65.1875f, 300.0f, 20.0f } });
		ExpectColours(TEXT("ud-fieldset"), TEXT("#000000ff"), TEXT("#00000000"), TEXT("#f0f0f0ff"));
	}

	TEST_METHOD(FieldsetUnderPreflight_KeepsItsChildrensMarginsAndFloatsInside)
	{
		ExpectBoxes(TEXT("CssParity/UserAgent/UserAgentFieldsetPreflightTest.rml"), TEXT("UserAgentFieldsetPreflight"),
			{ { TEXT("uf-fieldset"), 0.0f, 10.0f, 1280.0f, 40.0f }, { TEXT("uf-p"), 0.0f, 26.0f, 1280.0f, 24.0f },
				{ TEXT("uf-legend-fieldset"), 0.0f, 50.0f, 1280.0f, 48.0f }, { TEXT("uf-legend-next"), 0.0f, 74.0f, 1280.0f, 24.0f },
				{ TEXT("uf-float-fieldset"), 0.0f, 98.0f, 1280.0f, 40.0f }, { TEXT("uf-float"), 0.0f, 98.0f, 50.0f, 40.0f },
				{ TEXT("uf-after"), 0.0f, 138.0f, 1280.0f, 24.0f } });
	}
};

// place-content, place-items and place-self on 100 x 100 grid and flex containers 4 px apart holding 20 x 20 cells. Expected boxes come from
// Chrome on the PlaceTest fixture.
TEST_CLASS(ArcRmlCssParityPlaceTests, "ArcRmlUI.CssParity.Place")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/PlaceTest.rml"), TEXT("Place"), Boxes)));
	}

	TEST_METHOD(PlaceItemsWithOneValue_CentresTheItemOnBothAxes)
	{
		ExpectBoxes({ { TEXT("pl-items"), 40.0f, 40.0f, 20.0f, 20.0f } });
	}

	TEST_METHOD(PlaceContentWithTwoValues_AlignsTheTracksThenJustifiesThem)
	{
		ExpectBoxes({ { TEXT("pl-content"), 80.0f, 144.0f, 20.0f, 20.0f } });
	}

	TEST_METHOD(PlaceSelf_SetsBothSelfAlignments)
	{
		ExpectBoxes({ { TEXT("pl-self"), 80.0f, 288.0f, 20.0f, 20.0f } });
	}

	TEST_METHOD(PlaceItemsWithTwoValues_AlignsEndAndJustifiesCentre)
	{
		ExpectBoxes({ { TEXT("pl-items-two"), 40.0f, 392.0f, 20.0f, 20.0f } });
	}

	TEST_METHOD(PlaceContentOnAWrappingFlexContainer_CentresTheLineAndItsItems)
	{
		ExpectBoxes({ { TEXT("pl-flex-a"), 30.0f, 456.0f, 20.0f, 20.0f }, { TEXT("pl-flex-b"), 50.0f, 456.0f, 20.0f, 20.0f } });
	}
};

// Class selectors written with CSS hex escapes (Tailwind's .\32 xl\:p-4 for 2xl:p-4): 200 x 10 boxes 4 px apart, 20 x 10 children. Expected
// boxes come from Chrome on the HexEscapeTest fixture.
TEST_CLASS(ArcRmlCssParityHexEscapeTests, "ArcRmlUI.CssParity.HexEscape")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Selectors/HexEscapeTest.rml"), TEXT("HexEscape"),
			Boxes)));
	}

	TEST_METHOD(HexEscapeWithItsWhiteSpace_MatchesTheDecodedClass)
	{
		ExpectBoxes({ { TEXT("he-2xl"), 0.0f, 0.0f, 232.0f, 42.0f }, { TEXT("he-10"), 0.0f, 46.0f, 100.0f, 10.0f } });
	}

	TEST_METHOD(WhiteSpaceAfterTheEscapesWhiteSpace_IsADescendantCombinator)
	{
		ExpectBoxes({ { TEXT("he-10-inner"), 0.0f, 60.0f, 30.0f, 10.0f }, { TEXT("he-1-inner"), 0.0f, 74.0f, 50.0f, 10.0f } });
	}

	TEST_METHOD(HexEscapeOfAnAtSign_MatchesTheClass)
	{
		ExpectBoxes({ { TEXT("he-at"), 0.0f, 88.0f, 60.0f, 10.0f } });
	}
};

// calc() constants e, pi, infinity and -infinity in widths of 10 px high boxes 4 px apart. Expected boxes come from Chrome on the
// CalcConstantsTest fixture.
TEST_CLASS(ArcRmlCssParityCalcConstantsTests, "ArcRmlUI.CssParity.CalcConstants")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/CalcProps/CalcConstantsTest.rml"),
			TEXT("CalcConstants"), Boxes)));
	}

	TEST_METHOD(PiAndE_AreNumbers)
	{
		ExpectBoxes({ { TEXT("cc-pi"), 0.0f, 0.0f, 31.4062f, 10.0f }, { TEXT("cc-e"), 0.0f, 14.0f, 27.1719f, 10.0f },
			{ TEXT("cc-upper"), 0.0f, 56.0f, 62.8281f, 10.0f } });
	}

	TEST_METHOD(Infinities_AreBoundedByMinAndMax)
	{
		ExpectBoxes({ { TEXT("cc-min-inf"), 0.0f, 28.0f, 100.0f, 10.0f }, { TEXT("cc-max-neg-inf"), 0.0f, 42.0f, 20.0f, 10.0f } });
	}
};

// The small, large and dynamic viewport units and vmin / vmax in the 1280 x 720 test view, in widths of 10 px high boxes 4 px apart
// (one 5dvh high), plain and in calc(). Expected boxes come from Chrome on the ViewportUnitsTest fixture.
TEST_CLASS(ArcRmlCssParityViewportUnitsTests, "ArcRmlUI.CssParity.ViewportUnits")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/ViewportUnitsTest.rml"),
			TEXT("ViewportUnits"), Boxes)));
	}

	TEST_METHOD(SmallLargeAndDynamicWidthUnits_AreTheViewportWidth)
	{
		ExpectBoxes({ { TEXT("vu-dvw"), 0.0f, 0.0f, 128.0f, 10.0f }, { TEXT("vu-svw"), 0.0f, 14.0f, 128.0f, 10.0f },
			{ TEXT("vu-lvw"), 0.0f, 28.0f, 128.0f, 10.0f } });
	}

	TEST_METHOD(SmallLargeAndDynamicHeightUnits_AreTheViewportHeight)
	{
		ExpectBoxes({ { TEXT("vu-dvh"), 0.0f, 42.0f, 72.0f, 10.0f }, { TEXT("vu-svh"), 0.0f, 56.0f, 72.0f, 10.0f },
			{ TEXT("vu-lvh"), 0.0f, 70.0f, 72.0f, 10.0f }, { TEXT("vu-height"), 0.0f, 154.0f, 10.0f, 36.0f } });
	}

	TEST_METHOD(VminAndVmax_AreTheSmallerAndLargerSide)
	{
		ExpectBoxes({ { TEXT("vu-vmin"), 0.0f, 84.0f, 72.0f, 10.0f }, { TEXT("vu-vmax"), 0.0f, 98.0f, 128.0f, 10.0f },
			{ TEXT("vu-upper"), 0.0f, 194.0f, 72.0f, 10.0f } });
	}

	// In a portrait view vmin is the width and vmax the height, the reverse of the landscape test view; the fixture opens landscape and is
	// resized, so the viewport-unit properties must be recomputed. Expected boxes come from Chrome on the fixture in a 600 x 900 window.
	TEST_METHOD(VminAndVmaxAfterAResizeToPortrait_SwapSides)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Layout/ViewportUnitsTest.rml"),
			TEXT("ViewportUnitsPortrait"), { { TEXT("vu-vmin"), 0.0f, 84.0f, 60.0f, 10.0f }, { TEXT("vu-vmax"), 0.0f, 98.0f, 90.0f, 10.0f },
			{ TEXT("vu-calc-mixed"), 0.0f, 140.0f, 75.0f, 10.0f }, { TEXT("vu-height"), 0.0f, 154.0f, 10.0f, 45.0f },
			{ TEXT("vu-upper"), 0.0f, 203.0f, 60.0f, 10.0f } }, FIntPoint(600, 900))));
	}

	TEST_METHOD(ViewportUnitsInCalc_Resolve)
	{
		ExpectBoxes({ { TEXT("vu-calc-vmin"), 0.0f, 112.0f, 82.0f, 10.0f }, { TEXT("vu-calc-dvh"), 0.0f, 126.0f, 140.0f, 10.0f },
			{ TEXT("vu-calc-mixed"), 0.0f, 140.0f, 100.0f, 10.0f } });
	}
};

// overflow-wrap and its word-wrap alias on a run of 'a' (9.2 px each) in 400 px containers 4 px apart, 20 px lines. Expected boxes come
// from Chrome on the OverflowWrapTest fixture.
TEST_CLASS(ArcRmlCssParityOverflowWrapTests, "ArcRmlUI.CssParity.OverflowWrap")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Text/OverflowWrapTest.rml"),
			TEXT("OverflowWrap"), Boxes)));
	}

	TEST_METHOD(AnywhereInAMinContentBox_IsOneCharacterWide)
	{
		ExpectBoxes({ { TEXT("ow-anywhere-min"), 0.0f, 0.0f, 9.2031f, 160.0f } });
	}

	// Guard: break-word adds no break to the min-content width; word-break: break-word breaks it, as before the change.
	TEST_METHOD(BreakWordInAMinContentBox_KeepsTheWord)
	{
		ExpectBoxes({ { TEXT("ow-break-word-min"), 0.0f, 164.0f, 73.6094f, 20.0f }, { TEXT("ow-word-break-min"), 0.0f, 188.0f, 9.2031f, 160.0f } });
	}

	TEST_METHOD(BreakWordAndItsAliasInANarrowBox_BreakTheWord)
	{
		ExpectBoxes({ { TEXT("ow-break-word-narrow"), 0.0f, 352.0f, 40.0f, 60.0f }, { TEXT("ow-word-wrap-narrow"), 0.0f, 416.0f, 40.0f, 60.0f },
			{ TEXT("ow-anywhere-narrow"), 0.0f, 480.0f, 40.0f, 60.0f } });
	}

	// Guard: without overflow-wrap the word overflows its line.
	TEST_METHOD(NormalInANarrowBox_KeepsTheWordOnOneLine)
	{
		ExpectBoxes({ { TEXT("ow-normal-narrow"), 0.0f, 544.0f, 40.0f, 20.0f } });
	}

	// 'aa aaaaaaaaa' in 40 px (four 'a' per line): moving the nine-letter word to its own line first gives four lines, breaking it at
	// once after 'aa ' gives three.
	TEST_METHOD(BreakWordAfterAShortWord_MovesTheLongWordToItsOwnLineFirst)
	{
		ExpectBoxes({ { TEXT("ow-break-word-late"), 0.0f, 568.0f, 40.0f, 80.0f } });
	}

	TEST_METHOD(FlexItems_TakeAnywhereButNotBreakWordInTheirAutomaticMinimum)
	{
		ExpectBoxes({ { TEXT("ow-anywhere-flex"), 0.0f, 652.0f, 40.0f, 60.0f }, { TEXT("ow-break-word-flex"), 0.0f, 716.0f, 110.4062f, 20.0f } });
	}
};

namespace ArcRmlCssParityLayoutTestsPrivate
{
	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	// Opens the fixture, dumps it for chrome_layout_compare.py and renders it; returns the error, or an empty string.
	FString OpenAndRenderPixels(FArcRmlTestView& View, const TCHAR* Fixture, const TCHAR* DumpFolder)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(Fixture);
		FArcRmlTestView::DropCompiledDocument(Path);
		if (!(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled()))
		{
			return ArcRmlUITestHelpers::DescribeDiagnostics(View);
		}
		View.DumpLayout(FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("ArcRmlUITest"), DumpFolder)));
		if (!(View.Flush() && View.Flush() && View.RenderPixels()))
		{
			return TEXT("pixels not rendered");
		}
		return FString();
	}

	// Opens the fixture, dumps it for chrome_layout_compare.py, renders it and lists every pixel more than 2 per channel from Chrome's.
	FString CheckChromePixels(FArcRmlTestView& View, const TCHAR* Fixture, const TCHAR* DumpFolder, TConstArrayView<FChromePixel> Pixels)
	{
		const FString Error = OpenAndRenderPixels(View, Fixture, DumpFolder);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		FString Mismatches;
		for (const FChromePixel& Pixel : Pixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (FMath::Abs(Measured.R - Pixel.Chrome.R) > 2 || FMath::Abs(Measured.G - Pixel.Chrome.G) > 2 || FMath::Abs(Measured.B - Pixel.Chrome.B) > 2
				|| FMath::Abs(Measured.A - Pixel.Chrome.A) > 2)
			{
				Mismatches += FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d,%d, Chrome %d,%d,%d,%d; "), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G,
					Measured.B, Measured.A, Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B, Pixel.Chrome.A);
			}
		}
		return Mismatches;
	}
}

// Text decoration lines on 'xxxx' in 16 px Space Grotesk with 20 px lines, rows 30 px apart (the 32 px row 60 px). Expected pixels come from a
// Chrome screenshot of the TextDecorationTest fixture.
TEST_CLASS(ArcRmlCssParityTextDecorationPixelTests, "ArcRmlUI.Pixels.TextDecoration")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectPixels(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromePixel> Pixels)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/Text/TextDecorationTest.rml"),
			TEXT("TextDecoration"), Pixels)));
	}

	TEST_METHOD(UnderlinedLink_DrawsItsLineInItsOwnColourUnderAChildWithAnotherColour)
	{
		ExpectPixels({ { TEXT("above the link underline"), 20, 15, FColor(255, 255, 255, 255) }, { TEXT("link underline"), 20, 16, FColor(32, 80, 208, 255) },
			{ TEXT("below the link underline"), 20, 17, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(NoUnderlineOnADescendant_KeepsTheDecoratingBoxsLine)
	{
		ExpectPixels({ { TEXT("paragraph underline"), 20, 46, FColor(208, 32, 32, 255) }, { TEXT("below it"), 20, 47, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(OffsetAndThickness_MoveAndWidenTheLine)
	{
		ExpectPixels({ { TEXT("above the offset line"), 20, 78, FColor(255, 255, 255, 255) }, { TEXT("offset line first row"), 20, 79, FColor(32, 160, 64, 255) },
			{ TEXT("offset line second row"), 20, 80, FColor(32, 160, 64, 255) }, { TEXT("below the offset line"), 20, 81, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(AutoThicknessAt32px_IsThreeRowsTwoBelowTheBaseline)
	{
		ExpectPixels({ { TEXT("above the line"), 20, 122, FColor(255, 255, 255, 255) }, { TEXT("first row"), 20, 123, FColor(16, 128, 128, 255) },
			{ TEXT("third row"), 20, 125, FColor(16, 128, 128, 255) }, { TEXT("below the line"), 20, 126, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(DecorationColour_ColoursTheLineAndNotTheText)
	{
		ExpectPixels({ { TEXT("orange underline"), 20, 166, FColor(224, 128, 16, 255) } });
	}

	TEST_METHOD(AtomicInlinesAndAbsoluteBoxes_AreNotDecoratedByTheirAncestor)
	{
		ExpectPixels({ { TEXT("text before the inline-block"), 20, 196, FColor(128, 32, 192, 255) },
			{ TEXT("inline-block text"), 45, 196, FColor(255, 255, 255, 255) }, { TEXT("in-flow text"), 20, 226, FColor(128, 80, 32, 255) },
			{ TEXT("absolutely positioned text"), 105, 226, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(LineThroughAndOverline_SitAtChromesPositions)
	{
		ExpectPixels({ { TEXT("above the line-through between glyphs"), 19, 248, FColor(255, 255, 255, 255) },
			{ TEXT("line-through between glyphs"), 19, 249, FColor(16, 32, 128, 255) }, { TEXT("line-through"), 20, 249, FColor(16, 32, 128, 255) },
			{ TEXT("below the line-through between glyphs"), 19, 250, FColor(255, 255, 255, 255) }, { TEXT("above the overline"), 20, 267, FColor(255, 255, 255, 255) },
			{ TEXT("overline"), 20, 268, FColor(112, 112, 16, 255) }, { TEXT("below the overline"), 20, 269, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(Shorthand_SetsLineThicknessAndColour)
	{
		ExpectPixels({ { TEXT("above the line"), 20, 316, FColor(255, 255, 255, 255) }, { TEXT("first row"), 20, 317, FColor(16, 160, 160, 255) },
			{ TEXT("third row"), 20, 319, FColor(16, 160, 160, 255) }, { TEXT("below the line"), 20, 320, FColor(255, 255, 255, 255) } });
	}

	TEST_METHOD(BlockChild_IsDecoratedByItsParent)
	{
		ExpectPixels({ { TEXT("block child underline"), 20, 346, FColor(128, 16, 48, 255) } });
	}
};

// border-radius: calc(infinity * 1px) (Tailwind rounded-full) on a 100 x 24 and a 40 x 40 box. Expected pixels come from a Chrome screenshot of
// the RoundedFullTest fixture.
TEST_CLASS(ArcRmlCssParityRoundedFullPixelTests, "ArcRmlUI.Pixels.RoundedFull")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectPixels(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromePixel> Pixels)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/Pixels/RoundedFullTest.rml"),
			TEXT("RoundedFull"), Pixels)));
	}

	TEST_METHOD(InfiniteRadiusOnAWideBox_IsAPill)
	{
		ExpectPixels({ { TEXT("top-left corner"), 11, 11, FColor(255, 255, 255, 255) }, { TEXT("left cap centre"), 22, 22, FColor(51, 102, 204, 255) },
			{ TEXT("left cap edge"), 11, 22, FColor(51, 102, 204, 255) }, { TEXT("top edge"), 60, 11, FColor(51, 102, 204, 255) },
			{ TEXT("top-right corner"), 108, 11, FColor(255, 255, 255, 255) }, { TEXT("bottom-right corner"), 108, 32, FColor(255, 255, 255, 255) },
			{ TEXT("bottom edge"), 60, 32, FColor(51, 102, 204, 255) } });
	}

	TEST_METHOD(InfiniteRadiusOnASquare_IsACircle)
	{
		ExpectPixels({ { TEXT("top-left corner"), 11, 45, FColor(255, 255, 255, 255) }, { TEXT("near the top-left corner"), 13, 47, FColor(255, 255, 255, 255) },
			{ TEXT("just outside the circle at 45 degrees"), 14, 48, FColor(255, 255, 255, 255) },
			{ TEXT("just inside the circle at 45 degrees"), 16, 50, FColor(204, 51, 102, 255) },
			{ TEXT("bottom-right corner"), 47, 81, FColor(255, 255, 255, 255) }, { TEXT("centre"), 30, 64, FColor(204, 51, 102, 255) },
			{ TEXT("top"), 30, 45, FColor(204, 51, 102, 255) }, { TEXT("left"), 11, 64, FColor(204, 51, 102, 255) } });
	}
};

// Tables in the collapsing border model, the separated model's 'border-spacing', header and footer groups and a sizing keyword on
// a cell; cells are 40 x 20 boxes with 10 px 8 px padding. Expected boxes come from Chrome on the BorderCollapseTest fixture.
TEST_CLASS(ArcRmlCssParityBorderCollapseTests, "ArcRmlUI.CssParity.BorderCollapse")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		// Half-pixel collapsed edges must match exactly; only the keyword cells' text widths differ from Chrome, by 0.015 px.
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Table/BorderCollapseTest.rml"),
			TEXT("BorderCollapse"), Boxes, FIntPoint::ZeroValue, 0.02f)));
	}

	TEST_METHOD(PreflightHeaderAndRowBorders_GiveTheFirstBodyRowHalfOfEachEdge)
	{
		ExpectBoxes({ { TEXT("pf"), 0.0f, 0.0f, 112.0f, 165.0f }, { TEXT("pf-h"), 0.0f, 0.0f, 112.0f, 41.0f }, { TEXT("pf-h1"), 0.0f, 0.0f, 56.0f, 41.0f },
			{ TEXT("pf-b"), 0.0f, 41.0f, 112.0f, 123.5f }, { TEXT("pf-r1"), 0.0f, 41.0f, 112.0f, 41.5f }, { TEXT("pf-r1a"), 0.0f, 41.0f, 56.0f, 41.5f },
			{ TEXT("pf-r2"), 0.0f, 82.5f, 112.0f, 41.0f }, { TEXT("pf-r2b"), 56.0f, 82.5f, 56.0f, 41.0f }, { TEXT("pf-r3"), 0.0f, 123.5f, 112.0f, 41.0f } });
	}

	TEST_METHOD(OnePixelBordersOnEveryCell_ShareOneLine)
	{
		ExpectBoxes({ { TEXT("all"), 0.0f, 173.0f, 115.0f, 83.0f }, { TEXT("all-a"), 0.5f, 173.5f, 57.0f, 41.0f }, { TEXT("all-b"), 57.5f, 173.5f, 57.0f, 41.0f },
			{ TEXT("all-c"), 0.5f, 214.5f, 57.0f, 41.0f }, { TEXT("all-d"), 57.5f, 214.5f, 57.0f, 41.0f } });
	}

	TEST_METHOD(WiderBorder_WinsTheSharedEdge)
	{
		ExpectBoxes({ { TEXT("wide"), 0.0f, 264.0f, 119.0f, 46.0f }, { TEXT("wide-a"), 1.5f, 265.5f, 59.0f, 43.0f },
			{ TEXT("wide-b"), 60.5f, 265.5f, 58.0f, 43.0f } });
	}

	TEST_METHOD(DashedAndSolidBordersOfOneWidth_ShareOneEdge)
	{
		ExpectBoxes({ { TEXT("dash"), 0.0f, 318.0f, 118.0f, 44.0f }, { TEXT("dash-a"), 1.0f, 319.0f, 58.0f, 42.0f }, { TEXT("dash-b"), 59.0f, 319.0f, 58.0f, 42.0f } });
	}

	TEST_METHOD(RowBorderInTheSeparatedModel_TakesNoSpace)
	{
		ExpectBoxes({ { TEXT("sep"), 0.0f, 370.0f, 112.0f, 80.0f }, { TEXT("sep-r1"), 0.0f, 370.0f, 112.0f, 40.0f }, { TEXT("sep-a"), 0.0f, 370.0f, 56.0f, 40.0f },
			{ TEXT("sep-r2"), 0.0f, 410.0f, 112.0f, 40.0f }, { TEXT("sep-d"), 56.0f, 410.0f, 56.0f, 40.0f } });
	}

	TEST_METHOD(BorderSpacing_SeparatesTheCellsAndThePaddingEdge)
	{
		ExpectBoxes({ { TEXT("spc"), 0.0f, 458.0f, 132.0f, 94.0f }, { TEXT("spc-a"), 8.0f, 464.0f, 56.0f, 40.0f }, { TEXT("spc-b"), 68.0f, 464.0f, 56.0f, 40.0f },
			{ TEXT("spc-c"), 8.0f, 506.0f, 56.0f, 40.0f }, { TEXT("spc-d"), 68.0f, 506.0f, 56.0f, 40.0f } });
	}

	TEST_METHOD(FooterBeforeTheBodyAndHeaderAfterIt_AreLaidOutLastAndFirst)
	{
		ExpectBoxes({ { TEXT("ft"), 0.0f, 560.0f, 56.0f, 120.0f }, { TEXT("ft-h"), 0.0f, 560.0f, 56.0f, 40.0f }, { TEXT("ft-b"), 0.0f, 600.0f, 56.0f, 40.0f },
			{ TEXT("ft-f"), 0.0f, 640.0f, 56.0f, 40.0f }, { TEXT("ft-f1"), 0.0f, 640.0f, 56.0f, 40.0f } });
	}

	TEST_METHOD(UserAgentTable_Has2pxBorderSpacing)
	{
		ExpectBoxes({ { TEXT("ua"), 0.0f, 688.0f, 118.0f, 44.0f }, { TEXT("ua-a"), 2.0f, 690.0f, 56.0f, 40.0f }, { TEXT("ua-b"), 60.0f, 690.0f, 56.0f, 40.0f } });
	}

	TEST_METHOD(FitContentWidthOnACell_IsSizedAsAuto)
	{
		ExpectBoxes({ { TEXT("kw-t"), 0.0f, 740.0f, 200.0f, 84.0f }, { TEXT("kw-fit"), 0.0f, 740.0f, 138.609375f, 84.0f },
			{ TEXT("kw-other"), 138.609375f, 740.0f, 61.390625f, 84.0f }, { TEXT("kw-auto"), 0.0f, 832.0f, 138.609375f, 84.0f } });
	}
};

// Collapsed borders drawn once per edge, the winner's colour on a shared edge, and the separated model's spacing and undrawn row
// borders. Expected pixels come from a Chrome screenshot of the BorderCollapseTest fixture.
TEST_CLASS(ArcRmlCssParityBorderCollapsePixelTests, "ArcRmlUI.Pixels.BorderCollapse")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectPixels(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromePixel> Pixels)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/Table/BorderCollapseTest.rml"),
			TEXT("BorderCollapse"), Pixels)));
	}

	TEST_METHOD(PreflightHeaderAndRowBorders_SitOnTheirGridLines)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Header(32, 64, 128, 255);
		const FColor Row(160, 48, 48, 255);
		ExpectPixels({ { TEXT("above the header border"), 20, 39, White }, { TEXT("header border top"), 20, 40, Header },
			{ TEXT("header border bottom"), 20, 41, Header }, { TEXT("below the header border"), 20, 42, White },
			{ TEXT("above the first row border"), 20, 81, White }, { TEXT("first row border"), 20, 82, Row },
			{ TEXT("below the first row border"), 20, 83, White }, { TEXT("second row border"), 20, 123, Row },
			{ TEXT("last row border"), 20, 164, Row }, { TEXT("below the table"), 20, 165, White } });
	}

	TEST_METHOD(OnePixelBordersOnEveryCell_AreOneLine)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Green(32, 128, 64, 255);
		ExpectPixels({ { TEXT("left edge"), 0, 190, Green }, { TEXT("inside the first cell"), 1, 190, White },
			{ TEXT("before the middle edge"), 56, 190, White }, { TEXT("middle edge"), 57, 190, Green }, { TEXT("after the middle edge"), 58, 190, White },
			{ TEXT("right edge"), 114, 190, Green }, { TEXT("right of the table"), 115, 190, White }, { TEXT("top edge"), 30, 173, Green },
			{ TEXT("below the top edge"), 30, 174, White }, { TEXT("edge between the rows"), 30, 214, Green }, { TEXT("below it"), 30, 215, White },
			{ TEXT("bottom edge"), 30, 255, Green } });
	}

	TEST_METHOD(WiderBorder_IsDrawnOnTheSharedEdge)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Wide(96, 32, 128, 255);
		const FColor Thin(128, 96, 32, 255);
		ExpectPixels({ { TEXT("wide left edge"), 0, 285, Wide }, { TEXT("wide left edge inner pixel"), 2, 285, Wide },
			{ TEXT("inside the wide cell"), 3, 285, White }, { TEXT("before the shared edge"), 58, 285, White }, { TEXT("shared edge start"), 59, 285, Wide },
			{ TEXT("shared edge centre"), 60, 285, Wide }, { TEXT("shared edge end"), 61, 285, Wide }, { TEXT("after the shared edge"), 62, 285, White }, { TEXT("thin right edge"), 118, 285, Thin },
			{ TEXT("inside the thin cell"), 117, 285, White }, { TEXT("wide top edge"), 30, 264, Wide }, { TEXT("wide top edge inner pixel"), 30, 266, Wide },
			{ TEXT("below the wide top edge"), 30, 267, White }, { TEXT("above the thin top edge"), 90, 264, White },
			{ TEXT("thin top edge"), 90, 265, Thin }, { TEXT("below the thin top edge"), 90, 266, White } });
	}

	TEST_METHOD(SolidBorder_WinsOverADashedOneOfTheSameWidth)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Solid(32, 96, 160, 255);
		ExpectPixels({ { TEXT("before the shared edge"), 57, 340, White }, { TEXT("shared edge left pixel"), 58, 340, Solid },
			{ TEXT("shared edge right pixel"), 59, 340, Solid }, { TEXT("after the shared edge"), 60, 340, White },
			{ TEXT("solid top edge"), 90, 318, Solid }, { TEXT("below the solid top edge"), 90, 320, White } });
	}

	TEST_METHOD(RowBorderInTheSeparatedModel_IsNotDrawn)
	{
		const FColor White(255, 255, 255, 255);
		ExpectPixels({ { TEXT("last pixel row of the first row, left cell"), 30, 409, White },
			{ TEXT("last pixel row of the first row, right cell"), 90, 409, White }, { TEXT("first pixel row of the second row"), 30, 410, White },
			{ TEXT("last pixel row of the second row"), 30, 449, White } });
	}

	TEST_METHOD(BorderSpacing_LeavesGapsBetweenTheCellsAndThePaddingEdge)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Border(64, 64, 64, 255);
		const FColor Cell(208, 224, 208, 255);
		ExpectPixels({ { TEXT("table left border"), 0, 480, Border }, { TEXT("padding"), 3, 480, White }, { TEXT("left spacing"), 7, 480, White },
			{ TEXT("first cell"), 8, 480, Cell }, { TEXT("first cell right pixel"), 63, 480, Cell }, { TEXT("spacing between the columns"), 64, 480, White },
			{ TEXT("spacing between the columns end"), 67, 480, White }, { TEXT("second cell"), 68, 480, Cell }, { TEXT("right spacing"), 124, 480, White },
			{ TEXT("right padding"), 128, 480, White }, { TEXT("table right border"), 131, 480, Border }, { TEXT("table top border"), 30, 458, Border },
			{ TEXT("top spacing"), 30, 463, White }, { TEXT("first row"), 30, 464, Cell }, { TEXT("spacing between the rows"), 30, 504, White },
			{ TEXT("spacing between the rows end"), 30, 505, White }, { TEXT("second row"), 30, 506, Cell }, { TEXT("bottom spacing"), 30, 546, White },
			{ TEXT("table bottom border"), 30, 551, Border } });
	}
};

// '-webkit-line-clamp' on a vertical '-webkit-box': 200 px wide paragraphs of 16 px Space Grotesk with 20 px lines, 10 px apart. Expected
// boxes come from Chrome on the LineClampTest fixture.
TEST_CLASS(ArcRmlCssParityLineClampTests, "ArcRmlUI.CssParity.LineClamp")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Text/LineClampTest.rml"),
			TEXT("LineClamp"), Boxes)));
	}

	TEST_METHOD(ClampTwoOnSevenLines_EndsTheBoxAfterTheSecondLine)
	{
		ExpectBoxes({ { TEXT("lc-two"), 0.0f, 0.0f, 200.0f, 40.0f } });
	}

	TEST_METHOD(ClampOneOnFourLines_EndsTheBoxAfterTheFirstLine)
	{
		ExpectBoxes({ { TEXT("lc-one"), 0.0f, 50.0f, 200.0f, 20.0f } });
	}

	TEST_METHOD(ClampTwoOnTwoLines_KeepsBothLines)
	{
		ExpectBoxes({ { TEXT("lc-fits"), 0.0f, 80.0f, 200.0f, 40.0f } });
	}

	TEST_METHOD(BoxWithoutVerticalOrient_IsNotClamped)
	{
		ExpectBoxes({ { TEXT("lc-horizontal"), 0.0f, 130.0f, 200.0f, 80.0f }, { TEXT("lc-after"), 0.0f, 220.0f, 1280.0f, 20.0f } });
	}
};

namespace ArcRmlCssParityLayoutTestsPrivate
{
	// Chrome anti-aliases glyph edges per LCD subpixel and the fork in grey, so an ellipsis is checked by its ink: the coverage of the
	// text colour in the region (chrome_layout_compare.py coverage) within 25 %, and the darkness-weighted centre x (255 minus the RGB mean
	// per pixel) within 0.35 px, which tells a 1 px shift apart. Returns the measured and expected values; bOutMatches is the verdict.
	FString MeasureEllipsis(FArcRmlTestView& View, const TCHAR* Name, const FIntRect& Region, const FColor& Colour, double ChromeCoverage,
		double ChromeCentreX, bool& bOutMatches)
	{
		const double Coverage = View.SumCoverage(Region, Colour);
		double Darkness = 0.0;
		double WeightedX = 0.0;
		for (int32 Y = Region.Min.Y; Y < Region.Max.Y; ++Y)
		{
			for (int32 X = Region.Min.X; X < Region.Max.X; ++X)
			{
				const FColor Pixel = View.GetPixel(X, Y);
				const double Dark = 255.0 - (double(Pixel.R) + Pixel.G + Pixel.B) / 3.0;
				Darkness += Dark;
				WeightedX += Dark * (X + 0.5);
			}
		}
		const double CentreX = Darkness > 0.0 ? WeightedX / Darkness : 0.0;
		bOutMatches = FMath::Abs(Coverage - ChromeCoverage) <= ChromeCoverage * 0.25 && FMath::Abs(CentreX - ChromeCentreX) <= 0.35;
		return FString::Printf(TEXT("%s: coverage %.3f, centre x %.3f; Chrome %.3f, %.3f"), Name, Coverage, CentreX, ChromeCoverage, ChromeCentreX);
	}
}

// The ellipsis that ends the last kept line. Expected pixels come from a Chrome screenshot of the LineClampTest fixture.
TEST_CLASS(ArcRmlCssParityLineClampPixelTests, "ArcRmlUI.Pixels.LineClamp")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	// 'amet consectetur' ends at x 139.4 and the ellipsis fits after it whole; checked by its ink (MeasureEllipsis).
	void ExpectEllipsis(const TCHAR* Name, const FIntRect& Region, const FColor& Colour, double ChromeCoverage, double ChromeCentreX)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::OpenAndRenderPixels(View, TEXT("CssParity/Text/LineClampTest.rml"),
			TEXT("LineClamp"))));
		bool bMatches = false;
		const FString Measured = ArcRmlCssParityLayoutTestsPrivate::MeasureEllipsis(View, Name, Region, Colour, ChromeCoverage, ChromeCentreX,
			bMatches);
		TestRunner->AddInfo(Measured);
		ASSERT_THAT(IsTrue(bMatches, Measured));
	}

	TEST_METHOD(ClampTwo_EndsTheSecondLineWithAnEllipsisAfterItsLastWord)
	{
		ExpectEllipsis(TEXT("ellipsis after 'consectetur'"), FIntRect(137, 30, 152, 38), FColor(32, 80, 208, 255), 11.114, 145.007);
	}

	TEST_METHOD(ClampOne_EndsTheFirstLineWithAnEllipsis)
	{
		ExpectEllipsis(TEXT("ellipsis after 'sit'"), FIntRect(165, 60, 179, 68), FColor(32, 160, 64, 255), 11.356, 171.251);
	}

	// The third line would draw its glyphs' tops in the 10 px between the clamped box and the next paragraph; Chrome's coverage there is 0.
	TEST_METHOD(ClampTwo_ClipsTheLinesAfterTheSecond)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::OpenAndRenderPixels(View, TEXT("CssParity/Text/LineClampTest.rml"),
			TEXT("LineClamp"))));
		const double Coverage = View.SumCoverage(FIntRect(0, 40, 200, 50), FColor(32, 80, 208, 255));
		ASSERT_THAT(IsTrue(Coverage < 0.5, FString::Printf(TEXT("blue coverage below the clamped box %.3f, Chrome 0"), Coverage)));
	}

	// 'Lorem ipsum dolor sit amet.' fills both kept lines with nothing after, so no ellipsis: 'amet.' ends at x 43.5 and Chrome's coverage of
	// the dots' band right of it is 0 (14.6 over the band of 'amet.' itself, left of it).
	TEST_METHOD(TextThatFits_HasNoEllipsis)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::OpenAndRenderPixels(View, TEXT("CssParity/Text/LineClampTest.rml"),
			TEXT("LineClamp"))));
		const double Coverage = View.SumCoverage(FIntRect(44, 110, 60, 118), FColor(208, 32, 32, 255));
		ASSERT_THAT(IsTrue(Coverage < 0.5, FString::Printf(TEXT("red coverage after 'amet.' %.3f, Chrome 0"), Coverage)));
	}
};

// 'text-overflow: ellipsis' on a 152 px 'nowrap' line (Tailwind 'truncate'). Expected boxes are Chrome's on the TextOverflowCutTest fixture.
TEST_CLASS(ArcRmlCssParityTextOverflowCutTests, "ArcRmlUI.CssParity.TextOverflowCut")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(TruncatedLine_KeepsItsBox)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Text/TextOverflowCutTest.rml"),
			TEXT("TextOverflowCut"), { { TEXT("to-cut"), 0.0f, 0.0f, 152.0f, 20.0f }, { TEXT("to-fits"), 0.0f, 30.0f, 152.0f, 20.0f },
				{ TEXT("to-after"), 0.0f, 60.0f, 1280.0f, 20.0f } })));
	}

	// The ellipsis is paint only: the line still holds all its laid-out text, as Chrome measures it (chrome_layout_compare.py --lines).
	TEST_METHOD(TruncatedLine_KeepsItsLaidOutText)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Text/TextOverflowCutTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.ParseLayout(Elements)));
		const TArray<FArcRmlTestTextLine> Cut = ArcRmlUITestHelpers::GetLines(Elements, TEXT("to-cut"));
		ASSERT_THAT(AreEqual(1, Cut.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("amet consectetur adipisicing elit")), Cut[0].Text));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Cut[0].Width, 255.656f, 0.05f), FString::Printf(TEXT("to-cut line is %.3f px, Chrome 255.656"), Cut[0].Width)));
		const TArray<FArcRmlTestTextLine> Fits = ArcRmlUITestHelpers::GetLines(Elements, TEXT("to-fits"));
		ASSERT_THAT(AreEqual(1, Fits.Num()));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Fits[0].Width, 139.422f, 0.05f), FString::Printf(TEXT("to-fits line is %.3f px, Chrome 139.422"), Fits[0].Width)));
	}
};

// The 'text-overflow' ellipsis. Chrome cuts 'amet consectetur adipisicing elit' after the 'r' (139.4 px) and draws the ellipsis
// (10.9 px) after it unkerned, as for '-webkit-line-clamp'; shaped together with the 'r' it sat 1 px further left. Expected values come
// from a Chrome screenshot of the TextOverflowCutTest fixture.
TEST_CLASS(ArcRmlCssParityTextOverflowCutPixelTests, "ArcRmlUI.Pixels.TextOverflowCut")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(CutAfterR_PlacesTheEllipsisAfterTheText)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::OpenAndRenderPixels(View, TEXT("CssParity/Text/TextOverflowCutTest.rml"),
			TEXT("TextOverflowCut"))));
		bool bMatches = false;
		const FString Measured = ArcRmlCssParityLayoutTestsPrivate::MeasureEllipsis(View, TEXT("text-overflow ellipsis after 'consectetur'"),
			FIntRect(137, 10, 152, 18), FColor(32, 80, 208, 255), 11.114, 145.007, bMatches);
		TestRunner->AddInfo(Measured);
		ASSERT_THAT(IsTrue(bMatches, Measured));
	}

	// 'amet consectetur' fits its 152 px, so nothing is cut and no ellipsis drawn; Chrome's coverage right of it is 0 (28.0 left of it).
	TEST_METHOD(TextThatFits_HasNoEllipsis)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::OpenAndRenderPixels(View, TEXT("CssParity/Text/TextOverflowCutTest.rml"),
			TEXT("TextOverflowCut"))));
		const double Coverage = View.SumCoverage(FIntRect(140, 40, 152, 48), FColor(32, 160, 64, 255));
		ASSERT_THAT(IsTrue(Coverage < 0.5, FString::Printf(TEXT("green coverage after 'consectetur' %.3f, Chrome 0"), Coverage)));
	}
};

// 'translate', 'rotate' and 'scale' applied before 'transform', each on a box placed at 40,20 of an 80 px row. Expected boxes are Chrome's
// getBoundingClientRect with transforms on (chrome_layout_compare.py --animations-at 0) on the IndividualTransformsTest fixture.
TEST_CLASS(ArcRmlCssParityIndividualTransformTests, "ArcRmlUI.CssParity.IndividualTransforms")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/Transform/IndividualTransformsTest.rml"),
			TEXT("IndividualTransforms"), Boxes, FIntPoint::ZeroValue, 0.05f, true)));
	}

	TEST_METHOD(TranslateRotateAndTransform_ComposeInThatOrder)
	{
		ExpectBoxes({ { TEXT("it-compose"), 45.0f, 15.0f, 20.0f, 20.0f }, { TEXT("it-order"), 75.0f, 335.0f, 20.0f, 20.0f } });
	}

	TEST_METHOD(PercentTranslate_MovesByTheBoxWidth)
	{
		ExpectBoxes({ { TEXT("it-half"), 20.0f, 100.0f, 40.0f, 20.0f }, { TEXT("it-var"), 20.0f, 420.0f, 40.0f, 20.0f } });
	}

	TEST_METHOD(ScaleAndRotate_TransformAboutTheCentre)
	{
		ExpectBoxes({ { TEXT("it-scale"), 30.0f, 175.0f, 60.0f, 30.0f }, { TEXT("it-scale-two"), 20.0f, 505.0f, 80.0f, 10.0f },
			{ TEXT("it-rotate"), 38.786797f, 248.786797f, 42.426407f, 42.426407f } });
	}

	// 'rotate' comes after 'translate' and before 'scale' and 'transform': 'scale: 2 1' stretches the box along its own x axis, which
	// the 90 degree turn then stands upright (10 x 20; scale after rotate gives 20 x 10), and 'translateX(10px)' moves it along the turned
	// axis, down (transform before rotate moves it right).
	TEST_METHOD(Rotate_ComesBeforeScaleAndTransform)
	{
		ExpectBoxes({ { TEXT("it-rotate-scale"), 40.0f, 735.0f, 10.0f, 20.0f }, { TEXT("it-rotate-transform"), 40.0f, 830.0f, 10.0f, 10.0f } });
	}

	TEST_METHOD(KeyframesOfTranslateAndScale_Interpolate)
	{
		ExpectBoxes({ { TEXT("it-translate-anim"), 90.0f, 590.0f, 40.0f, 20.0f }, { TEXT("it-scale-anim"), 20.0f, 650.0f, 80.0f, 40.0f } });
	}
};

// Boxes drawn under 'translate', 'rotate' and 'scale'. Expected pixels come from a Chrome screenshot of the IndividualTransformsTest fixture with
// transforms on (--animations-at 0).
TEST_CLASS(ArcRmlCssParityIndividualTransformPixelTests, "ArcRmlUI.Pixels.IndividualTransforms")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectPixels(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromePixel> Pixels)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/Transform/IndividualTransformsTest.rml"),
			TEXT("IndividualTransforms"), Pixels)));
	}

	TEST_METHOD(RotatedBox_IsDrawnAsADiamond)
	{
		const FColor White(255, 255, 255, 255);
		const FColor Purple(128, 32, 192, 255);
		ExpectPixels({ { TEXT("centre"), 60, 270, Purple }, { TEXT("left corner"), 42, 262, Purple }, { TEXT("above the box"), 52, 252, Purple },
			{ TEXT("box top-right, outside the diamond"), 78, 262, White }, { TEXT("box bottom-left, outside the diamond"), 42, 278, White } });
	}

	TEST_METHOD(ComposedAndTranslatedBoxes_AreDrawnWhereChromeDrawsThem)
	{
		const FColor White(255, 255, 255, 255);
		ExpectPixels({ { TEXT("composed centre"), 55, 25, FColor(32, 80, 208, 255) }, { TEXT("composed top-left"), 46, 16, FColor(32, 80, 208, 255) },
			{ TEXT("composed bottom-right"), 64, 34, FColor(32, 80, 208, 255) }, { TEXT("left of the composed box"), 42, 25, White },
			{ TEXT("half translate"), 25, 110, FColor(32, 160, 64, 255) }, { TEXT("right of the half translate"), 65, 110, White },
			{ TEXT("scaled top-left"), 31, 176, FColor(208, 32, 32, 255) }, { TEXT("scaled bottom-right"), 89, 204, FColor(208, 32, 32, 255) },
			{ TEXT("left of the scaled box"), 28, 190, White }, { TEXT("right of the scaled box"), 92, 190, White },
			{ TEXT("ordered translate and scale"), 94, 345, FColor(128, 80, 32, 255) }, { TEXT("untransformed place"), 45, 345, White },
			{ TEXT("translate keyframes"), 125, 600, FColor(128, 16, 48, 255) }, { TEXT("left of the translated box"), 85, 600, White },
			{ TEXT("scale keyframes top-left"), 21, 651, FColor(48, 48, 48, 255) }, { TEXT("scale keyframes bottom-right"), 99, 689, FColor(48, 48, 48, 255) },
			{ TEXT("left of the scaled keyframes box"), 18, 670, White } });
	}
};

// '@property' registrations, '@supports' conditions and '@starting-style' rules on 10 px high boxes, 40 px wide unless a rule widens
// them. Expected boxes and colours come from Chrome on the PropertySupportsTest fixture.
TEST_CLASS(ArcRmlCssParityPropertySupportsTests, "ArcRmlUI.CssParity.PropertySupports")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/PropertySupportsTest.rml"),
			TEXT("PropertySupports"), Boxes)));
	}

	void ExpectColour(const TCHAR* DomId, const TCHAR* Colour)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(AreEqual(FString(Colour), Element->Color, DomId));
	}

	TEST_METHOD(RegisteredPropertyThatDoesNotInherit_StartsAtItsInitialValueUnderAParentThatSetsIt)
	{
		ExpectBoxes({ { TEXT("pb-none-child"), 0.0f, 46.0f, 208.0f, 28.0f }, { TEXT("reg-child"), 0.0f, 84.0f, 70.0f, 10.0f } });
	}

	TEST_METHOD(RegisteredPropertySetToInitial_TakesItsInitialValue)
	{
		ExpectBoxes({ { TEXT("reg-initial"), 0.0f, 94.0f, 70.0f, 10.0f } });
	}

	TEST_METHOD(RegisteredPropertySetToInherit_TakesTheParentsValue)
	{
		ExpectBoxes({ { TEXT("reg-inherit"), 0.0f, 104.0f, 90.0f, 10.0f }, { TEXT("reg-inherit-grand"), 0.0f, 114.0f, 70.0f, 10.0f } });
	}

	TEST_METHOD(UnregisteredPropertySetToInitial_IsGuaranteedInvalid)
	{
		ExpectBoxes({ { TEXT("unreg-initial"), 0.0f, 124.0f, 30.0f, 10.0f } });
	}

	TEST_METHOD(RegistrationInAnotherLinkedSheet_HoldsForTheDocument)
	{
		ExpectBoxes({ { TEXT("other-sheet"), 0.0f, 134.0f, 60.0f, 10.0f } });
	}

	TEST_METHOD(SupportedDeclaration_AppliesTheBody)
	{
		ExpectBoxes({ { TEXT("sup-grid"), 0.0f, 144.0f, 50.0f, 10.0f } });
		ExpectColour(TEXT("sup-grid"), TEXT("#ff0000ff"));
	}

	TEST_METHOD(UnknownProperty_DropsTheBody)
	{
		ExpectBoxes({ { TEXT("sup-moz"), 0.0f, 154.0f, 40.0f, 10.0f } });
		ExpectColour(TEXT("sup-moz"), TEXT("#000000ff"));
	}

	TEST_METHOD(NotOfAnUnknownProperty_AppliesTheBody)
	{
		ExpectBoxes({ { TEXT("sup-not"), 0.0f, 164.0f, 50.0f, 10.0f } });
	}

	TEST_METHOD(AndAndOr_CombineTheirConditions)
	{
		ExpectBoxes({ { TEXT("sup-and"), 0.0f, 174.0f, 40.0f, 10.0f }, { TEXT("sup-or"), 0.0f, 184.0f, 50.0f, 10.0f } });
	}

	TEST_METHOD(SelectorFunction_IsTrueForASelectorThatParses)
	{
		ExpectBoxes({ { TEXT("sup-selector"), 0.0f, 194.0f, 50.0f, 10.0f }, { TEXT("sup-unknown-selector"), 0.0f, 204.0f, 40.0f, 10.0f } });
	}

	TEST_METHOD(StartingStyle_RulesAreDropped)
	{
		ExpectBoxes({ { TEXT("start"), 0.0f, 214.0f, 40.0f, 10.0f } });
	}
};

// The child of a '.border-dashed' box reads the registered non-inherited '--tw-border-style' initial value, so its 4 px top border
// is solid. Expected pixels come from a Chrome screenshot of the PropertySupportsTest fixture.
TEST_CLASS(ArcRmlCssParityPropertySupportsPixelTests, "ArcRmlUI.Pixels.PropertySupports")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(ChildOfADashedBox_DrawsASolidBorder)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FColor Blue(32, 80, 208, 255);
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/AtRules/PropertySupportsTest.rml"),
			TEXT("PropertySupports"),
			{ { TEXT("top border at x 10"), 10, 6, Blue }, { TEXT("top border at x 30"), 30, 6, Blue }, { TEXT("top border at x 50"), 50, 6, Blue },
				{ TEXT("top border at x 70"), 70, 6, Blue }, { TEXT("top border at x 90"), 90, 6, Blue }, { TEXT("top border at x 110"), 110, 6, Blue },
				{ TEXT("top border at x 130"), 130, 6, Blue }, { TEXT("top border at x 150"), 150, 6, Blue },
				{ TEXT("top border at x 170"), 170, 6, Blue }, { TEXT("top border at x 190"), 190, 6, Blue } })));
	}

	TEST_METHOD(StartingStyleOpacity_IsNotApplied)
	{
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FColor Grey(128, 128, 128, 255);
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromePixels(View, TEXT("CssParity/AtRules/PropertySupportsTest.rml"),
			TEXT("PropertySupports"),
			{ { TEXT("start box at x 5"), 5, 216, Grey }, { TEXT("start box at x 20"), 20, 219, Grey }, { TEXT("start box at x 35"), 35, 222, Grey } })));
	}
};

// Rules nested in a style rule (CSS Nesting) apply with the parent selector put in for '&', or before the nested selector when it has no
// '&'; nested '@media' and '@supports' bodies apply to the parent selector. Expected boxes and colours come from Chrome on the NestingTest
// fixture.
TEST_CLASS(ArcRmlCssParityNestingTests, "ArcRmlUI.CssParity.Nesting")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/NestingTest.rml"),
			TEXT("Nesting"), Boxes)));
	}

	const FArcRmlTestLayoutElement* FindElement(TArray<FArcRmlTestLayoutElement>& Elements, const TCHAR* DomId)
	{
		if (!View.ParseLayout(Elements))
		{
			return nullptr;
		}
		return ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
	}

	void ExpectColour(const TCHAR* DomId, const TCHAR* Colour)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = FindElement(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		ASSERT_THAT(AreEqual(FString(Colour), Element->Color, DomId));
	}

	void ExpectHoverColour(const TCHAR* DomId, const TCHAR* Colour)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = FindElement(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, DomId));
		View.MouseMove(Element->Rect.Position + Element->Rect.Size * 0.5f);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ExpectColour(DomId, Colour);
	}

	TEST_METHOD(NestedRules_PutTheParentSelectorForTheAmpersand)
	{
		ExpectBoxes({ { TEXT("a"), 0.0f, 0.0f, 40.0f, 10.0f }, { TEXT("a-on"), 0.0f, 10.0f, 50.0f, 10.0f }, { TEXT("b-a"), 0.0f, 20.0f, 60.0f, 10.0f } });
		ExpectColour(TEXT("a"), TEXT("#ff0000ff"));
		ExpectColour(TEXT("b-a"), TEXT("#008000ff"));
	}

	TEST_METHOD(NestedHoverRule_AppliesOnHover)
	{
		ExpectBoxes({ { TEXT("a"), 0.0f, 0.0f, 40.0f, 10.0f } });
		ExpectHoverColour(TEXT("a"), TEXT("#0000ffff"));
	}

	TEST_METHOD(NestedRulesOfASelectorList_MatchEveryParent)
	{
		ExpectBoxes({ { TEXT("item"), 0.0f, 30.0f, 30.0f, 10.0f }, { TEXT("entry"), 0.0f, 40.0f, 30.0f, 10.0f },
			{ TEXT("item-on"), 0.0f, 50.0f, 35.0f, 10.0f }, { TEXT("wrap-item"), 0.0f, 60.0f, 70.0f, 10.0f } });
	}

	TEST_METHOD(NestedSelectorWithoutAmpersand_IsADescendantOrRelativeSelector)
	{
		ExpectBoxes({ { TEXT("direct"), 0.0f, 70.0f, 90.0f, 10.0f }, { TEXT("child"), 0.0f, 80.0f, 80.0f, 10.0f },
			{ TEXT("indirect"), 0.0f, 90.0f, 40.0f, 10.0f }, { TEXT("mid"), 0.0f, 100.0f, 100.0f, 10.0f }, { TEXT("leaf"), 0.0f, 100.0f, 25.0f, 10.0f } });
	}

	TEST_METHOD(NestedGroupRules_ApplyTheirBodyToTheParent)
	{
		ExpectBoxes({ { TEXT("m"), 0.0f, 110.0f, 55.0f, 10.0f }, { TEXT("m-s"), 0.0f, 120.0f, 75.0f, 10.0f },
			{ TEXT("container"), 0.0f, 130.0f, 640.0f, 10.0f } });
	}

	TEST_METHOD(MediaNestedInAHoverRule_AppliesOnHover)
	{
		ExpectBoxes({ { TEXT("hover-blue"), 0.0f, 140.0f, 45.0f, 10.0f } });
		ExpectColour(TEXT("hover-blue"), TEXT("#000000ff"));
		ExpectHoverColour(TEXT("hover-blue"), TEXT("#0000ffff"));
	}

	TEST_METHOD(RuleAfterANestingRule_StillApplies)
	{
		ExpectBoxes({ { TEXT("after-nest"), 0.0f, 150.0f, 33.0f, 10.0f } });
	}
};

// A sheet whose every rule is dropped (only '@starting-style' and a false '@supports') still compiles, so the document that links it opens
// and its other sheet applies. Expected boxes come from Chrome on the DroppedSheetTest fixture.
TEST_CLASS(ArcRmlCssParityDroppedSheetTests, "ArcRmlUI.CssParity.DroppedSheet")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(SheetWithOnlyDroppedRules_CompilesAndTheDocumentOpens)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/DroppedSheetTest.rml"),
			TEXT("DroppedSheet"), { { TEXT("ds-box"), 0.0f, 0.0f, 70.0f, 10.0f } })));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("does not parse")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}
};

// A line feed in a style sheet saved with LF endings is white space: it separates the compounds of a selector, the values of a declaration
// and a nested selector from its '&', and the diagnostics keep their lines. Expected boxes come from Chrome on the NewlineTest fixture.
TEST_CLASS(ArcRmlCssParityNewlineTests, "ArcRmlUI.CssParity.Newline")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/NewlineTest.rml"),
			TEXT("Newline"), Boxes)));
	}

	void ExpectLine(const TCHAR* MessagePart, int32 Line)
	{
		const TArray<FArcRmlDiagnostic> Found = ArcRmlUITestHelpers::FindDiagnostics(View, MessagePart);
		ASSERT_THAT(AreEqual(1, Found.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(Line, Found[0].Line, MessagePart));
	}

	TEST_METHOD(LineFeedBetweenCompounds_IsADescendantCombinator)
	{
		ExpectBoxes({ { TEXT("nl-inner"), 0.0f, 0.0f, 60.0f, 10.0f } });
	}

	TEST_METHOD(LineFeedsBetweenValues_SeparateThem)
	{
		ExpectBoxes({ { TEXT("nl-border"), 0.0f, 10.0f, 48.0f, 18.0f } });
	}

	TEST_METHOD(LineFeedBeforeTheAmpersand_KeepsTheDescendantCombinator)
	{
		ExpectBoxes({ { TEXT("nl-parent"), 0.0f, 28.0f, 70.0f, 10.0f }, { TEXT("nl-line"), 0.0f, 38.0f, 20.0f, 10.0f },
			{ TEXT("nl-plain"), 0.0f, 48.0f, 30.0f, 10.0f } });
	}

	TEST_METHOD(Diagnostics_KeepTheirLines)
	{
		ExpectBoxes({ { TEXT("nl-line"), 0.0f, 38.0f, 20.0f, 10.0f } });
		ExpectLine(TEXT("nl-bogus-nested"), 39);
		ExpectLine(TEXT("nl-bogus-after"), 41);
		ExpectLine(TEXT("nl-bogus-last"), 46);
	}
};

// A brace inside a quoted string (either quote, escapes included) is text: in a declaration value it opens no nested rule and ends no
// rule, and in a selector or an '@supports' condition it ends no prelude. Expected boxes come from Chrome on the QuotedBraceTest fixture.
TEST_CLASS(ArcRmlCssParityQuotedBraceTests, "ArcRmlUI.CssParity.QuotedBrace")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/QuotedBraceTest.rml"),
			TEXT("QuotedBrace"), Boxes)));
	}

	TEST_METHOD(BraceInAQuotedValue_IsText)
	{
		ExpectBoxes({ { TEXT("q1"), 0.0f, 0.0f, 50.0f, 10.0f }, { TEXT("q2"), 0.0f, 10.0f, 55.0f, 10.0f }, { TEXT("q3"), 0.0f, 20.0f, 60.0f, 10.0f },
			{ TEXT("q4"), 0.0f, 30.0f, 65.0f, 10.0f } });
	}

	TEST_METHOD(BraceInAQuotedSelectorOrCondition_IsText)
	{
		ExpectBoxes({ { TEXT("q5"), 0.0f, 40.0f, 70.0f, 10.0f }, { TEXT("q6"), 0.0f, 50.0f, 75.0f, 10.0f }, { TEXT("q7"), 0.0f, 60.0f, 80.0f, 10.0f },
			{ TEXT("after"), 0.0f, 70.0f, 33.0f, 10.0f } });
	}
};

// A line feed ends an unterminated string (CSS Syntax 3 §4.3.5): the declaration, selector or at-rule prelude holding it is dropped and
// the rest of the sheet parses; an escaped line feed continues the string. Expected boxes come from Chrome on the BadStringTest fixture.
TEST_CLASS(ArcRmlCssParityBadStringTests, "ArcRmlUI.CssParity.BadString")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	void ExpectBoxes(TConstArrayView<ArcRmlCssParityLayoutTestsPrivate::FChromeBox> Boxes)
	{
		ASSERT_THAT(AreEqual(FString(), ArcRmlCssParityLayoutTestsPrivate::CheckChromeBoxes(View, TEXT("CssParity/AtRules/BadStringTest.rml"),
			TEXT("BadString"), Boxes)));
	}

	TEST_METHOD(UnmatchedQuoteInAValue_DropsTheDeclarationUpToItsSemicolon)
	{
		ExpectBoxes({ { TEXT("v1"), 0.0f, 0.0f, 45.0f, 10.0f }, { TEXT("v2"), 0.0f, 10.0f, 47.0f, 10.0f } });
	}

	TEST_METHOD(UnmatchedQuoteInASelector_DropsTheRule)
	{
		ExpectBoxes({ { TEXT("s1-a"), 0.0f, 20.0f, 52.0f, 0.0f }, { TEXT("s1"), 0.0f, 30.0f, 52.0f, 10.0f }, { TEXT("s2"), 0.0f, 40.0f, 54.0f, 10.0f } });
	}

	TEST_METHOD(UnmatchedQuoteInANestedOrAtRulePrelude_DropsTheRule)
	{
		ExpectBoxes({ { TEXT("n1"), 0.0f, 50.0f, 30.0f, 12.0f }, { TEXT("n2"), 0.0f, 62.0f, 30.0f, 14.0f }, { TEXT("a1"), 0.0f, 76.0f, 56.0f, 10.0f } });
	}

	TEST_METHOD(LineFeedInAString_DropsTheDeclaration)
	{
		ExpectBoxes({ { TEXT("r1"), 0.0f, 86.0f, 50.0f, 20.0f } });
	}

	TEST_METHOD(EscapedLineFeedInAString_ContinuesIt)
	{
		ExpectBoxes({ { TEXT("e1"), 0.0f, 106.0f, 61.0f, 10.0f } });
	}

	TEST_METHOD(UnmatchedQuoteBeforeTheRuleEnd_NestsTheLaterRules)
	{
		ExpectBoxes({ { TEXT("ow"), 0.0f, 116.0f, 40.0f, 10.0f }, { TEXT("ow-in-b"), 0.0f, 116.0f, 40.0f, 10.0f },
			{ TEXT("ow-in-c"), 0.0f, 126.0f, 12.0f, 10.0f }, { TEXT("ow-out-b"), 0.0f, 126.0f, 40.0f, 10.0f },
			{ TEXT("ow-out-c"), 0.0f, 136.0f, 40.0f, 10.0f } });
	}

	// .s3 and .s4 would take width 99px if only the bad selector of the list were dropped; their heights show the rules after it apply.
	TEST_METHOD(UnmatchedQuoteInASelectorList_DropsTheMatchingSelectorToo)
	{
		ExpectBoxes({ { TEXT("s3"), 0.0f, 146.0f, 40.0f, 13.0f }, { TEXT("s4"), 0.0f, 159.0f, 40.0f, 13.0f } });
	}

	// The quote in the nested rule body ends at the line feed, so the next two braces close .c and .p and .q is a top-level rule.
	TEST_METHOD(UnmatchedQuoteInANestedRuleBody_EndsAtTheLineFeed)
	{
		ExpectBoxes({ { TEXT("c"), 0.0f, 169.0f, 40.0f, 11.0f }, { TEXT("q"), 0.0f, 179.0f, 61.0f, 10.0f } });
	}

	TEST_METHOD(EscapedLineFeedInANestedRuleBody_ContinuesTheString)
	{
		ExpectBoxes({ { TEXT("c2"), 0.0f, 189.0f, 63.0f, 10.0f } });
	}

	// The nested rule body keeps no character for the escaped line feed, so the attribute selector's string is "ab", not "a b".
	TEST_METHOD(EscapedLineFeedInANestedSelectorString_JoinsTheLines)
	{
		ExpectBoxes({ { TEXT("pe-k"), 0.0f, 199.0f, 65.0f, 10.0f } });
	}

	// The bad keyframe's block holds a nested block: read as properties it would end at the inner brace and leak `to { width: 64px; }`
	// out of @keyframes as a top-level rule matching the <to> element.
	TEST_METHOD(UnmatchedQuoteInAKeyframeSelector_DropsTheKeyframe)
	{
		ExpectBoxes({ { TEXT("kt"), 0.0f, 209.0f, 1280.0f, 13.0f } });
	}

	TEST_METHOD(Diagnostics_KeepTheirLines)
	{
		ExpectBoxes({ { TEXT("e1"), 0.0f, 106.0f, 61.0f, 10.0f } });
		TArray<int32> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("ends at a line feed")))
		{
			Lines.Add(Diagnostic.Line);
		}
		Lines.Sort();
		ASSERT_THAT(AreEqual(FString(TEXT("23,29,34,41,49,56,62,70,82,88,95,101,117")),
			FString::JoinBy(Lines, TEXT(","), [](int32 Line) { return FString::FromInt(Line); }), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}
};

// A string that ends at a line feed inside an open '(' or '[' keeps that block open up to its matching closer (CSS Syntax 3 section 5.4.8), so
// everything up to it is dropped and, with no closer, the rest of its sheet. Each case is its own sheet; expected widths and heights come
// from Chrome on the BadStringBlockTest fixture. Only x, width and height are compared, so a box's mismatch does not shift the later ones.
TEST_CLASS(ArcRmlCssParityBadStringBlockTests, "ArcRmlUI.CssParity.BadStringBlock")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(BadStringInsideAnOpenBlock_KeepsItOpenAsChrome)
	{
		using namespace ArcRmlCssParityLayoutTestsPrivate;
		const FString Opened = CheckChromeBoxes(View, TEXT("CssParity/AtRules/BadStringBlockTest.rml"), TEXT("BadStringBlock"), {});
		ASSERT_THAT(AreEqual(FString(), Opened));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));

		const FChromeBox Chrome[] = {
			{TEXT("b1"), 0.0f, 0.0f, 41.0f, 10.0f},
			{TEXT("b1-after"), 0.0f, 10.0f, 40.0f, 10.0f},
			{TEXT("p1"), 0.0f, 20.0f, 42.0f, 10.0f},
			{TEXT("p1-after"), 0.0f, 30.0f, 40.0f, 10.0f},
			{TEXT("m1"), 0.0f, 40.0f, 43.0f, 10.0f},
			{TEXT("m2"), 0.0f, 50.0f, 40.0f, 10.0f},
			{TEXT("m3"), 0.0f, 60.0f, 40.0f, 10.0f},
			{TEXT("m4"), 0.0f, 70.0f, 44.0f, 10.0f},
			{TEXT("n1"), 0.0f, 80.0f, 45.0f, 10.0f},
			{TEXT("n1-after"), 0.0f, 90.0f, 40.0f, 10.0f},
			{TEXT("ns"), 0.0f, 100.0f, 46.0f, 10.0f},
			{TEXT("ns-after"), 0.0f, 110.0f, 40.0f, 10.0f},
			{TEXT("vf"), 0.0f, 120.0f, 47.0f, 10.0f},
			{TEXT("vf-after"), 0.0f, 130.0f, 40.0f, 10.0f},
			{TEXT("k1"), 0.0f, 140.0f, 48.0f, 10.0f},
			{TEXT("k2"), 0.0f, 150.0f, 40.0f, 10.0f},
			{TEXT("k3"), 0.0f, 160.0f, 40.0f, 10.0f},
			{TEXT("k4"), 0.0f, 170.0f, 49.0f, 10.0f},
		};
		FString Mismatches;
		for (const FChromeBox& Box : Chrome)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, Box.DomId);
			if (!Element)
			{
				Mismatches += FString::Printf(TEXT("[%s missing] "), Box.DomId);
				continue;
			}
			const FArcRmlInspectorRect& Rect = Element->Rect;
			if (!IsNear(Rect.Position.X, Box.X) || !IsNear(Rect.Size.X, Box.Width) || !IsNear(Rect.Size.Y, Box.Height))
			{
				Mismatches += FString::Printf(TEXT("[%s x %g, %gx%g; Chrome x %g, %gx%g] "), Box.DomId, Rect.Position.X, Rect.Size.X, Rect.Size.Y, Box.X,
					Box.Width, Box.Height);
			}
		}

		const TCHAR* const HeldSheets[] = {TEXT("BadStringBlockBracket"), TEXT("BadStringBlockParen"), TEXT("BadStringBlockNested"),
			TEXT("BadStringBlockNestedSupports"), TEXT("BadStringBlockValue")};
		TMap<FString, int32> HeldCounts;
		for (const FArcRmlDiagnostic& Diagnostic : ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("stays open")))
		{
			if (Diagnostic.Kind == EArcRmlDiagnosticKind::InvalidAtRule && Diagnostic.Severity == EArcRmlDiagnosticSeverity::Warning)
			{
				++HeldCounts.FindOrAdd(FPaths::GetBaseFilename(Diagnostic.File));
			}
		}
		for (const TCHAR* Sheet : HeldSheets)
		{
			const int32 Count = HeldCounts.FindRef(Sheet);
			if (Count != 1)
			{
				Mismatches += FString::Printf(TEXT("[%s: %d invalid-at-rule warnings for the held block, expected 1] "), Sheet, Count);
			}
			HeldCounts.Remove(Sheet);
		}
		const TCHAR* const ClosedSheets[] = {TEXT("BadStringBlockMatch"), TEXT("BadStringBlockDeep")};
		for (const TCHAR* Sheet : ClosedSheets)
		{
			const int32 Count = HeldCounts.FindRef(Sheet);
			if (Count != 0)
			{
				Mismatches += FString::Printf(TEXT("[%s: %d held-block warnings, expected none as the block closes] "), Sheet, Count);
			}
			HeldCounts.Remove(Sheet);
		}
		for (const TPair<FString, int32>& Extra : HeldCounts)
		{
			Mismatches += FString::Printf(TEXT("[%s: %d unexpected held-block warnings] "), *Extra.Key, Extra.Value);
		}
		ASSERT_THAT(AreEqual(FString(), Mismatches, Mismatches + TEXT("| ") + ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}
};
