// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"

namespace ArcRmlRestyleValueTestsPrivate
{
	const FString Red(TEXT("#ff0000ff"));
	const FString Blue(TEXT("#0000ffff"));
	const FString Green(TEXT("#00ff00ff"));

	int32 CountColour(const TArray<FArcRmlTestDraw>& Draws, const FColor& Colour)
	{
		int32 Count = 0;
		for (const FArcRmlTestDraw& Draw : Draws)
		{
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured && Draw.FirstColourPremultiplied == Colour)
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
			if (Draw.Op == EArcRmlTestDrawOp::Geometry && !Draw.bCallbackTexture && !Draw.bTextured)
			{
				Parts.Add(FString::Printf(TEXT("(%d,%d,%d,%d)"), Draw.FirstColourPremultiplied.R, Draw.FirstColourPremultiplied.G, Draw.FirstColourPremultiplied.B,
					Draw.FirstColourPremultiplied.A));
			}
		}
		return FString::Join(Parts, TEXT(" "));
	}

	const FColor PlainHalf(127, 0, 0, 127);
	const FColor InheritHalf(0, 63, 0, 63);
	const FColor VarHalf(0, 0, 63, 63);
	const FColor PlainFull(255, 0, 0, 255);
	const FColor InheritFull(0, 255, 0, 255);
	const FColor VarFull(0, 0, 127, 127);
}

TEST_CLASS(ArcRmlRestyleValueTests, "ArcRmlUI.Restyle")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;

	AFTER_EACH()
	{
		View.Close();
	}

	bool Open(const TCHAR* RelativePath)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(RelativePath);
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.FlushUntilSettled() && View.GetProbe().bHasDocument
			&& View.ParseLayout(Elements);
	}

	bool Read()
	{
		return View.Flush() && View.Flush() && View.ParseLayout(Elements);
	}

	bool SetFlag(const TCHAR* Field, bool bValue)
	{
		View.SetModelValue(TEXT("rv"), ArcRmlUITestHelpers::MakeFieldPath(Field), ArcRmlUITestHelpers::MakeBool(bValue));
		return Read();
	}

	void ExpectColour(const TCHAR* Moment, const TCHAR* DomId, const FString& Expected)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, DomId);
		ASSERT_THAT(IsNotNull(Element, FString::Printf(TEXT("%s: no element %s"), Moment, DomId)));
		ASSERT_THAT(AreEqual(Expected, Element->Color, FString::Printf(TEXT("%s: color of %s"), Moment, DomId)));
	}

	void ExpectBorderTop(const TCHAR* Moment, const FString& Expected)
	{
		const FArcRmlTestLayoutElement* Element = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sh"));
		ASSERT_THAT(IsNotNull(Element, Moment));
		ASSERT_THAT(AreEqual(Expected, Element->BorderTopColor, FString::Printf(TEXT("%s: border-top color of sh"), Moment)));
	}

	bool SetFullParentOpacity(bool bFull)
	{
		const uint64 Before = View.GetProbe().FrameSerial;
		View.SetRootClass(TEXT("full"), bFull);
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			if (!View.FlushUntilSettled())
			{
				return false;
			}
		}
		return View.GetProbe().FrameSerial > Before;
	}

	TEST_METHOD(CustomPropertyChangedToEqualValue_KeepsDescendantsAndVarShorthands)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Vars.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline"), TEXT("a"), Red);
		ExpectColour(TEXT("baseline"), TEXT("deep"), Red);
		ExpectBorderTop(TEXT("baseline"), Red);

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetFlag(TEXT("Same"), true)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::DefinitionsChangedEqualRules) > 0, TEXT("the equal rule did not change a definition")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesVariables), TEXT("descendants restyled for an equal variable")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes), TEXT("styles computed for an equal definition")));
		ExpectColour(TEXT("equal rule added"), TEXT("a"), Red);
		ExpectColour(TEXT("equal rule added"), TEXT("deep"), Red);
		ExpectBorderTop(TEXT("equal rule added"), Red);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Noop"), true)));
		ExpectColour(TEXT("rule without custom properties added"), TEXT("a"), Red);
		ExpectColour(TEXT("rule without custom properties added"), TEXT("deep"), Red);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Same"), false)));
		ExpectColour(TEXT("equal rule removed"), TEXT("deep"), Red);
		ExpectBorderTop(TEXT("equal rule removed"), Red);
	}

	TEST_METHOD(CustomPropertyChangedToOtherValue_UpdatesVarUsersInTheWholeSubtree)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Vars.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetFlag(TEXT("Diff"), true)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesVariables) > 0, TEXT("variable users not restyled")));
		ExpectColour(TEXT("value changed"), TEXT("a"), Blue);
		ExpectColour(TEXT("value changed"), TEXT("deep"), Blue);
		ExpectBorderTop(TEXT("value changed"), Blue);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Diff"), false)));
		ExpectColour(TEXT("value restored"), TEXT("a"), Red);
		ExpectColour(TEXT("value restored"), TEXT("deep"), Red);
		ExpectBorderTop(TEXT("value restored"), Red);
	}

	TEST_METHOD(CustomPropertyAddedThenRemoved_TakesEffectBothWays)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Vars.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ExpectColour(TEXT("baseline uses the fallback"), TEXT("b"), Green);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Has"), true)));
		ExpectColour(TEXT("property added"), TEXT("b"), Blue);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Has"), false)));
		ExpectColour(TEXT("property removed"), TEXT("b"), Green);
	}

	TEST_METHOD(VarUserWithEqualExpressionAndOtherPropertyChanged_StillFollowsTheAncestorVariable)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Vars.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Alt"), true)));
		const FArcRmlTestLayoutElement* Alt = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("a"));
		ASSERT_THAT(IsNotNull(Alt));
		ASSERT_THAT(IsNear(30.0f, Alt->Rect.Size.Y, 0.5f, TEXT("the class did not change the height")));
		ExpectColour(TEXT("class with the var user added"), TEXT("a"), Red);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Diff"), true)));
		ExpectColour(TEXT("variable changed under the new definition"), TEXT("a"), Blue);

		ASSERT_THAT(IsTrue(SetFlag(TEXT("Alt"), false)));
		ExpectColour(TEXT("class removed while the variable differs"), TEXT("a"), Blue);
	}

	TEST_METHOD(ParentOpacityChangedByClass_GivesChildrenTheFullComputeOpacity)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Opacity.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), InheritHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), VarHalf), DescribeColours(View.GetDraws())));

		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(SetFullParentOpacity(true)));
		ASSERT_THAT(IsTrue(View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputesOpacityOnly) > 0, TEXT("no opacity-only compute")));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::PropertiesComputed), TEXT("the property loop ran for an opacity-only change")));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainFull), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), InheritFull), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), VarFull), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), PlainHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), InheritHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(0, CountColour(View.GetDraws(), VarHalf), DescribeColours(View.GetDraws())));

		ASSERT_THAT(IsTrue(SetFullParentOpacity(false)));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), InheritHalf), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), VarHalf), DescribeColours(View.GetDraws())));
	}

	TEST_METHOD(ShownAndSettledDocument_ComputesNoStyleOnFurtherUpdates)
	{
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Vars.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.ResetPerfCounters()));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(IsTrue(Read()));
		ASSERT_THAT(AreEqual(int64(0), View.GetPerfCounter(Rml::ArcPerf::Counter::StyleComputes), TEXT("styles computed in updates after the document settled")));
	}

	TEST_METHOD(ParentOpacityChangedInline_GivesChildrenTheFullComputeOpacity)
	{
		using namespace ArcRmlRestyleValueTestsPrivate;
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/Opacity.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), InheritHalf), DescribeColours(View.GetDraws())));

		const uint64 Before = View.GetProbe().FrameSerial;
		View.InspectEdit(TEXT("#p1"), TEXT("opacity"), TEXT("1"));
		View.InspectEdit(TEXT("#p2"), TEXT("opacity"), TEXT("1"));
		View.InspectEdit(TEXT("#p3"), TEXT("opacity"), TEXT("1"));
		for (int32 Attempt = 0; Attempt < 50 && View.GetProbe().FrameSerial <= Before; ++Attempt)
		{
			ASSERT_THAT(IsTrue(View.FlushUntilSettled()));
		}
		ASSERT_THAT(IsTrue(View.GetProbe().FrameSerial > Before));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), PlainFull), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), InheritFull), DescribeColours(View.GetDraws())));
		ASSERT_THAT(AreEqual(1, CountColour(View.GetDraws(), VarFull), DescribeColours(View.GetDraws())));
	}

	TEST_METHOD(DocumentShownUnderUnstyledRoot_StylesInheritedFontAndLineHeightOnFirstPass)
	{
		ASSERT_THAT(IsTrue(Open(TEXT("Restyle/RootFirst.rml")), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const FArcRmlTestLayoutElement* Inherits = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("inh"));
		const FArcRmlTestLayoutElement* Own = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("own"));
		const FArcRmlTestLayoutElement* Sans = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("sans"));
		ASSERT_THAT(IsNotNull(Inherits));
		ASSERT_THAT(IsNotNull(Own));
		ASSERT_THAT(IsNotNull(Sans));
		ASSERT_THAT(IsNear(40.0f, Inherits->Rect.Size.Y, 0.5f, TEXT("line-height of the body reached the element")));
		ASSERT_THAT(IsNear(40.0f, Own->Rect.Size.Y, 0.5f));

		const TArray<FArcRmlTestTextLine> InheritsLines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("inh"));
		const TArray<FArcRmlTestTextLine> OwnLines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("own"));
		const TArray<FArcRmlTestTextLine> SansLines = ArcRmlUITestHelpers::GetLines(Elements, TEXT("sans"));
		ASSERT_THAT(IsTrue(InheritsLines.Num() > 0 && OwnLines.Num() > 0 && SansLines.Num() > 0));
		ASSERT_THAT(IsNear(OwnLines[0].Width, InheritsLines[0].Width, 0.5f, TEXT("the font-family of the body reached the element")));
		ASSERT_THAT(IsTrue(FMath::Abs(SansLines[0].Width - OwnLines[0].Width) > 1.0f, TEXT("the two families measure the same, the probe cannot tell them apart")));
	}
};
