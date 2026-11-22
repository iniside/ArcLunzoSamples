// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityColourShadowTestsPrivate
{
	struct FChromeSwatch
	{
		const TCHAR* Id;
		FIntVector4 Chrome;
		int32 Tolerance = 1;
	};

	const FChromeSwatch ChromeSwatches[] = {
		{TEXT("rgb-alpha"), FIntVector4(0, 0, 0, 26)},
		{TEXT("rgb-plain"), FIntVector4(255, 128, 0, 255)},
		{TEXT("rgba-percent-alpha"), FIntVector4(10, 20, 30, 128)},
		{TEXT("rgb-percent"), FIntVector4(255, 128, 0, 64)},
		{TEXT("rgb-none"), FIntVector4(0, 200, 0, 255)},
		{TEXT("rgb-tight-slash"), FIntVector4(40, 80, 120, 153)},
		{TEXT("rgb-fraction"), FIntVector4(128, 0, 255, 255), 0},
		{TEXT("hsl-plain"), FIntVector4(0, 255, 0, 255)},
		{TEXT("hsl-deg-alpha"), FIntVector4(0, 0, 255, 128)},
		{TEXT("hsla-turn"), FIntVector4(0, 128, 128, 102)},
		{TEXT("hsl-numbers"), FIntVector4(235, 153, 71, 255)},
		{TEXT("hsl-rad"), FIntVector4(64, 191, 191, 255)},
		{TEXT("oklch-alpha"), FIntVector4(43, 127, 255, 128)},
		{TEXT("mix-modern"), FIntVector4(85, 0, 170, 191)},
		{TEXT("rgb-comma"), FIntVector4(1, 2, 3, 255)},
	};

	const FChromeSwatch ChromeVarKeywordSwatches[] = {
		{TEXT("initial-fallback"), FIntVector4(0, 255, 0, 255)},
		{TEXT("initial-empty-fallback"), FIntVector4(0, 0, 255, 255)},
		{TEXT("initial-inherited"), FIntVector4(255, 128, 0, 255)},
		{TEXT("initial-overrides"), FIntVector4(0, 128, 128, 255)},
		{TEXT("unset-inherits"), FIntVector4(18, 52, 86, 255)},
		{TEXT("inherit-inherits"), FIntVector4(101, 67, 33, 255)},
		{TEXT("initial-no-fallback"), FIntVector4(0, 0, 0, 0)},
	};

	const FChromeSwatch ChromeInvalidVarSwatches[] = {
		{TEXT("chain-fallback"), FIntVector4(0, 255, 128, 255)},
		{TEXT("chain-inherited"), FIntVector4(128, 255, 0, 255)},
		{TEXT("chain-no-fallback"), FIntVector4(0, 0, 0, 0)},
		{TEXT("cycle-fallback"), FIntVector4(128, 0, 255, 255)},
		{TEXT("cycle-other-fallback"), FIntVector4(255, 0, 128, 255)},
		{TEXT("cycle-nested-fallback"), FIntVector4(0, 255, 255, 255)},
		{TEXT("cycle-self"), FIntVector4(128, 64, 0, 255)},
	};

	struct FExpectedError
	{
		const TCHAR* Text;
		int32 Count;
	};

	const FExpectedError VarKeywordFixtureErrors[] = {
		{TEXT("Invalid substitution, variable '--c' not defined. In element: div#initial-no-fallback"), 1},
		{TEXT("Invalid substitution, variable '--missing' not defined. In element: div#chain-fallback"), 1},
		{TEXT("Invalid substitution, variable '--missing' not defined. In element: div#chain-inherited"), 1},
		{TEXT("Invalid substitution, variable '--missing' not defined. In element: div#chain-no-fallback"), 1},
		{TEXT("Invalid substitution, cycle detected with variable: --a. In element: div#cycle-fallback"), 1},
		{TEXT("Invalid substitution, cycle detected with variable: --b. In element: div#cycle-other-fallback"), 1},
		{TEXT("Invalid substitution, cycle detected with variable: --a. In element: div#cycle-nested-fallback"), 1},
		{TEXT("Invalid substitution, cycle detected with variable: --b. In element: div#cycle-nested-fallback"), 1},
		{TEXT("Invalid substitution, cycle detected with variable: --a. In element: div#cycle-self"), 1},
	};

	const FChromeSwatch ChromeVarUndefined[] = {
		{TEXT("chain-inherited"), FIntVector4(128, 255, 0, 255)},
	};

	const FChromeSwatch ChromeVarDefinedLater[] = {
		{TEXT("chain-inherited"), FIntVector4(64, 128, 192, 255)},
	};

	const TCHAR* const VarDependencyFixtureError = TEXT("Invalid substitution, variable '--missing' not defined. In element: div#chain-inherited");

	struct FChromePixel
	{
		const TCHAR* Name;
		int32 X;
		int32 Y;
		FColor Chrome;
	};

	const FChromePixel ChromeShadowPixels[] = {
		{TEXT("shadow-sm first row below the box"), 150, 160, FColor(231, 231, 231, 255)},
		{TEXT("shadow-sm second row below the box"), 150, 161, FColor(245, 245, 245, 255)},
		{TEXT("shadow-sm third row below the box"), 150, 162, FColor(251, 251, 251, 255)},
		{TEXT("shadow-sm first column left of the box"), 99, 130, FColor(245, 245, 245, 255)},
		{TEXT("shadow-sm second column left of the box"), 98, 130, FColor(251, 251, 251, 255)},
		{TEXT("shadow-sm second column right of the box"), 201, 130, FColor(251, 251, 251, 255)},
		{TEXT("shadow-lg first row below the box"), 350, 160, FColor(224, 224, 224, 255)},
		{TEXT("shadow-lg 3 px below the box"), 350, 163, FColor(235, 235, 235, 255)},
		{TEXT("shadow-lg 5 px below the box"), 350, 165, FColor(239, 239, 239, 255)},
		{TEXT("shadow-lg 10 px below the box"), 350, 170, FColor(247, 247, 247, 255)},
		{TEXT("shadow-lg 15 px below the box"), 350, 175, FColor(252, 252, 252, 255)},
		{TEXT("shadow-lg first column left of the box"), 299, 140, FColor(246, 246, 246, 255)},
		{TEXT("shadow-lg first column right of the box"), 400, 140, FColor(246, 246, 246, 255)},
		{TEXT("shadow-lg 5 px left of the box"), 295, 150, FColor(251, 251, 251, 255)},
	};

	const FChromePixel ChromeRingPixels[] = {
		{TEXT("ring-2 outside the ring"), 497, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-2 currentcolor ring outer column"), 498, 130, FColor(0, 0, 0, 255)},
		{TEXT("ring-2 currentcolor ring inner column"), 499, 130, FColor(0, 0, 0, 255)},
		{TEXT("ring-2 box"), 500, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-2 ring below the box"), 550, 161, FColor(0, 0, 0, 255)},
		{TEXT("ring-2 outside the ring below the box"), 550, 162, FColor(255, 255, 255, 255)},
		{TEXT("ring-blue-500 outside the ring"), 697, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-blue-500 ring outer column"), 698, 130, FColor(43, 127, 255, 255)},
		{TEXT("ring-blue-500 ring inner column"), 699, 130, FColor(43, 127, 255, 255)},
		{TEXT("ring-blue-500 box"), 700, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-offset-2 container outside the ring"), 915, 130, FColor(128, 128, 128, 255)},
		{TEXT("ring-offset-2 ring outer column"), 916, 130, FColor(43, 127, 255, 255)},
		{TEXT("ring-offset-2 ring inner column"), 917, 130, FColor(43, 127, 255, 255)},
		{TEXT("ring-offset-2 white offset outer column"), 918, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-offset-2 white offset inner column"), 919, 130, FColor(255, 255, 255, 255)},
		{TEXT("ring-offset-2 ring right of the box"), 1023, 130, FColor(43, 127, 255, 255)},
		{TEXT("ring-offset-2 container right of the ring"), 1025, 130, FColor(128, 128, 128, 255)},
		{TEXT("ring-offset-2 ring above the box"), 970, 96, FColor(43, 127, 255, 255)},
		{TEXT("ring-offset-2 white offset above the box"), 970, 98, FColor(255, 255, 255, 255)},
		{TEXT("ring-offset-2 ring below the box"), 970, 163, FColor(43, 127, 255, 255)},
		{TEXT("rgb(255 0 0 / 50%) ring outside the ring"), 97, 330, FColor(255, 255, 255, 255)},
		{TEXT("rgb(255 0 0 / 50%) ring outer column"), 98, 330, FColor(255, 127, 127, 255)},
		{TEXT("rgb(255 0 0 / 50%) ring inner column"), 99, 330, FColor(255, 127, 127, 255)},
		{TEXT("rgb(255 0 0 / 50%) ring box"), 100, 330, FColor(255, 255, 255, 255)},
		{TEXT("rgb(255 0 0 / 50%) ring below the box"), 150, 361, FColor(255, 127, 127, 255)},
		{TEXT("rgb(255 0 0 / 50%) outside the ring below the box"), 150, 362, FColor(255, 255, 255, 255)},
	};

	const FChromePixel ChromeVarUndefinedPixels[] = {
		{TEXT("chain-inherited fallback"), 20, 5, FColor(128, 255, 0, 255)},
	};

	const FChromePixel ChromeVarDefinedLaterPixels[] = {
		{TEXT("chain-inherited defined variable"), 20, 5, FColor(64, 128, 192, 255)},
	};

	constexpr int32 VarDependencyPixelTolerance = 1;
	constexpr int32 RingTolerance = 2;
	constexpr int32 ShadowTolerance = 2;

	FString ColourSyntaxPath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/ColourShadow/ColourSyntaxTest.rml"));
	}

	FString VarKeywordPath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/ColourShadow/VarKeywordTest.rml"));
	}

	FString VarDependencyPath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/ColourShadow/VarDependencyTest.rml"));
	}

	FString TailwindShadowPath()
	{
		return ArcRmlUITestHelpers::MakePath(TEXT("CssParity/ColourShadow/TailwindShadowTest.rml"));
	}

	void DropCompiledFixtures()
	{
		FArcRmlTestView::DropCompiledDocument(ColourSyntaxPath());
		FArcRmlTestView::DropCompiledDocument(TailwindShadowPath());
		FArcRmlTestView::DropCompiledDocument(VarKeywordPath());
		FArcRmlTestView::DropCompiledDocument(VarDependencyPath());
		const FString Dir = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CssParity/ColourShadow"));
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("ColourSyntax.rcssc")), false, false, true);
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("TailwindShadow.rcssc")), false, false, true);
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("VarKeyword.rcssc")), false, false, true);
		IFileManager::Get().Delete(*FPaths::Combine(Dir, TEXT("VarDependency.rcssc")), false, false, true);
		FArcRmlTestView::ClearDocumentCaches();
	}

	bool ParseHex(const FString& Text, FIntVector4& OutColour)
	{
		if (Text.Len() != 9 || !Text.StartsWith(TEXT("#")))
		{
			return false;
		}
		OutColour = FIntVector4(FParse::HexNumber(*Text.Mid(1, 2)), FParse::HexNumber(*Text.Mid(3, 2)), FParse::HexNumber(*Text.Mid(5, 2)),
			FParse::HexNumber(*Text.Mid(7, 2)));
		return true;
	}

	FString DescribeSwatchMismatches(const TArray<FArcRmlTestLayoutElement>& Layout, TConstArrayView<FChromeSwatch> Swatches)
	{
		FString Failures;
		for (const FChromeSwatch& Swatch : Swatches)
		{
			const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Layout, Swatch.Id);
			FIntVector4 Colour(-1, -1, -1, -1);
			if (!Element || !ParseHex(Element->BackgroundColor, Colour))
			{
				Failures += FString::Printf(TEXT("[%s has no background colour] "), Swatch.Id);
				continue;
			}
			const FIntVector4 Difference = Colour - Swatch.Chrome;
			if (FMath::Abs(Difference.X) > Swatch.Tolerance || FMath::Abs(Difference.Y) > Swatch.Tolerance || FMath::Abs(Difference.Z) > Swatch.Tolerance
				|| FMath::Abs(Difference.W) > Swatch.Tolerance)
			{
				Failures += FString::Printf(TEXT("[%s is %s, Chrome %d,%d,%d,%d] "), Swatch.Id, *Element->BackgroundColor, Swatch.Chrome.X, Swatch.Chrome.Y,
					Swatch.Chrome.Z, Swatch.Chrome.W);
			}
		}
		return Failures;
	}

	FString DescribePixelMismatches(FArcRmlTestView& View, TConstArrayView<FChromePixel> Pixels, int32 Tolerance)
	{
		TArray<FString> Mismatches;
		for (const FChromePixel& Pixel : Pixels)
		{
			const FColor Measured = View.GetPixel(Pixel.X, Pixel.Y);
			if (FMath::Abs(Measured.R - Pixel.Chrome.R) > Tolerance || FMath::Abs(Measured.G - Pixel.Chrome.G) > Tolerance
				|| FMath::Abs(Measured.B - Pixel.Chrome.B) > Tolerance || FMath::Abs(Measured.A - Pixel.Chrome.A) > Tolerance)
			{
				Mismatches.Add(FString::Printf(TEXT("%s (%d,%d): measured %d,%d,%d chrome %d,%d,%d"), Pixel.Name, Pixel.X, Pixel.Y, Measured.R, Measured.G, Measured.B,
					Pixel.Chrome.R, Pixel.Chrome.G, Pixel.Chrome.B));
			}
		}
		return FString::Join(Mismatches, TEXT(" | "));
	}
}

TEST_CLASS(ArcRmlCssParityColourSyntaxTests, "ArcRmlUI.CssParity.ColourSyntax")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		DropCompiledFixtures();
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, ColourSyntaxPath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(SpaceSeparatedColourDeclarations_AreValidValues)
	{
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(SpaceSeparatedRgbHslAndAlpha_ComputeTheColoursAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		const FString Failures = DescribeSwatchMismatches(Layout, ChromeSwatches);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};

TEST_CLASS(ArcRmlCssParityVarKeywordTests, "ArcRmlUI.CssParity.VarKeyword")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Layout;

	BEFORE_EACH()
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		DropCompiledFixtures();
		for (const FExpectedError& Error : VarKeywordFixtureErrors)
		{
			TestRunner->AddExpectedErrorPlain(Error.Text, EAutomationExpectedErrorFlags::Contains, Error.Count);
		}
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, VarKeywordPath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(CssWideKeywordCustomProperties_TakeTheFallbackOrTheParentValueAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		const FString Failures = DescribeSwatchMismatches(Layout, ChromeVarKeywordSwatches);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}

	TEST_METHOD(CustomPropertiesWithAnUndefinedVarOrInACycle_AreGuaranteedInvalidForTheirReadersAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		const FString Failures = DescribeSwatchMismatches(Layout, ChromeInvalidVarSwatches);
		ASSERT_THAT(IsTrue(Failures.IsEmpty(), Failures));
	}
};

TEST_CLASS(ArcRmlCssParityVarDependencyTests, "ArcRmlUI.CssParity.VarDependency")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(DefiningAndRemovingTheMissingVariableOfAnInheritedCustomPropertyAtRuntime_RestylesItsReaderAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		DropCompiledFixtures();
		TestRunner->AddExpectedErrorPlain(VarDependencyFixtureError, EAutomationExpectedErrorFlags::Contains, 2);
		TArray<FArcRmlTestLayoutElement> Layout;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, VarDependencyPath()) && View.FlushUntilSettled() && View.GetProbe().bHasDocument && View.ParseLayout(Layout),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Undefined = DescribeSwatchMismatches(Layout, ChromeVarUndefined);
		ASSERT_THAT(IsTrue(Undefined.IsEmpty(), Undefined));
		View.SetRootClass(TEXT("defined"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Defined = DescribeSwatchMismatches(Layout, ChromeVarDefinedLater);
		ASSERT_THAT(IsTrue(Defined.IsEmpty(), Defined));
		View.SetRootClass(TEXT("defined"), false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.ParseLayout(Layout), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString UndefinedAgain = DescribeSwatchMismatches(Layout, ChromeVarUndefined);
		ASSERT_THAT(IsTrue(UndefinedAgain.IsEmpty(), UndefinedAgain));
	}
};

TEST_CLASS(ArcRmlCssParityVarDependencyPixelTests, "ArcRmlUI.Pixels.VarDependency")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	TEST_METHOD(DefiningAndRemovingTheMissingVariableOfAnInheritedCustomPropertyAtRuntime_RepaintsItsReaderAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		DropCompiledFixtures();
		TestRunner->AddExpectedErrorPlain(VarDependencyFixtureError, EAutomationExpectedErrorFlags::Contains, 2);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, VarDependencyPath()) && View.FlushUntilSettled() && View.RenderPixels(),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeVarUndefinedPixels, VarDependencyPixelTolerance)));
		View.SetRootClass(TEXT("defined"), true);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.RenderPixels(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeVarDefinedLaterPixels, VarDependencyPixelTolerance)));
		View.SetRootClass(TEXT("defined"), false);
		ASSERT_THAT(IsTrue(View.FlushUntilSettled() && View.RenderPixels(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeVarUndefinedPixels, VarDependencyPixelTolerance)));
	}
};

TEST_CLASS(ArcRmlCssParityTailwindShadowPixelTests, "ArcRmlUI.Pixels.TailwindShadow")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndRender()
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		DropCompiledFixtures();
		return ArcRmlUITestHelpers::OpenView(View, TailwindShadowPath()) && View.FlushUntilSettled() && View.RenderPixels()
			&& View.GetPixelSize() == ArcRmlUITestHelpers::ViewSize;
	}

	TEST_METHOD(ShadowSmAndShadowLg_ReadBackAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenAndRender(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeShadowPixels, ShadowTolerance)));
	}

	TEST_METHOD(Ring2SpaceSeparatedRgbRingColourAndRingOffset2_ReadBackAsChrome)
	{
		using namespace ArcRmlCssParityColourShadowTestsPrivate;
		if (!ArcRmlUITestHelpers::CanRenderPixels(*TestRunner))
		{
			return;
		}
		ASSERT_THAT(IsTrue(OpenAndRender(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(), DescribePixelMismatches(View, ChromeRingPixels, RingTolerance)));
	}
};
