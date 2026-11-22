// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityOutlineTestsPrivate
{
	constexpr float Tolerance = 0.05f;
	constexpr float RotatedTolerance = 2.0f;
	constexpr float EdgeTolerance = 1.0f;

	const TCHAR* const OutlinePath = TEXT("CssParity/Outline/OutlineTest.rml");
	const TCHAR* const InkClipPath = TEXT("CssParity/InkClip/InkClipTest.rml");

	const FColor Red(255, 0, 0, 255);
	const FColor Green(0, 255, 0, 255);
	const FColor Blue(0, 0, 255, 255);
	const FColor Magenta(255, 0, 255, 255);
	const FColor Yellow(255, 255, 0, 255);
	const FColor Orange(255, 136, 0, 255);
	const FColor Purple(136, 1, 255, 255);
	const FColor Mint(0, 255, 136, 255);
	const FColor SpanRed(255, 68, 68, 255);
	const FColor SpanGreen(68, 255, 68, 255);
	const FColor SpanBlue(68, 68, 255, 255);
	const FColor SpanAmber(255, 170, 0, 255);
	const FColor FlexOutline(255, 0, 102, 255);
	const FColor FlexNext(102, 255, 0, 255);
	const FColor ChipOutline(0, 102, 255, 255);
	const FColor ChipNext(255, 204, 0, 255);
	const FColor GridOutline(204, 0, 255, 255);
	const FColor GridNext(0, 204, 255, 255);
	const FColor FloatOutline(255, 102, 0, 255);
	const FColor FloatNext(102, 0, 255, 255);
	const FColor NegOutline(170, 255, 0, 255);
	const FColor EllipseOutline(136, 0, 204, 255);
	const FColor StackNext(0, 255, 255, 255);
	const FColor PositionedOutline(0, 170, 85, 255);
	const FColor PositionedChild(170, 85, 0, 255);
	const FColor OrangeRecoloured(255, 128, 0, 255);
	const FColor ShadowCyan(0, 255, 255, 255);
	const FColor EffectPink(255, 0, 170, 255);
	const FColor GlyphPink(255, 102, 255, 255);
	const FColor PlainChild(85, 153, 204, 255);

	FBox2f Box(float MinX, float MinY, float MaxX, float MaxY)
	{
		return FBox2f(FVector2f(MinX, MinY), FVector2f(MaxX, MaxY));
	}

	FString DescribeBox(const FBox2f& Source)
	{
		return FString::Printf(TEXT("(%.2f,%.2f)-(%.2f,%.2f)"), Source.Min.X, Source.Min.Y, Source.Max.X, Source.Max.Y);
	}

	FString DescribeRect(const FIntRect& Rect)
	{
		return FString::Printf(TEXT("(%d,%d)-(%d,%d)"), Rect.Min.X, Rect.Min.Y, Rect.Max.X, Rect.Max.Y);
	}

	bool SameBox(const FBox2f& A, const FBox2f& B, float Slack)
	{
		return FMath::IsNearlyEqual(A.Min.X, B.Min.X, Slack) && FMath::IsNearlyEqual(A.Min.Y, B.Min.Y, Slack) && FMath::IsNearlyEqual(A.Max.X, B.Max.X, Slack)
			&& FMath::IsNearlyEqual(A.Max.Y, B.Max.Y, Slack);
	}

	FBox2f Scaled(const FBox2f& Source, float Factor)
	{
		return Box(Source.Min.X * Factor, Source.Min.Y * Factor, Source.Max.X * Factor, Source.Max.Y * Factor);
	}

	void DropCompiledFixture(const TCHAR* RelativeRml)
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(RelativeRml));
		const FString Dir = ArcRmlUITestHelpers::GetTestContentDir();
		const FString Sheet = FPaths::Combine(Dir, FPaths::GetPath(RelativeRml), FPaths::GetBaseFilename(RelativeRml) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
	}

	int32 FindGeometry(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (Draws[Index].Op == EArcRmlTestDrawOp::Geometry && Draws[Index].FirstColourPremultiplied == Colour)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	int32 CountGeometry(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && Draw.FirstColourPremultiplied == Colour)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 FindTexturedByBounds(const TArray<FArcRmlTestDraw>& Draws, const FBox2f& Bounds, float Slack)
	{
		for (int32 Index = 0; Index < Draws.Num(); ++Index)
		{
			if (Draws[Index].Op == EArcRmlTestDrawOp::Geometry && Draws[Index].bTextured && SameBox(Draws[Index].Bounds, Bounds, Slack))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	FIntRect Intersect(const FBox2f& Bounds, const FIntRect& Scissor)
	{
		return FIntRect(FMath::Max(FMath::RoundToInt(Bounds.Min.X), Scissor.Min.X), FMath::Max(FMath::RoundToInt(Bounds.Min.Y), Scissor.Min.Y),
			FMath::Min(FMath::RoundToInt(Bounds.Max.X), Scissor.Max.X), FMath::Min(FMath::RoundToInt(Bounds.Max.Y), Scissor.Max.Y));
	}

	bool HasScissor(const FArcRmlTestDraw& Draw, const FIntRect& Expected)
	{
		return Draw.Scissor.IsSet() && Draw.Scissor.GetValue() == Expected;
	}
}

TEST_CLASS(ArcRmlCssParityOutlineTests, "ArcRmlUI.CssParity.Outline")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenOutline(float DpRatio)
	{
		ArcRmlCssParityOutlineTestsPrivate::DropCompiledFixture(ArcRmlCssParityOutlineTestsPrivate::OutlinePath);
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ArcRmlCssParityOutlineTestsPrivate::OutlinePath), DpRatio) && View.FlushUntilSettled()
			&& View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool WaitForFrame(uint64 Before)
	{
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
	}

	bool CheckOutline(const FColor& Colour, const FBox2f& Expected, float Slack, const TCHAR* What)
	{
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		if (!Assert.AreEqual(1, ArcRmlCssParityOutlineTestsPrivate::CountGeometry(Draws, Colour), FString::Printf(TEXT("%s outline draw count"), What)))
		{
			return false;
		}
		const FBox2f Actual = Draws[ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, Colour)].Bounds;
		return Assert.IsTrue(ArcRmlCssParityOutlineTestsPrivate::SameBox(Actual, Expected, Slack),
			FString::Printf(TEXT("%s outline bounds %s, Chrome %s"), What, *ArcRmlCssParityOutlineTestsPrivate::DescribeBox(Actual),
				*ArcRmlCssParityOutlineTestsPrivate::DescribeBox(Expected)));
	}

	bool CheckBasicRings(float Factor)
	{
		return CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Red, ArcRmlCssParityOutlineTestsPrivate::Scaled(ArcRmlCssParityOutlineTestsPrivate::Box(15, 15, 125, 65), Factor),
				ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("radius 8, offset 3"))
			&& CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Green, ArcRmlCssParityOutlineTestsPrivate::Scaled(ArcRmlCssParityOutlineTestsPrivate::Box(166, 26, 254, 54), Factor),
				ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("offset -10"))
			&& CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Blue, ArcRmlCssParityOutlineTestsPrivate::Scaled(ArcRmlCssParityOutlineTestsPrivate::Box(297, 17, 403, 63), Factor),
				ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("dashed 3px"))
			&& CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Purple, ArcRmlCssParityOutlineTestsPrivate::Scaled(ArcRmlCssParityOutlineTestsPrivate::Box(15, 215, 85, 255), Factor),
				ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("thick"))
			&& CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Mint, ArcRmlCssParityOutlineTestsPrivate::Scaled(ArcRmlCssParityOutlineTestsPrivate::Box(157, 217, 223, 253), Factor),
				ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("medium"));
	}

	bool CheckBefore(const FColor& First, const FColor& Second, const TCHAR* What)
	{
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 FirstIndex = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, First);
		const int32 SecondIndex = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, Second);
		if (!Assert.IsTrue(FirstIndex != INDEX_NONE && SecondIndex != INDEX_NONE, FString::Printf(TEXT("%s: both draws found"), What)))
		{
			return false;
		}
		return Assert.IsTrue(FirstIndex < SecondIndex, FString::Printf(TEXT("%s: draw index %d must precede %d"), What, FirstIndex, SecondIndex));
	}

	TEST_METHOD(RingBoundsMatchChrome)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckBasicRings(1.0f)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Magenta, ArcRmlCssParityOutlineTestsPrivate::Box(16, 96, 224, 144),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("block with following sibling"))));
	}

	TEST_METHOD(RingBoundsScaleWithDpRatio)
	{
		ASSERT_THAT(IsTrue(OpenOutline(2.0f)));
		ASSERT_THAT(IsTrue(CheckBasicRings(2.0f)));
	}

	TEST_METHOD(ThickAndMediumWidthsFollowDpRatioChange)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckBasicRings(1.0f)));
		const uint64 Before = View.GetProbe().FrameSerial;
		View.Resize(ArcRmlUITestHelpers::ViewSize, 2.0f);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(IsTrue(CheckBasicRings(2.0f)));
	}

	TEST_METHOD(RotatedElementOutlineBoundsMatchChrome)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Orange, ArcRmlCssParityOutlineTestsPrivate::Box(440, 17, 540, 103),
			ArcRmlCssParityOutlineTestsPrivate::RotatedTolerance, TEXT("rotate(30deg), offset 2"))));
	}

	TEST_METHOD(NegativeOffsetClampsToHalfTheSnappedBox)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::NegOutline, ArcRmlCssParityOutlineTestsPrivate::Box(910, 430, 915, 435),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("25px box, offset -20"))));
	}

	TEST_METHOD(NegativeOffsetClampsPerAxisOnAnEllipticalRing)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::EllipseOutline, ArcRmlCssParityOutlineTestsPrivate::Box(1011, 420, 1189, 480),
			ArcRmlCssParityOutlineTestsPrivate::EdgeTolerance, TEXT("200x60 box, radius 30, outline 30, offset -40"))));
	}

	TEST_METHOD(SplitInlineElementHasOneUnionOutline)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::SpanRed, ArcRmlCssParityOutlineTestsPrivate::Box(18, 299, 123, 372),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("solid 2px over lines"))));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::SpanGreen, ArcRmlCssParityOutlineTestsPrivate::Box(195, 296, 306, 375),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("offset 3, radius 6"))));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::SpanBlue, ArcRmlCssParityOutlineTestsPrivate::Box(378, 299, 483, 372),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("dashed 2px"))));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::SpanAmber, ArcRmlCssParityOutlineTestsPrivate::Box(557, 298, 693, 373),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("solid 3px over lines of different width"))));
	}

	TEST_METHOD(AtomicUnitOutlineIsCoveredByTheNextSibling)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::FlexOutline, ArcRmlCssParityOutlineTestsPrivate::FlexNext, TEXT("flex item outline, next item background"))));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::ChipOutline, ArcRmlCssParityOutlineTestsPrivate::ChipNext, TEXT("inline-block outline, next chip background"))));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::GridOutline, ArcRmlCssParityOutlineTestsPrivate::GridNext, TEXT("grid item outline, next cell background"))));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::FloatOutline, ArcRmlCssParityOutlineTestsPrivate::FloatNext, TEXT("float outline, next float background"))));
	}

	TEST_METHOD(PlainBlockOutlinePaintsAfterFollowingContent)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::StackNext, ArcRmlCssParityOutlineTestsPrivate::Magenta, TEXT("next sibling background, block outline"))));
	}

	TEST_METHOD(OutlinePaintsBeforePositionedChildren)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(IsTrue(CheckBefore(ArcRmlCssParityOutlineTestsPrivate::PositionedOutline, ArcRmlCssParityOutlineTestsPrivate::PositionedChild,
			TEXT("block outline, positioned child background"))));
	}

	TEST_METHOD(OutlineIsClippedByAFittingOverflowHiddenAncestor)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		ASSERT_THAT(AreEqual(1, ArcRmlCssParityOutlineTestsPrivate::CountGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::Yellow)));
		const FArcRmlTestDraw& Ring = Draws[ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::Yellow)];
		ASSERT_THAT(IsTrue(Ring.Scissor.IsSet(), TEXT("the outline carries a scissor")));
		const FIntRect Scissor = Ring.Scissor.GetValue();
		ASSERT_THAT(IsTrue(Scissor == FIntRect(300, 100, 400, 160), FString::Printf(TEXT("scissor %s"), *ArcRmlCssParityOutlineTestsPrivate::DescribeRect(Scissor))));
		const FIntRect Visible = ArcRmlCssParityOutlineTestsPrivate::Intersect(Ring.Bounds, Scissor);
		ASSERT_THAT(IsTrue(Visible == FIntRect(300, 100, 385, 155),
			FString::Printf(TEXT("visible ring %s, Chrome (300,100)-(385,155)"), *ArcRmlCssParityOutlineTestsPrivate::DescribeRect(Visible))));
	}

	TEST_METHOD(ChangedOutlineWidthRegeneratesTheRing)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("wide"), true);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Red, ArcRmlCssParityOutlineTestsPrivate::Box(12, 12, 128, 68),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("width 5, offset 3"))));
		Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("wide"), false);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Red, ArcRmlCssParityOutlineTestsPrivate::Box(15, 15, 125, 65),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("width back to 2"))));
	}

	TEST_METHOD(ChangedOutlineOffsetRegeneratesTheRing)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("off"), true);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::Red, ArcRmlCssParityOutlineTestsPrivate::Box(11, 11, 129, 69),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("offset 7, width 2"))));
	}

	TEST_METHOD(ChangedOutlineColourRegeneratesTheRing)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("col"), true);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityOutlineTestsPrivate::CountGeometry(View.GetDraws(), ArcRmlCssParityOutlineTestsPrivate::Red), TEXT("old colour draws")));
		ASSERT_THAT(IsTrue(CheckOutline(ArcRmlCssParityOutlineTestsPrivate::OrangeRecoloured, ArcRmlCssParityOutlineTestsPrivate::Box(15, 15, 125, 65),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("recoloured"))));
	}

	TEST_METHOD(ChangedOutlineStyleToNoneRemovesTheRing)
	{
		ASSERT_THAT(IsTrue(OpenOutline(1.0f)));
		ASSERT_THAT(AreEqual(1, ArcRmlCssParityOutlineTestsPrivate::CountGeometry(View.GetDraws(), ArcRmlCssParityOutlineTestsPrivate::Red)));
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("sty"), true);
		ASSERT_THAT(IsTrue(WaitForFrame(Before)));
		ASSERT_THAT(AreEqual(0, ArcRmlCssParityOutlineTestsPrivate::CountGeometry(View.GetDraws(), ArcRmlCssParityOutlineTestsPrivate::Red), TEXT("ring draws with outline-style none")));
	}
};

TEST_CLASS(ArcRmlCssParityInkClipTests, "ArcRmlUI.CssParity.InkClip")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenInkClip()
	{
		ArcRmlCssParityOutlineTestsPrivate::DropCompiledFixture(ArcRmlCssParityOutlineTestsPrivate::InkClipPath);
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(ArcRmlCssParityOutlineTestsPrivate::InkClipPath)) && View.FlushUntilSettled()
			&& View.FlushUntilSettled() && View.GetProbe().bHasDocument;
	}

	bool CheckShadowScissor(const FBox2f& ShadowBounds, const FIntRect& Expected, float Slack, const TCHAR* What)
	{
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindTexturedByBounds(Draws, ShadowBounds, Slack);
		if (!Assert.IsTrue(Index != INDEX_NONE, FString::Printf(TEXT("%s: shadow draw %s found"), What, *ArcRmlCssParityOutlineTestsPrivate::DescribeBox(ShadowBounds))))
		{
			return false;
		}
		if (!Assert.IsTrue(Draws[Index].Scissor.IsSet(), FString::Printf(TEXT("%s: shadow draw has a scissor"), What)))
		{
			return false;
		}
		const FIntRect Actual = Draws[Index].Scissor.GetValue();
		return Assert.IsTrue(Actual == Expected, FString::Printf(TEXT("%s: scissor %s, Chrome clip %s"), What, *ArcRmlCssParityOutlineTestsPrivate::DescribeRect(Actual),
			*ArcRmlCssParityOutlineTestsPrivate::DescribeRect(Expected)));
	}

	TEST_METHOD(BoxShadowIsClippedByAFittingParent)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		ASSERT_THAT(IsTrue(CheckShadowScissor(ArcRmlCssParityOutlineTestsPrivate::Box(15, 35, 145, 105), FIntRect(20, 40, 140, 100),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("spread shadow"))));
	}

	TEST_METHOD(BoxShadowIsClippedByARoundedParentWithAClipMask)
	{
		const FBox2f ShadowBounds = ArcRmlCssParityOutlineTestsPrivate::Box(175, 35, 305, 105);
		ASSERT_THAT(IsTrue(OpenInkClip()));
		ASSERT_THAT(IsTrue(CheckShadowScissor(ShadowBounds, FIntRect(180, 40, 300, 100), ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("radius parent"))));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindTexturedByBounds(Draws, ShadowBounds, ArcRmlCssParityOutlineTestsPrivate::Tolerance);
		ASSERT_THAT(IsTrue(Draws[Index].bClipMask, TEXT("the shadow is drawn under the clip mask")));
	}

	TEST_METHOD(BoxShadowIsClippedByTheInnerOfNestedParents)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		ASSERT_THAT(IsTrue(CheckShadowScissor(ArcRmlCssParityOutlineTestsPrivate::Box(340, 40, 480, 120), FIntRect(350, 50, 470, 110),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("nested parents"))));
	}

	TEST_METHOD(OneAxisOverflowClipsBothAxes)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		ASSERT_THAT(IsTrue(CheckShadowScissor(ArcRmlCssParityOutlineTestsPrivate::Box(535, 35, 665, 105), FIntRect(540, 40, 660, 100),
			ArcRmlCssParityOutlineTestsPrivate::Tolerance, TEXT("overflow-x only"))));
	}

	TEST_METHOD(RotatedChildShadowIsClippedByTheParent)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		ASSERT_THAT(IsTrue(CheckShadowScissor(ArcRmlCssParityOutlineTestsPrivate::Box(693.36f, 21.29f, 826.64f, 118.71f), FIntRect(700, 40, 820, 100),
			ArcRmlCssParityOutlineTestsPrivate::EdgeTolerance, TEXT("rotate(20deg)"))));
	}

	TEST_METHOD(TextShadowIsClippedAtTheParentEdge)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::ShadowCyan);
		ASSERT_THAT(IsTrue(Index != INDEX_NONE, TEXT("text-shadow draw found")));
		ASSERT_THAT(IsTrue(Draws[Index].Bounds.Max.Y > 80.0f, TEXT("the shadow reaches past the parent bottom edge 80")));
		ASSERT_THAT(IsTrue(ArcRmlCssParityOutlineTestsPrivate::HasScissor(Draws[Index], FIntRect(860, 40, 980, 80)), TEXT("scissor is the parent's padding box, Chrome cuts at y 80")));
	}

	TEST_METHOD(FontEffectShadowIsClippedByAFittingParent)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::EffectPink);
		ASSERT_THAT(IsTrue(Index != INDEX_NONE, TEXT("font-effect shadow draw found")));
		ASSERT_THAT(IsTrue(Draws[Index].Bounds.Max.Y > 220.0f, TEXT("the shadow reaches past the parent bottom edge 220")));
		ASSERT_THAT(IsTrue(ArcRmlCssParityOutlineTestsPrivate::HasScissor(Draws[Index], FIntRect(20, 180, 140, 220)), TEXT("scissor is the parent's padding box")));
	}

	TEST_METHOD(DropShadowFilterIsClippedByAFittingParent)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		int32 Composites = 0;
		FIntRect Scissor;
		for (const FArcRmlTestDraw& Draw : View.GetDraws())
		{
			if (Draw.Op == EArcRmlTestDrawOp::CompositeLayers && Draw.Scissor.IsSet())
			{
				++Composites;
				Scissor = Draw.Scissor.GetValue();
			}
		}
		ASSERT_THAT(AreEqual(1, Composites, TEXT("composite draws with a scissor")));
		ASSERT_THAT(IsTrue(Scissor.Max.Y == 100 && Scissor.Max.X <= 1140 && Scissor.Min.X >= 1020 && Scissor.Min.Y >= 40,
			FString::Printf(TEXT("composite scissor %s, parent padding box (1020,40)-(1140,100)"), *ArcRmlCssParityOutlineTestsPrivate::DescribeRect(Scissor))));
		ASSERT_THAT(IsTrue(Scissor.Min.X <= 1030 && Scissor.Max.X >= 1130 && Scissor.Min.Y <= 90, TEXT("the scissor keeps Chrome's visible shadow (1030,90)-(1130,100)")));
	}

	TEST_METHOD(OverhangingGlyphsAreClippedByAFittingParent)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::GlyphPink);
		ASSERT_THAT(IsTrue(Index != INDEX_NONE, TEXT("glyph draw found")));
		ASSERT_THAT(IsTrue(Draws[Index].Bounds.Min.Y < 180.0f, TEXT("the glyphs reach above the parent top edge 180, inside its client height")));
		ASSERT_THAT(IsTrue(ArcRmlCssParityOutlineTestsPrivate::HasScissor(Draws[Index], FIntRect(600, 180, 800, 250)), TEXT("scissor is the parent's padding box")));
	}

	TEST_METHOD(ElementsWithoutInkOverflowKeepTheUnclippedScissor)
	{
		ASSERT_THAT(IsTrue(OpenInkClip()));
		const TArray<FArcRmlTestDraw>& Draws = View.GetDraws();
		const int32 Index = ArcRmlCssParityOutlineTestsPrivate::FindGeometry(Draws, ArcRmlCssParityOutlineTestsPrivate::PlainChild);
		ASSERT_THAT(IsTrue(Index != INDEX_NONE, TEXT("plain child draw found")));
		const FIntRect WholeView(0, 0, ArcRmlUITestHelpers::ViewSize.X, ArcRmlUITestHelpers::ViewSize.Y);
		ASSERT_THAT(IsTrue(ArcRmlCssParityOutlineTestsPrivate::HasScissor(Draws[Index], WholeView), TEXT("the child of a fitting parent keeps the whole-view scissor")));
		ASSERT_THAT(IsFalse(Draws[Index].bClipMask));
	}
};
