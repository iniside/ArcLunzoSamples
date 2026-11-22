// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlCssParityOpacityTestsPrivate
{
	const FColor PlainFullColour(255, 0, 0, 255);
	const FColor PlainUnderHalfParent(127, 0, 0, 127);
	const FColor HalfFullColour(0, 0, 255, 255);
	const FColor HalfUnderHalfParent(0, 0, 63, 63);

	int32 CountColour(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && Draw.FirstColourPremultiplied == Colour)
			{
				++Count;
			}
		}
		return Count;
	}

	FString DescribeColours(const TArray<FArcRmlTestDraw>& Draws)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture)
			{
				Parts.Add(FString::Printf(TEXT("(%d,%d,%d,%d)"), Draw.FirstColourPremultiplied.R, Draw.FirstColourPremultiplied.G, Draw.FirstColourPremultiplied.B,
					Draw.FirstColourPremultiplied.A));
			}
		}
		return FString::Join(Parts, TEXT(" "));
	}
}

TEST_CLASS(ArcRmlCssParityOpacityTests, "ArcRmlUI.CssParity.Opacity")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenFixture()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("CssParity/Anim/OpacityInherit.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		return View.Open(Path, ArcRmlUITestHelpers::ViewSize, 1.0f) && View.FlushUntilSettled() && View.GetProbe().FrameSerial > 0
			&& !View.GetProbe().bPendingAsyncWork;
	}

	TEST_METHOD(ChildWithoutOpacity_DoesNotInheritTheParentOpacityButPaintsWithIt)
	{
		using namespace ArcRmlCssParityOpacityTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainUnderHalfParent), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), PlainFullColour), DescribeColours(View.GetDraws())));
	}

	TEST_METHOD(ChildWithOwnOpacity_MultipliesItWithTheParentOpacity)
	{
		using namespace ArcRmlCssParityOpacityTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), HalfUnderHalfParent), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), HalfFullColour), DescribeColours(View.GetDraws())));
	}

	TEST_METHOD(ParentOpacityChangedAtRuntime_RepaintsThePlainChildAtFullColour)
	{
		using namespace ArcRmlCssParityOpacityTestsPrivate;
		ASSERT_THAT(IsTrue(OpenFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const uint64 Before = View.GetProbe().FrameSerial;
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainUnderHalfParent), DescribeColours(View.GetDraws())));

		View.SetRootClass(TEXT("flat"), true);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().FrameSerial > Before));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainFullColour), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), PlainUnderHalfParent), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), HalfUnderHalfParent), DescribeColours(View.GetDraws())));
	}
};
