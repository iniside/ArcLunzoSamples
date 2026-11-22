// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ArcRmlCssParityAnimationTestsPrivate
{
	const TCHAR* const LonghandsPath = TEXT("CssParity/Anim/Longhands.rml");
	const TCHAR* const ParseAllPath = TEXT("CssParity/Anim/ParseAll.rml");
	const TCHAR* const ParseInvalidPath = TEXT("CssParity/Anim/ParseInvalid.rml");
	const TCHAR* const ReversePath = TEXT("CssParity/Anim/Reverse.rml");
	const TCHAR* const CountsPath = TEXT("CssParity/Anim/Counts.rml");
	const TCHAR* const VarEndPath = TEXT("CssParity/Anim/VarEnd.rml");
	const TCHAR* const ShorthandsPath = TEXT("CssParity/Anim/Shorthands.rml");
	const TCHAR* const OrderValuesPath = TEXT("CssParity/Anim/OrderValues.rml");

	void RunConsole(const FString& Command)
	{
		IConsoleManager::Get().ProcessUserConsoleInput(*Command, *GLog, nullptr);
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

	FString ToText(const FIntVector4& Colour)
	{
		return FString::Printf(TEXT("rgba(%d, %d, %d, %d)"), Colour.X, Colour.Y, Colour.Z, Colour.W);
	}

	void DropCompiledFixture(const TCHAR* RelativeRml)
	{
		FArcRmlTestView::DropCompiledDocument(ArcRmlUITestHelpers::MakePath(RelativeRml));
		const FString Dir = ArcRmlUITestHelpers::GetTestContentDir();
		const FString Sheet = FPaths::Combine(Dir, FPaths::GetPath(RelativeRml), FPaths::GetBaseFilename(RelativeRml) + TEXT(".rcssc"));
		IFileManager::Get().Delete(*Sheet, false, true, true);
	}

	bool ReadStringFields(const FString& LayoutJson, const FString& DomId, FString& OutDisplay, FString& OutVisibility)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(LayoutJson);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Elements = nullptr;
		if (!Root->TryGetArrayField(TEXT("elements"), Elements))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Elements)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FString Id;
			if (Value->TryGetObject(Object) && (*Object)->TryGetStringField(TEXT("dom_id"), Id) && Id == DomId)
			{
				return (*Object)->TryGetStringField(TEXT("display"), OutDisplay) && (*Object)->TryGetStringField(TEXT("visibility"), OutVisibility);
			}
		}
		return false;
	}
}

TEST_CLASS(ArcRmlCssParityAnimationTests, "ArcRmlUI.CssParity.Animation")
{
	FArcRmlTestView View;
	double Elapsed = 0.0;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlCssParityAnimationTestsPrivate::RunConsole(TEXT("ArcRml.Clock live"));
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

	bool StartFixture(const TCHAR* RelativePath, bool bSetRootClass)
	{
		using namespace ArcRmlCssParityAnimationTestsPrivate;
		DropCompiledFixture(RelativePath);
		RunConsole(TEXT("ArcRml.Clock 1000"));
		Elapsed = 0.0;
		if (!ArcRmlUITestHelpers::OpenView(View, ArcRmlUITestHelpers::MakePath(RelativePath)) || !SettleFrames() || !View.GetProbe().bHasDocument)
		{
			return false;
		}
		if (bSetRootClass)
		{
			View.SetRootClass(TEXT("go"), true);
			return SettleFrames();
		}
		return true;
	}

	bool AdvanceTo(double Seconds)
	{
		ArcRmlCssParityAnimationTestsPrivate::RunConsole(FString::Printf(TEXT("ArcRml.ClockAdvance %.4f"), Seconds - Elapsed));
		Elapsed = Seconds;
		return SettleFrames();
	}

	void ExpectColour(const TCHAR* DomId, double Seconds, bool bBackground, const FIntVector4& Chrome)
	{
		using namespace ArcRmlCssParityAnimationTestsPrivate;
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		const FString& Text = bBackground ? Element->BackgroundColor : Element->Color;
		FIntVector4 Colour;
		ASSERT_THAT(IsTrue(ParseHex(Text, Colour), Text));
		ASSERT_THAT(IsTrue(IsNear(Colour, Chrome),
			FString::Printf(TEXT("%s %s at %.2f s is %s, Chrome %s"), DomId, bBackground ? TEXT("background-color") : TEXT("color"), Seconds, *Text, *ToText(Chrome))));
	}

	void ExpectSize(const TCHAR* DomId, double Seconds, float ChromeWidth, float ChromeHeight)
	{
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Element->Rect.Size.X, ChromeWidth, 0.5f) && FMath::IsNearlyEqual(Element->Rect.Size.Y, ChromeHeight, 0.5f),
			FString::Printf(TEXT("%s at %.2f s is %.2f x %.2f, Chrome %.2f x %.2f"), DomId, Seconds, Element->Rect.Size.X, Element->Rect.Size.Y, ChromeWidth, ChromeHeight)));
	}

	void ExpectDisplayAndVisibility(const TCHAR* DomId, double Seconds, const TCHAR* ChromeDisplay, const TCHAR* ChromeVisibility)
	{
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		FString Display;
		FString Visibility;
		ASSERT_THAT(IsTrue(ArcRmlCssParityAnimationTestsPrivate::ReadStringFields(View.GetProbe().LayoutJson, DomId, Display, Visibility)));
		ASSERT_THAT(IsTrue(Display == ChromeDisplay && Visibility == ChromeVisibility,
			FString::Printf(TEXT("%s at %.2f s is display %s visibility %s, Chrome %s %s"), DomId, Seconds, *Display, *Visibility, ChromeDisplay, ChromeVisibility)));
	}

	void ExpectOpacity(const TCHAR* DomId, double Seconds, float Chrome, float AbsentValue)
	{
		ASSERT_THAT(IsTrue(AdvanceTo(Seconds)));
		ExpectOpacityNow(DomId, Chrome, AbsentValue);
	}

	void ExpectOpacityNow(const TCHAR* DomId, float Chrome, float AbsentValue)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element));
		const FString* Running = Element->Animated.Find(TEXT("opacity"));
		const float Value = Running ? FCString::Atof(**Running) : AbsentValue;
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Value, Chrome, 0.02f),
			FString::Printf(TEXT("%s at %.3f s has opacity %.4f (%s), Chrome %.4f"), DomId, Elapsed, Value, Running ? **Running : TEXT("no animated value"), Chrome)));
	}

	bool HasAnimatedOpacity(const TCHAR* DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		const FArcRmlTestLayoutElement* Element = View.ParseLayout(Elements) ? ArcRmlUITestHelpers::FindByDomId(Elements, DomId) : nullptr;
		return Element && Element->Animated.Contains(TEXT("opacity"));
	}

	bool SetClass(const TCHAR* Class, bool bSet)
	{
		View.SetRootClass(Class, bSet);
		return SettleFrames();
	}

	TEST_METHOD(DurationLonghandAfterShorthand_OverridesTheShorthandDuration)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("a-short"), 0.1, false, FIntVector4(230, 0, 26, 255));
		ExpectColour(TEXT("a-short"), 0.5, false, FIntVector4(128, 0, 128, 255));
		ExpectColour(TEXT("a-short"), 1.0, false, FIntVector4(0, 0, 255, 255));
	}

	TEST_METHOD(ThreePropertiesTwoDurations_RepeatTheDurationList)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("b-list"), 0.5, false, FIntVector4(128, 0, 128, 255));
		ExpectColour(TEXT("b-list"), 0.5, true, FIntVector4(43, 213, 0, 255));
		ExpectSize(TEXT("b-list"), 0.5, 150.0f, 20.0f);
		ExpectColour(TEXT("b-list"), 1.5, false, FIntVector4(0, 0, 255, 255));
		ExpectColour(TEXT("b-list"), 1.5, true, FIntVector4(128, 128, 0, 255));
		ExpectSize(TEXT("b-list"), 1.5, 200.0f, 20.0f);
	}

	TEST_METHOD(AllAndColourInOneList_TheLastEntryWinsForColour)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("c-all"), 0.5, false, FIntVector4(213, 0, 43, 255));
		ExpectSize(TEXT("c-all"), 0.5, 150.0f, 20.0f);
		ExpectColour(TEXT("c-all"), 1.5, false, FIntVector4(128, 0, 128, 255));
		ExpectSize(TEXT("c-all"), 1.5, 200.0f, 20.0f);
	}

	TEST_METHOD(PausedPlayState_HoldsTheStartValueWhileARunningTwinMoves)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("f-run"), 0.5, 250.0f, 20.0f);
		ExpectSize(TEXT("f-paused"), 0.5, 200.0f, 20.0f);
		ExpectSize(TEXT("f-run"), 1.0, 300.0f, 20.0f);
		ExpectSize(TEXT("f-paused"), 1.0, 200.0f, 20.0f);
		ExpectSize(TEXT("f-run"), 1.25, 325.0f, 20.0f);
		ExpectSize(TEXT("f-paused"), 1.25, 200.0f, 20.0f);
	}

	TEST_METHOD(PausedWithNegativeDelay_HoldsTheValueAtTheDelayOffset)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("f-paused-neg"), 0.5, 250.0f, 20.0f);
		ExpectSize(TEXT("f-paused-neg"), 1.0, 250.0f, 20.0f);
	}

	TEST_METHOD(PausedWithPositiveDelayAndNoFill_KeepsTheElementsOwnWidth)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("f-paused-delay"), 0.5, 100.0f, 20.0f);
		ExpectSize(TEXT("f-paused-delay"), 1.5, 100.0f, 20.0f);
	}

	TEST_METHOD(AnimationDelayList_StartsEachAnimationAtItsOwnDelay)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("g-delay"), 0.5, 200.0f, 40.0f);
		ExpectSize(TEXT("g-delay"), 1.0, 300.0f, 40.0f);
		ExpectSize(TEXT("g-delay"), 1.5, 400.0f, 40.0f);
		ExpectSize(TEXT("g-delay"), 2.0, 400.0f, 60.0f);
	}

	TEST_METHOD(IterationCountAndAlternateDirection_ReverseTheSecondIteration)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("h-alt"), 0.5, 300.0f, 20.0f);
		ExpectSize(TEXT("h-alt"), 0.9, 380.0f, 20.0f);
		ExpectSize(TEXT("h-alt"), 1.25, 350.0f, 20.0f);
		ExpectSize(TEXT("h-alt"), 2.0, 200.0f, 20.0f);
	}

	TEST_METHOD(TransitionBehaviorNormal_SwitchesDisplayAtOnce)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectDisplayAndVisibility(TEXT("d-normal"), 0.1, TEXT("none"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("d-normal"), 0.9, TEXT("none"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("d-normal"), 1.0, TEXT("none"), TEXT("visible"));
	}

	TEST_METHOD(TransitionBehaviorAllowDiscrete_KeepsDisplayShownUntilTheEnd)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectDisplayAndVisibility(TEXT("d-discrete"), 0.1, TEXT("block"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("d-discrete"), 0.9, TEXT("block"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("d-discrete"), 1.0, TEXT("none"), TEXT("visible"));
	}

	TEST_METHOD(VisibilityTransitionUnderNormal_StaysVisibleUntilTheEnd)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::LonghandsPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectDisplayAndVisibility(TEXT("e-vis"), 0.1, TEXT("block"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("e-vis"), 0.5, TEXT("block"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("e-vis"), 0.9, TEXT("block"), TEXT("visible"));
		ExpectDisplayAndVisibility(TEXT("e-vis"), 1.0, TEXT("block"), TEXT("hidden"));
	}

	TEST_METHOD(EveryRegisteredDeclaration_ParsesWithoutADiagnostic)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::ParseAllPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetHeadDiagnostics().Num(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}

	TEST_METHOD(InvalidTransitionDeclarations_AreDroppedAndTheEarlierValueSurvives)
	{
		using namespace ArcRmlCssParityAnimationTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(ParseInvalidPath, true), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Sheet = ArcRmlUITestHelpers::MakePath(ParseInvalidPath).Replace(TEXT(".rml"), TEXT(".rcss"));
		ASSERT_THAT(AreEqual(2, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("'transition-property: none, color;' is dropped"), Sheet, 24), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("'transition: color 1s 0.5;' is dropped"), Sheet, 29), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectSize(TEXT("inv-prop"), 0.5, 150.0f, 20.0f);
		ExpectSize(TEXT("inv-short"), 0.5, 150.0f, 20.0f);
		ExpectSize(TEXT("inv-ctrl"), 0.5, 150.0f, 20.0f);
	}

	TEST_METHOD(HoverRemoval_CancelsTheRunningTransition)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::ReversePath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("hover"), true)));
		ExpectOpacity(TEXT("hov"), 0.3, 0.743f, -1.0f);
		ASSERT_THAT(IsTrue(HasAnimatedOpacity(TEXT("hov")), TEXT("the hover transition is not running at 0.3 s")));
		ASSERT_THAT(IsTrue(SetClass(TEXT("hover"), false)));
		ExpectOpacityNow(TEXT("hov"), 1.0f, 1.0f);
		ASSERT_THAT(IsFalse(HasAnimatedOpacity(TEXT("hov")), TEXT("a transition still pins opacity after the hover class was removed")));
		ExpectOpacity(TEXT("hov"), 0.5, 1.0f, 1.0f);
		ASSERT_THAT(IsFalse(HasAnimatedOpacity(TEXT("hov"))));
	}

	TEST_METHOD(ReversedTransition_ShortensToTheElapsedFraction)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::ReversePath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), true)));
		ExpectOpacity(TEXT("rev"), 0.25, 0.75f, -1.0f);
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), false)));
		ExpectOpacityNow(TEXT("rev"), 0.75f, -1.0f);
		ExpectOpacity(TEXT("rev"), 0.375, 0.875f, -1.0f);
		ExpectOpacity(TEXT("rev"), 0.5, 1.0f, 1.0f);
		ExpectOpacity(TEXT("rev"), 0.75, 1.0f, 1.0f);
		ASSERT_THAT(IsFalse(HasAnimatedOpacity(TEXT("rev"))));
	}

	TEST_METHOD(ReversedTwice_UsesTheCombinedShorteningFactor)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::ReversePath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), true)));
		ExpectOpacity(TEXT("rev"), 0.25, 0.75f, -1.0f);
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), false)));
		ExpectOpacity(TEXT("rev"), 0.375, 0.875f, -1.0f);
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), true)));
		ExpectOpacityNow(TEXT("rev"), 0.875f, -1.0f);
		ExpectOpacity(TEXT("rev"), 0.8125, 0.4375f, -1.0f);
		ExpectOpacity(TEXT("rev"), 1.25, 0.0f, 0.0f);
	}

	TEST_METHOD(FractionalAndZeroIterationCounts_StopAtTheirFraction)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::CountsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectOpacity(TEXT("i-zero"), 0.25, 0.0f, -1.0f);
		ExpectOpacity(TEXT("i-half"), 0.25, 0.25f, -1.0f);
		ExpectOpacity(TEXT("i-alt"), 0.25, 0.25f, -1.0f);
		ExpectOpacity(TEXT("i-zero"), 0.5, 0.0f, -1.0f);
		ExpectOpacity(TEXT("i-half"), 0.5, 0.5f, -1.0f);
		ExpectOpacity(TEXT("i-alt"), 0.5, 0.5f, -1.0f);
		ExpectOpacity(TEXT("i-zero"), 1.0, 0.0f, -1.0f);
		ExpectOpacity(TEXT("i-half"), 1.0, 0.5f, -1.0f);
		ExpectOpacity(TEXT("i-alt"), 1.0, 1.0f, -1.0f);
		ExpectOpacity(TEXT("i-zero"), 2.0, 0.0f, -1.0f);
		ExpectOpacity(TEXT("i-half"), 2.0, 0.5f, -1.0f);
		ExpectOpacity(TEXT("i-alt"), 2.0, 0.5f, -1.0f);
	}

	TEST_METHOD(ZeroDurationAnimationWithForwardsFill_ShowsItsFractionalProgress)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::CountsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectOpacity(TEXT("z-half"), 0.25, 0.5f, -1.0f);
		ExpectOpacity(TEXT("z-quarter"), 0.25, 0.25f, -1.0f);
		ExpectOpacity(TEXT("z-alt"), 0.25, 0.5f, -1.0f);
		ExpectOpacity(TEXT("z-zero"), 0.25, 0.0f, -1.0f);
		ExpectOpacity(TEXT("z-two"), 0.25, 1.0f, -1.0f);
		ExpectOpacity(TEXT("z-mid"), 0.25, 1.0f, -1.0f);
		ExpectOpacity(TEXT("z-half"), 2.0, 0.5f, -1.0f);
		ExpectOpacity(TEXT("z-zero"), 2.0, 0.0f, -1.0f);
		ExpectOpacity(TEXT("z-mid"), 2.0, 1.0f, -1.0f);
	}

	TEST_METHOD(NonIntegerOrderValues_AreDroppedAndSignedIntegersAreAccepted)
	{
		using namespace ArcRmlCssParityAnimationTestsPrivate;
		ASSERT_THAT(IsTrue(StartFixture(OrderValuesPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FString Sheet = ArcRmlUITestHelpers::MakePath(OrderValuesPath).Replace(TEXT(".rml"), TEXT(".rcss"));
		ASSERT_THAT(AreEqual(3, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("'order: 1.0;' is dropped"), Sheet, 16), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("'order: 1.5;' is dropped"), Sheet, 20), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("'order: 2px;' is dropped"), Sheet, 24), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(DefinitionChangeWithTheSameResolvedEnd_KeepsTheRunningTransition)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::VarEndPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("a"), true)));
		ExpectOpacity(TEXT("v"), 0.4, 0.8f, -1.0f);
		ASSERT_THAT(IsTrue(SetClass(TEXT("b"), true)));
		ExpectOpacityNow(TEXT("v"), 0.8f, -1.0f);
		ExpectOpacity(TEXT("v"), 0.6, 0.7f, -1.0f);
		ExpectOpacity(TEXT("v"), 1.0, 0.5f, 0.5f);
	}

	TEST_METHOD(ClassRemovalWithTheTransitionStillDeclared_RunsAFreshTransitionBack)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::VarEndPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("hover"), true)));
		ExpectOpacity(TEXT("h"), 0.5, 0.75f, -1.0f);
		ExpectOpacity(TEXT("h"), 1.0, 0.5f, 0.5f);
		ASSERT_THAT(IsTrue(SetClass(TEXT("hover"), false)));
		ExpectOpacityNow(TEXT("h"), 0.5f, -1.0f);
		ExpectOpacity(TEXT("h"), 1.5, 0.75f, -1.0f);
		ExpectOpacity(TEXT("h"), 2.0, 1.0f, 1.0f);
	}

	TEST_METHOD(ShorthandItems_LandInTheirLonghands)
	{
		ASSERT_THAT(IsTrue(StartFixture(ArcRmlCssParityAnimationTestsPrivate::ShorthandsPath, false), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(SetClass(TEXT("go"), true)));
		ExpectSize(TEXT("s-trans"), 0.25, 100.0f, 20.0f);
		ExpectSize(TEXT("s-anim"), 0.25, 200.0f, 20.0f);
		ExpectSize(TEXT("s-two"), 0.25, 100.0f, 26.25f);
		ExpectSize(TEXT("s-trans"), 1.0, 150.0f, 20.0f);
		ExpectSize(TEXT("s-anim"), 1.0, 300.0f, 20.0f);
		ExpectSize(TEXT("s-two"), 1.0, 112.5f, 45.0f);
		ExpectSize(TEXT("s-trans"), 1.5, 200.0f, 20.0f);
		ExpectSize(TEXT("s-anim"), 1.5, 400.0f, 20.0f);
		ExpectSize(TEXT("s-two"), 1.5, 125.0f, 57.5f);
		ExpectSize(TEXT("s-anim"), 1.75, 350.0f, 20.0f);
		ExpectSize(TEXT("s-anim"), 2.0, 300.0f, 20.0f);
		ExpectSize(TEXT("s-two"), 2.0, 137.5f, 70.0f);
		ExpectSize(TEXT("s-anim"), 3.0, 200.0f, 20.0f);
		ExpectSize(TEXT("s-two"), 4.0, 187.5f, 120.0f);
	}
};
