// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlBodyRulePixelTestsPrivate
{
	constexpr int32 Tolerance = 2;

	struct FChromePixel
	{
		const TCHAR* Name = TEXT("");
		int32 X = 0;
		int32 Y = 0;
		FColor Chrome;
	};

	const FChromePixel ChromePixels[] = {
		{TEXT("box first pixel"), 10, 300, FColor(170, 187, 204, 255)},
		{TEXT("box inside"), 30, 310, FColor(170, 187, 204, 255)},
		{TEXT("box last pixel"), 49, 339, FColor(170, 187, 204, 255)},
		{TEXT("left of the box"), 9, 300, FColor(48, 32, 16, 255)},
		{TEXT("above the box"), 10, 299, FColor(48, 32, 16, 255)},
		{TEXT("right of the box"), 50, 339, FColor(48, 32, 16, 255)},
		{TEXT("under the box"), 49, 340, FColor(48, 32, 16, 255)},
		{TEXT("where the body box would put it"), 80, 360, FColor(48, 32, 16, 255)},
		{TEXT("canvas"), 640, 500, FColor(48, 32, 16, 255)},
	};

	bool IsNear(const FColor& Measured, const FColor& Expected)
	{
		return FMath::Abs(Measured.R - Expected.R) <= Tolerance && FMath::Abs(Measured.G - Expected.G) <= Tolerance && FMath::Abs(Measured.B - Expected.B) <= Tolerance
			&& FMath::Abs(Measured.A - Expected.A) <= Tolerance;
	}
}

TEST_CLASS(ArcRmlBodyRulePixelTests, "ArcRmlUI.Pixels.BodyRule")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(AbsoluteBoxInABodyWithMargin_LandsAtTheInitialContainingBlockAsInChrome)
	{
		using namespace ArcRmlBodyRulePixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Canvas/BodyBackgroundNoRepeatTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		ASSERT_THAT(IsTrue(View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize, View.GetPixelSize().ToString()));
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : ChromePixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		ASSERT_THAT(AreEqual(FString(), FString::Join(Mismatches, TEXT(" | ")), TEXT("the absolute box is not where Chrome puts it: left 10, top 300 against the initial containing block, not the body box")));
	}

	TEST_METHOD(TranslucentBodyBackground_ReadsBackAsChromeOncePaintedGreyInTheColumnAndTheSideBar)
	{
		using namespace ArcRmlBodyRulePixelTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Canvas/BodyTranslucentTest.rml"));
		FArcRmlTestView::DropCompiledDocument(Path);
		View.SetMaxContentAspect(16.0f / 9.0f);
		ASSERT_THAT(IsTrue(View.Open(Path, FIntPoint(1600, 720), 1.0f) && View.FlushUntilSettled(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().ContentRect == FIntRect(160, 0, 1440, 720), TEXT("the 1280 column sits at x 160 of the 1600 host")));
		ASSERT_THAT(IsTrue(View.RenderPixels()));
		const FChromePixel Grey[] = {
			{TEXT("inside the body, under the box"), 600, 300, FColor(127, 127, 127, 255)},
			{TEXT("inside the body, bottom right"), 1430, 710, FColor(127, 127, 127, 255)},
			{TEXT("left side bar"), 50, 300, FColor(127, 127, 127, 255)},
			{TEXT("right side bar"), 1550, 300, FColor(127, 127, 127, 255)},
			{TEXT("box over the body"), 180, 20, FColor(224, 192, 64, 255)},
		};
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : Grey)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (!IsNear(Measured, Pixel.Chrome))
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		ASSERT_THAT(AreEqual(FString(), FString::Join(Mismatches, TEXT(" | ")),
			TEXT("rgba(0,0,0,.5) on body over the white backdrop is 127 grey everywhere in Chrome; painted twice it would be 64")));
	}
};
