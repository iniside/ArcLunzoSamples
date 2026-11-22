// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"

namespace ArcRmlTintTestsPrivate
{
	const FColor Red(255, 0, 0, 255);
	const FColor Green(0, 255, 0, 255);
	const FColor Blue(0, 0, 255, 255);
	const FColor White(255, 255, 255, 255);
	const FColor Cyan(0, 255, 255, 255);
	const FColor Yellow(255, 255, 0, 255);
	const FColor Magenta(255, 0, 255, 255);
	const FColor Orange(255, 128, 0, 255);
	const FColor RedHalf(128, 0, 0, 128);
	const FColor GreenHalf(0, 128, 0, 128);
	const FColor BlueHalf(0, 0, 128, 128);
	const FColor WhiteHalf(128, 128, 128, 128);

	bool IsNearColour(const FColor& A, const FColor& B)
	{
		return FMath::Abs(int32(A.R) - int32(B.R)) <= 1 && FMath::Abs(int32(A.G) - int32(B.G)) <= 1 && FMath::Abs(int32(A.B) - int32(B.B)) <= 1
			&& FMath::Abs(int32(A.A) - int32(B.A)) <= 1;
	}

	int32 CountColour(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour, bool bTextured)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && Draw.bTextured == bTextured && IsNearColour(Draw.FirstColourPremultiplied, Colour))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountVisibleText(const TArray<FArcRmlTestDraw>& Draws)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && Draw.bTextured && Draw.FirstColourPremultiplied.A > 1)
			{
				++Count;
			}
		}
		return Count;
	}

	FString DescribeDraws(const TArray<FArcRmlTestDraw>& Draws)
	{
		TArray<FString> Parts;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture)
			{
				Parts.Add(FString::Printf(TEXT("%s(%d,%d,%d,%d)"), Draw.bTextured ? TEXT("T") : TEXT("S"), Draw.FirstColourPremultiplied.R, Draw.FirstColourPremultiplied.G,
					Draw.FirstColourPremultiplied.B, Draw.FirstColourPremultiplied.A));
			}
		}
		return FString::Join(Parts, TEXT(" "));
	}

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}
}

TEST_CLASS(ArcRmlTintTests, "ArcRmlUI.Tint")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlTintTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
	}

	bool OpenTint()
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Tint/Tint.rml"));
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ResetPerfCounters();
	}

	bool SetClassAndRender(const TCHAR* Class, bool bSet)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(Class, bSet);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
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

	bool Advance(double Seconds)
	{
		ArcRmlTintTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.3f"), Seconds));
		return SettleFrames();
	}

	void ExpectNoGeometry(const TCHAR* Moment)
	{
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::ElementsRendered) > 0, FString::Printf(TEXT("%s: no render counted"), Moment)));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::GeometriesMade), FString::Printf(TEXT("%s: geometries made"), Moment)));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::BackgroundBorderRegens), FString::Printf(TEXT("%s: background and border regens"), Moment)));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::TextGeometryRegens), FString::Printf(TEXT("%s: text regens"), Moment)));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::TextShadowRegens), FString::Printf(TEXT("%s: text shadow regens"), Moment)));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::FontEffectsRegens), FString::Printf(TEXT("%s: font effect regens"), Moment)));
	}

	void ExpectSolid(const TCHAR* Moment, const FColor& Colour, int32 Expected)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(AreEqual(Expected, CountColour(View.GetDraws(), Colour, false),
			FString::Printf(TEXT("%s: solid draws of (%d,%d,%d,%d): %s"), Moment, Colour.R, Colour.G, Colour.B, Colour.A, *DescribeDraws(View.GetDraws()))));
	}

	void ExpectTextLayers(const TCHAR* Moment, const FColor& Colour)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(IsTrue(CountColour(View.GetDraws(), Colour, true) >= 3,
			FString::Printf(TEXT("%s: fill, shadow and stroke draw (%d,%d,%d,%d): %s"), Moment, Colour.R, Colour.G, Colour.B, Colour.A, *DescribeDraws(View.GetDraws()))));
	}

	TEST_METHOD(ColourClassChange_DrawsTheNewColoursWithoutGeometry)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(IsTrue(OpenTint(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSolid(TEXT("baseline red"), Red, 1);
		ExpectSolid(TEXT("baseline green"), Green, 1);
		ExpectSolid(TEXT("baseline blue"), Blue, 1);
		ExpectTextLayers(TEXT("baseline"), White);

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("alt"), true)));
		ExpectSolid(TEXT("alt background"), Cyan, 1);
		ExpectSolid(TEXT("alt border"), Yellow, 1);
		ExpectSolid(TEXT("alt outline"), Magenta, 1);
		ExpectSolid(TEXT("alt old red"), Red, 0);
		ExpectSolid(TEXT("alt old green"), Green, 0);
		ExpectSolid(TEXT("alt old blue"), Blue, 0);
		ExpectTextLayers(TEXT("alt text follows color in fill, shadow and stroke"), Orange);
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), White, true), DescribeDraws(View.GetDraws())));
		ExpectNoGeometry(TEXT("colour change"));

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("alt"), false)));
		ExpectSolid(TEXT("restored background"), Red, 1);
		ExpectSolid(TEXT("restored border"), Green, 1);
		ExpectSolid(TEXT("restored outline"), Blue, 1);
		ExpectTextLayers(TEXT("restored text"), White);
		ExpectNoGeometry(TEXT("colour change back"));
	}

	TEST_METHOD(OpacityClassChange_DrawsHalfTintsWithoutGeometry)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(IsTrue(OpenTint(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("half"), true)));
		ExpectSolid(TEXT("half background"), RedHalf, 1);
		ExpectSolid(TEXT("half border"), GreenHalf, 1);
		ExpectSolid(TEXT("half outline"), BlueHalf, 1);
		ExpectSolid(TEXT("half old red"), Red, 0);
		ExpectTextLayers(TEXT("half text"), WhiteHalf);
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), White, true), DescribeDraws(View.GetDraws())));
		ExpectNoGeometry(TEXT("opacity change"));

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("half"), false)));
		ExpectSolid(TEXT("full background"), Red, 1);
		ExpectSolid(TEXT("full border"), Green, 1);
		ExpectSolid(TEXT("full outline"), Blue, 1);
		ExpectTextLayers(TEXT("full text"), White);
		ExpectNoGeometry(TEXT("opacity change back"));
	}

	TEST_METHOD(TransparentToVisibleBackground_BuildsGeometry)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(IsTrue(OpenTint(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("clear"), true)));
		ExpectSolid(TEXT("transparent background"), Red, 0);
		ExpectSolid(TEXT("transparent background keeps the border"), Green, 1);
		ASSERT_THAT(AreEqual(0, CountVisibleText(View.GetDraws()), TEXT("transparent text draws something: ") + DescribeDraws(View.GetDraws())));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("clear"), false)));
		ExpectSolid(TEXT("visible background"), Red, 1);
		ExpectTextLayers(TEXT("visible text"), White);
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::GeometriesMade) > 0, TEXT("no geometry for the background and text that became visible")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::BackgroundBorderRegens) > 0, TEXT("background not rebuilt")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::TextGeometryRegens) == 0, TEXT("text rebuilt although its colour is only a tint")));
	}

	TEST_METHOD(VisibleToTransparentBackground_RebuildsGeometry)
	{
		using namespace ArcRmlTintTestsPrivate;
		ASSERT_THAT(IsTrue(OpenTint(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(SetClassAndRender(TEXT("clear"), true)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::BackgroundBorderRegens) > 0, TEXT("background not rebuilt")));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::TextGeometryRegens) == 0, TEXT("text rebuilt although its colour is only a tint")));
	}

	TEST_METHOD(AnimatedColoursAndOpacity_DrawTheAnimatedValueWithoutGeometry)
	{
		using namespace ArcRmlTintTestsPrivate;
		RunConsole(TEXT("ArcRml.Clock 1000"));
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Tint/TintAnim.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && SettleFrames() && View.GetProbe().bHasDocument,
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));

		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectSolid(TEXT("half way background"), FColor(128, 0, 128, 255), 1);
		ExpectSolid(TEXT("half way border"), FColor(0, 128, 128, 255), 1);
		ExpectSolid(TEXT("half way opacity"), RedHalf, 1);
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), FColor(128, 128, 255, 255), true), DescribeDraws(View.GetDraws())));
		ExpectNoGeometry(TEXT("half way"));

		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectSolid(TEXT("three quarters background"), FColor(64, 0, 191, 255), 1);
		ExpectSolid(TEXT("three quarters border"), FColor(0, 64, 191, 255), 1);
		ExpectSolid(TEXT("three quarters opacity"), FColor(64, 0, 0, 64), 1);
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), FColor(64, 64, 255, 255), true), DescribeDraws(View.GetDraws())));
		ExpectNoGeometry(TEXT("three quarters"));
	}
};
