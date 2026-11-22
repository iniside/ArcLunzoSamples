// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"

namespace ArcRmlAnimationTestsPrivate
{
	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
	}

	FString ToHex(const FIntVector4& Colour)
	{
		return FString::Printf(TEXT("#%02x%02x%02x%02x"), Colour.X, Colour.Y, Colour.Z, Colour.W);
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

	bool IsNear(const FIntVector4& A, const FIntVector4& B)
	{
		return FMath::Abs(A.X - B.X) <= 1 && FMath::Abs(A.Y - B.Y) <= 1 && FMath::Abs(A.Z - B.Z) <= 1 && A.W == B.W;
	}
}

TEST_CLASS(ArcRmlAnimationTests, "ArcRmlUI.Animation")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlAnimationTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
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

	bool OpenFrozen(const TCHAR* RelativePath)
	{
		ArcRmlAnimationTestsPrivate::RunConsole(TEXT("ArcRml.Clock 1000"));
		return ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) && SettleFrames() && View.GetProbe().bHasDocument;
	}

	bool Advance(double Seconds)
	{
		ArcRmlAnimationTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.3f"), Seconds));
		return SettleFrames();
	}

	void ExpectColour(const TCHAR* Moment, const TCHAR* DomId, const FIntVector4& Expected)
	{
		using namespace ArcRmlAnimationTestsPrivate;
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseHex(Element->Color, Colour), Element->Color));
		ASSERT_THAT(IsTrue(IsNear(Colour, Expected), FString::Printf(TEXT("%s %s color is %s, Chrome %s"), Moment, DomId, *Element->Color, *ToHex(Expected))));
	}

	TEST_METHOD(ColourKeyframes_InterpolateInSrgbOverTheWholeClockJump)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/ColorKeyframesTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("at the start"), TEXT("tint"), FIntVector4(255, 32, 32, 255));
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectColour(TEXT("after 1 s"), TEXT("tint"), FIntVector4(199, 88, 88, 255));
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectColour(TEXT("after 3 s"), TEXT("tint"), FIntVector4(88, 199, 199, 255));
	}

	void ExpectTransformedX(const TCHAR* DomId, float Expected, const TCHAR* Why)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Element->TransformedRect.Position.X, Expected, 0.6f),
			FString::Printf(TEXT("%s transformed x is %.2f, Chrome %.2f: %s"), DomId, Element->TransformedRect.Position.X, Expected, Why)));
	}

	TEST_METHOD(TranslateMixingPercentAndPx_ResolvesThePercentAgainstTheCurrentBox)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/TranslateMixTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectTransformedX(TEXT("mix"), 112.5f, TEXT("calc(-25% + 50px) of a 150 px box"));
	}

	int32 ReadFontLayers(const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		if (!View.ParseLayout(Elements))
		{
			return -1;
		}
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		if (!Element)
		{
			return -1;
		}
		const FArcRmlTestLayoutElement* Text = Elements.FindByPredicate([Element](const FArcRmlTestLayoutElement& Candidate) -> bool
		{
			return Candidate.Parent == Element->Path && Candidate.Tag == TEXT("#text");
		});
		return Text ? Text->FontLayers : -1;
	}

	TEST_METHOD(AnimatedColourOnEffectText_KeepsTheFontLayerCount)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/FontLayerTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Advance(0.1)));
		const int32 StrokeLayers = ReadFontLayers(TEXT("fl-stroke"));
		const int32 ShadowLayers = ReadFontLayers(TEXT("fl-shadow"));
		ASSERT_THAT(IsTrue(StrokeLayers > 1 && ShadowLayers > 1, FString::Printf(TEXT("stroke %d, outline %d layers"), StrokeLayers, ShadowLayers)));
		for (int32 Step = 0; Step < 20; ++Step)
		{
			ASSERT_THAT(IsTrue(Advance(0.25)));
		}
		ASSERT_THAT(AreEqual(StrokeLayers, ReadFontLayers(TEXT("fl-stroke")), TEXT("every colour of an animated text stroke kept a font layer")));
		ASSERT_THAT(AreEqual(ShadowLayers, ReadFontLayers(TEXT("fl-shadow")), TEXT("every colour of an animated currentcolor outline kept a font layer")));
	}

	TEST_METHOD(DecomposedTransformWithPercent_ResolvesThePercentEachFrame)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/TranslateMixTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.SetRootClass(TEXT("wide"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectTransformedX(TEXT("decomposed"), 62.5f, TEXT("half of -50% of the box, which grew to 150 px after the animation started"));
	}

	void ExpectWidth(const TCHAR* Moment, const TCHAR* DomId, float Expected)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Element->Rect.Size.X, Expected, 1.0f),
			FString::Printf(TEXT("%s %s width is %.2f, Chrome %.2f"), Moment, DomId, Element->Rect.Size.X, Expected)));
	}

	TEST_METHOD(ElementShownAgainAfterDisplayNone_RestartsItsAnimationFromTheStart)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/DisplayNoneRestartTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectWidth(TEXT("at the start"), TEXT("grow"), 100.0f);
		ASSERT_THAT(IsTrue(Advance(2.0)));
		ExpectWidth(TEXT("after 2 s"), TEXT("grow"), 200.0f);

		View.SetRootClass(TEXT("hide"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(Advance(1.0)));

		View.SetRootClass(TEXT("hide"), false);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ExpectWidth(TEXT("shown again"), TEXT("grow"), 100.0f);
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectWidth(TEXT("1 s after it was shown again"), TEXT("grow"), 150.0f);
	}

	int64 StyleComputesOfAnAdvance()
	{
		if (!Assert.IsTrue(View.ResetPerfCounters()) || !Assert.IsTrue(Advance(0.5)))
		{
			return -1;
		}
		return View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes);
	}

	TEST_METHOD(ElementHiddenInTheMiddleOfItsAnimation_ComputesNoStyleWhileTheDriverAnimates)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/DisplayNoneDriverTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(Advance(1.0)));
		View.SetRootClass(TEXT("hide"), true);
		ASSERT_THAT(IsTrue(SettleFrames()));
		const int64 WhileHidden = StyleComputesOfAnAdvance();

		View.SetRootClass(TEXT("hide"), false);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ASSERT_THAT(IsTrue(Advance(5.0)));
		ExpectWidth(TEXT("after its animation finished"), TEXT("grow"), 300.0f);
		const int64 Finished = StyleComputesOfAnAdvance();
		ASSERT_THAT(IsTrue(Finished > 0, TEXT("the driver animation computed no style on an advance, so nothing drives an update")));
		ASSERT_THAT(AreEqual(Finished, WhileHidden,
			TEXT("the animation of the hidden #grow computed its style on an advance, Chrome cancels an animation whose element becomes display: none")));
	}

	TEST_METHOD(ElementOpenedAsDisplayNone_ComputesNoStyleAndStartsFromTheStartWhenShown)
	{
		ASSERT_THAT(IsTrue(OpenFrozen(TEXT("Anim/DisplayNoneOpenHiddenTest.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const int64 WhileHidden = StyleComputesOfAnAdvance();
		ASSERT_THAT(IsTrue(Advance(1.5)));

		View.SetRootClass(TEXT("hide"), false);
		ASSERT_THAT(IsTrue(SettleFrames()));
		ExpectWidth(TEXT("shown 2 s after the open"), TEXT("grow"), 100.0f);
		ASSERT_THAT(IsTrue(Advance(1.0)));
		ExpectWidth(TEXT("1 s after it was shown"), TEXT("grow"), 150.0f);

		ASSERT_THAT(IsTrue(Advance(4.0)));
		const int64 Finished = StyleComputesOfAnAdvance();
		ASSERT_THAT(IsTrue(Finished > 0, TEXT("the driver animation computed no style on an advance, so nothing drives an update")));
		ASSERT_THAT(AreEqual(Finished, WhileHidden,
			TEXT("the animation of #grow, opened as display: none, computed its style on an advance, Chrome starts no animation in display: none")));
	}
};
