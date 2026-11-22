// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

namespace ArcRmlCssParityInlineTransitionTestsPrivate
{
	const TCHAR* const FixturePath = TEXT("CssParity/Anim/InlineTransition.rml");
	const TCHAR* const ModelName = TEXT("inline");
	const FColor FadeAtHalf(127, 0, 0, 127);
	const FColor FadeAtQuarter(63, 0, 0, 63);

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	void DropCompiledFixture(const TCHAR* RelativeRml)
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(RelativeRml));
		const FString Dir = ArcRmlUITestHelpers::GetTestContentDir();
		const FString Sheet = FPaths::Combine(Dir, FPaths::GetPath(RelativeRml), FPaths::GetBaseFilename(RelativeRml) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
	}

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

TEST_CLASS(ArcRmlCssParityInlineTransitionTests, "ArcRmlUI.CssParity.InlineTransition")
{
	FArcRmlTestView View;
	double Elapsed = 0.0;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlCssParityInlineTransitionTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
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

	bool StartFixture()
	{
		using namespace ArcRmlCssParityInlineTransitionTestsPrivate;
		DropCompiledFixture(FixturePath);
		RunConsole(TEXT("ArcRml.Clock 1000"));
		Elapsed = 0.0;
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(FixturePath)) && SettleFrames() && View.GetProbe().bHasDocument;
	}

	bool AdvanceTo(double Seconds)
	{
		ArcRmlCssParityInlineTransitionTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.4f"), Seconds - Elapsed));
		Elapsed = Seconds;
		return SettleFrames();
	}

	void SetValue(const TCHAR* Field, const TCHAR* Value)
	{
		View.SetModelValue(ArcRmlCssParityInlineTransitionTestsPrivate::ModelName, ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeString(Value));
	}

	bool SetValueNow(const TCHAR* Field, const TCHAR* Value)
	{
		SetValue(Field, Value);
		return SettleFrames();
	}

	const FArcRmlTestLayoutElement* FindElement(TArray<FArcRmlTestLayoutElement>& Elements, const TCHAR* DomId)
	{
		return View.ParseLayout(Elements) ? ArcRmlUITestHelpers::FindByDomId(Elements, DomId) : nullptr;
	}

	void ExpectWidthNow(const TCHAR* DomId, float ChromeWidth)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = FindElement(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Element->Rect.Size.X, ChromeWidth, 0.5f),
			FString::Printf(TEXT("%s at %.2f s is %.2f wide, Chrome %.2f"), DomId, Elapsed, Element->Rect.Size.X, ChromeWidth)));
	}

	void ExpectWidth(const TCHAR* DomId, double Seconds, float ChromeWidth)
	{
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		ExpectWidthNow(DomId, ChromeWidth);
	}

	void ExpectAuthorWidthOnly(const TCHAR* DomId, const TCHAR* AuthorWidth)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = FindElement(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		const FString* Input = Element->Inputs.Find(TEXT("width"));
		ASSERT_THAT(IsTrue(Input && *Input == AuthorWidth,
			FString::Printf(TEXT("%s inputs width is '%s', expected '%s'"), DomId, Input ? **Input : TEXT("<none>"), AuthorWidth)));
		const FString* Animated = Element->Animated.Find(TEXT("width"));
		ASSERT_THAT(IsTrue(Animated == nullptr, FString::Printf(TEXT("%s still has the animated width '%s'"), DomId, Animated ? **Animated : TEXT(""))));
	}

	bool HasAnimatedWidth(const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = FindElement(Elements, DomId);
		return Element && Element->Animated.Contains(TEXT("width"));
	}

	TEST_METHOD(FirstStyle_ShowsTheBoundWidthWithoutATransition)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidthNow(TEXT("cls"), 100.0f);
		ASSERT_THAT(IsFalse(HasAnimatedWidth(TEXT("cls")), TEXT("the first style of cls started a width transition")));
	}

	TEST_METHOD(BoundWidthChange_TransitionsWithTheClassTransitionAndKeepsTheAuthorValue)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("cls"), TEXT("200"))));
		ExpectWidth(TEXT("cls"), 0.75, 125.0f);
		ExpectWidth(TEXT("cls"), 1.5, 200.0f);
		ExpectAuthorWidthOnly(TEXT("cls"), TEXT("200px"));
		ExpectWidth(TEXT("cls"), 2.0, 200.0f);
	}

	TEST_METHOD(TransitionAndWidthBoundInOneUpdate_UseTheNewTiming)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		SetValue(TEXT("timing"), TEXT("300"));
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("timingTransition"), TEXT("width 2s linear"))));
		ExpectWidth(TEXT("timing"), 0.5, 150.0f);
	}

	TEST_METHOD(BoundWidthChangedBackDuringTheTransition_ReversesInTheElapsedTime)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("rev"), TEXT("200"))));
		ExpectWidth(TEXT("rev"), 0.5, 150.0f);
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("rev"), TEXT("100"))));
		ExpectWidthNow(TEXT("rev"), 150.0f);
		ExpectWidth(TEXT("rev"), 0.75, 125.0f);
		ExpectWidth(TEXT("rev"), 1.0, 100.0f);
		ASSERT_THAT(IsFalse(HasAnimatedWidth(TEXT("rev"))));
	}

	TEST_METHOD(NewBoundWidthDuringTheTransition_RetargetsFromTheShownValue)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("ret"), TEXT("200"))));
		ExpectWidth(TEXT("ret"), 0.5, 150.0f);
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("ret"), TEXT("300"))));
		ExpectWidthNow(TEXT("ret"), 150.0f);
		ExpectWidth(TEXT("ret"), 1.0, 225.0f);
		ExpectWidth(TEXT("ret"), 1.5, 300.0f);
	}

	TEST_METHOD(BoundWidthWrittenDuringAKeyframeAnimation_ShowsOnlyWhenItEnds)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ASSERT_THAT(IsTrue(SetValueNow(TEXT("kf"), TEXT("80"))));
		ExpectWidthNow(TEXT("kf"), 100.0f);
		ExpectWidth(TEXT("kf"), 1.0, 200.0f);
		ExpectWidth(TEXT("kf"), 2.5, 80.0f);
		ExpectAuthorWidthOnly(TEXT("kf"), TEXT("80px"));
	}

	TEST_METHOD(InlineImportantWidth_BeatsTheRunningAnimation)
	{
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("imp"), 0.5, 60.0f);
	}

	TEST_METHOD(ImportantRuleOpacity_BeatsTheRunningAnimationInTheDraw)
	{
		using namespace ArcRmlCssParityInlineTransitionTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(AdvanceTo(0.5)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), FadeAtHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), FadeAtQuarter), DescribeColours(View.GetDraws())));
	}
};
