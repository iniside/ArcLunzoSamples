// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "InputCoreTypes.h"

namespace ArcRmlDiagnosticsTestsPrivate
{
	const TCHAR* const SpacerShiftNote = TEXT("the spacer component above the element shifts expanded lines, so only a mapped diagnostic matches the source line");
}

TEST_CLASS(ArcRmlDiagnosticsTests, "ArcRmlUI.Diagnostics")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool OpenAndFlush(const FString& Path)
	{
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush();
	}

	TEST_METHOD(LiteralInlineStyle_ReportsOnceAtSourceLine)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/InlineStyleTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path), TEXT("the compiled document could not be dropped")));
		TestRunner->AddExpectedMessagePlain(Path + TEXT("(11): "), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr"), Path, 11),
			FString::Printf(TEXT("%s; %s"), ArcRmlDiagnosticsTestsPrivate::SpacerShiftNote, *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(BoundStyleInDataForRows_ReportsOnceNotPerRow)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/InlineStyleRowsTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path), TEXT("the compiled document could not be dropped")));
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr"), Path, 13), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("tint")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("tint"), Path, 12), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(AnimatedBoundStyle_StaysAtOneDiagnostic)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/AnimatedStyleTest.rml"));
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("widht")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		for (int32 Step = 0; Step < 8; ++Step)
		{
			View.SetModelValue(TEXT("rows"), ArcRmlUITestHelpers::MakeRowFieldPath(TEXT("Rows"), Step % 4, TEXT("Value")), ArcRmlUITestHelpers::MakeInt(100 + Step * 7));
			ASSERT_THAT(IsTrue(View.Flush(), FString::Printf(TEXT("flush after model step %d failed"), Step)));
		}

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("widht")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("widht"), Path, 11), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(TwoBadDeclarationsOnOneElement_ReportTwice)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/TwoDropsTest.rml"));
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr"), Path, 11), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("widht"), Path, 11), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("widht")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(BoundStyleFirstAppliedDuringClickElement_MapsToSourceLine)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/BoundStyleTest.rml"));
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("bound"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("bad")), ArcRmlUITestHelpers::MakeBool(true));
		View.ClickElement(TEXT("#target"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the click failed")));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("colr"), Path, 11, EArcRmlDiagnosticScope::Click),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(BoundStyleFirstAppliedDuringConfirmWithInvalidNavFocus_MapsToSourceLine)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/BoundStyleTest.rml"));
		ASSERT_THAT(IsTrue(OpenAndFlush(Path), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsFalse(View.GetProbe().bFocusNavigable, TEXT("the document must start without a navigable focus")));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("bound"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("bad")), ArcRmlUITestHelpers::MakeBool(true));
		View.RawNavFocusStart(true);
		View.RawKeyDown(EKeys::Enter);
		View.RawKeyUp(EKeys::Enter);
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the confirm failed")));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("colr"), Path, 11, EArcRmlDiagnosticScope::Confirm),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(FrameBoundStyleFirstAppliedDuringTabEmbed_MapsToFrameLine)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedFrame.rml"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedTab.rml"));
		ASSERT_THAT(IsTrue(OpenAndFlush(FramePath), TEXT("the frame did not open and answer a probe")));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("widht")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("frame"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("bad")), ArcRmlUITestHelpers::MakeBool(true));
		ASSERT_THAT(IsTrue(View.Embed(TabPath), TEXT("the frame did not accept the tab")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("the model change and the embed did not answer one probe")));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("widht")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("widht"), FramePath, 11, EArcRmlDiagnosticScope::Embed),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("tabcolr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("tabcolr"), TabPath, 10, EArcRmlDiagnosticScope::Embed),
			FString::Printf(TEXT("%s; %s"), ArcRmlDiagnosticsTestsPrivate::SpacerShiftNote, *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}

	TEST_METHOD(TabBoundStyleFirstAppliedDuringClickElement_MapsToTabLine)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedFrame.rml"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedTab.rml"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FramePath), TEXT("the frame did not open")));
		ASSERT_THAT(IsTrue(View.Embed(TabPath), TEXT("the frame did not accept the tab")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("the frame load and the embed did not answer one probe")));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("clickcolr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		View.SetModelValue(TEXT("tab"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("clickBad")), ArcRmlUITestHelpers::MakeBool(true));
		View.ClickElement(TEXT("#tab-click"));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("flush after the click failed")));

		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("clickcolr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("clickcolr"), TabPath, 11, EArcRmlDiagnosticScope::Click),
			FString::Printf(TEXT("%s; %s"), ArcRmlDiagnosticsTestsPrivate::SpacerShiftNote, *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("tabcolr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(TabStyleSheetWarning_ClearedWhenAnotherTabIsEmbedded)
	{
		const FString FramePath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedFrame.rml"));
		const FString StyleTabPath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedStyleTab.rml"));
		const FString StyleSheetPath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedStyleTab.rcss"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(TEXT("Diag/EmbedTab.rml"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, FramePath), TEXT("the frame did not open")));
		ASSERT_THAT(IsTrue(View.Embed(StyleTabPath), TEXT("the frame did not accept the style tab")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("the frame load and the embed did not answer one probe")));
		ASSERT_THAT(AreEqual(StyleTabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAtInScope(View, TEXT("unknown-probe"), StyleSheetPath, 4, EArcRmlDiagnosticScope::Embed),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.Embed(TabPath), TEXT("the frame did not accept the second tab")));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("the second embed did not answer a probe")));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("unknown-probe")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("tabcolr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}
};
