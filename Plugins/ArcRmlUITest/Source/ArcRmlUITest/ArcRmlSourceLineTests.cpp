// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "Compile/ArcRmlDiagnostics.h"
#include "CQTest.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"

namespace ArcRmlSourceLineTestsPrivate
{
	class FLogLineCounter final : public FOutputDevice
	{
	public:
		explicit FLogLineCounter(const TCHAR* InNeedle)
			: Needle(InNeedle)
		{
			GLog->AddOutputDevice(this);
		}

		virtual ~FLogLineCounter() override
		{
			GLog->RemoveOutputDevice(this);
		}

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (FCString::Strstr(Message, *Needle))
			{
				++Count;
			}
		}

		virtual bool CanBeUsedOnAnyThread() const override
		{
			return true;
		}

		virtual bool CanBeUsedOnMultipleThreads() const override
		{
			return true;
		}

		int32 GetCount() const
		{
			return Count.load();
		}

	private:
		FString Needle;
		std::atomic<int32> Count{0};
	};

	const FArcRmlInspectorNode* FindNode(const FArcRmlInspectorSnapshot& Snapshot, const TCHAR* Tag, const TCHAR* Id)
	{
		return Snapshot.Nodes.FindByPredicate([Tag, Id](const FArcRmlInspectorNode& Node) -> bool
		{
			return Node.Tag == Tag && Node.Id == Id;
		});
	}

	const FArcRmlInspectorSnapshot* FindLastSnapshot(const FArcRmlTestView& View)
	{
		const TArray<FArcRmlInspectorAnswer>& Answers = View.GetInspectorAnswers();
		for (int32 Index = Answers.Num() - 1; Index >= 0; --Index)
		{
			if (Answers[Index].Snapshot.IsValid())
			{
				return Answers[Index].Snapshot.Get();
			}
		}
		return nullptr;
	}
}

TEST_CLASS(ArcRmlSourceLineTests, "ArcRmlUI.SourceLines")
{
	FArcRmlTestView View;

	AFTER_EACH()
	{
		View.Close();
	}

	bool CompileAndOpen(const FString& Path)
	{
		return FArcRmlTestView::DropCompiledDocument(Path) && ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush();
	}

	TEST_METHOD(MultiLineOpenTag_ReportsAtTheTagStart)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/SourceLinesTest.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr"), Path, 9), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("colr")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(ElementAndBodyLines_AreTheirOpenTagStarts)
	{
		using namespace ArcRmlSourceLineTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/SourceLinesTest.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		View.ClearInspectorAnswers();
		View.SetInspectorActive(true);
		View.InspectorSnapshot();
		ASSERT_THAT(IsTrue(View.Flush()));
		const FArcRmlInspectorSnapshot* Snapshot = FindLastSnapshot(View);
		ASSERT_THAT(IsNotNull(Snapshot, TEXT("the view did not answer the inspector")));
		const FArcRmlInspectorNode* Multi = FindNode(*Snapshot, TEXT("div"), TEXT("sl-multi"));
		const FArcRmlInspectorNode* Body = FindNode(*Snapshot, TEXT("body"), TEXT(""));
		ASSERT_THAT(IsNotNull(Multi));
		ASSERT_THAT(IsNotNull(Body));
		ASSERT_THAT(AreEqual(9, Multi->Line, TEXT("a multi-line open tag is not at the line of its '<'")));
		ASSERT_THAT(AreEqual(Path, Multi->File));
		ASSERT_THAT(AreEqual(8, Body->Line, TEXT("the document body has no line")));
	}

	TEST_METHOD(LongTypeTypo_SuggestsOnlyForInsertionDeletionOrSwap)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/SourceLinesTest.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("unknown data attribute 'data-tex'; did you mean 'data-text'?"), Path, 12),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("data-test")).Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(MismatchedCloseTag_IsReportedAtItsLine)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/CloseMismatchTest.rml"));
		TestRunner->AddExpectedErrorPlain(TEXT("Closing tag 'span' mismatched"), EAutomationExpectedErrorFlags::Contains, 0);
		ASSERT_THAT(IsTrue(CompileAndOpen(Path), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("the close tag '</span>' does not match the open tag '<p>'"), Path, 9),
			ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(UnparsedExpression_CarriesTheReasonAndIsNotParsedAgainAtLoad)
	{
		using namespace ArcRmlSourceLineTestsPrivate;
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/FailedExpressionTest.rml"));
		FLogLineCounter RuntimeParseErrors(TEXT("Error in data expression"));
		FLogLineCounter RuntimeViewErrors(TEXT("Could not add data-"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		const TArray<FArcRmlDiagnostic> Found = ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("Data expression ' 1 + ' does not parse: Expected "));
		ASSERT_THAT(AreEqual(1, Found.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(10, Found[0].Line));
		ASSERT_THAT(IsTrue(Found[0].Message.Contains(TEXT("(at character")), Found[0].Message));

		ASSERT_THAT(IsTrue(View.Reload() && View.Flush() && View.GetProbe().bHasDocument, TEXT("the compiled document did not load again")));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, RuntimeParseErrors.GetCount(), TEXT("the load parsed an expression its compile already failed")));
		ASSERT_THAT(AreEqual(0, RuntimeViewErrors.GetCount(), TEXT("the load reported a data view its compile already failed")));
	}

	TEST_METHOD(SlotChildrenAndPropLines_MapToTheirOwnFiles)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/SlotLineTest.rml"));
		const FString Component = ArcRmlUITestHelpers::MakePath(TEXT("Comp/SlotLineBox.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: green"), Component, 4),
			FString::Printf(TEXT("a prop value with a line break shifted the component lines; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: purple"), Component, 5),
			FString::Printf(TEXT("the slot line of the component resolved to another file; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: red"), Path, 11), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(MultiLineTags_MapToTheirOpenTagStartAcrossComponentsSlotsAndFoldedText)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/MultiLineTest.rml"));
		const FString Component = ArcRmlUITestHelpers::MakePath(TEXT("Comp/MultiRootBox.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: orange"), Component, 2),
			FString::Printf(TEXT("a component root tag over two lines is not at its '<' line; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: olive"), Component, 4), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: teal"), Path, 10),
			FString::Printf(TEXT("a first slot child over two lines is not at its '<' line; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: navy"), Path, 14),
			FString::Printf(TEXT("a tag over two lines after a folded text run is not at its '<' line; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: maroon"), Path, 16), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
	}

	TEST_METHOD(SlotChildrenInsideAnSvg_KeepTheirLines)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/SvgSlotTest.rml"));
		const FString Component = ArcRmlUITestHelpers::MakePath(TEXT("Comp/SvgBox.rml"));
		ASSERT_THAT(IsTrue(CompileAndOpen(Path) && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: olive"), Component, 3),
			FString::Printf(TEXT("the component line after an svg slot is shifted; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, TEXT("colr: lime"), Path, 11),
			FString::Printf(TEXT("the document line after an import with an svg slot is shifted; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
		const FArcRmlTestLayoutElement* Svg = Elements.FindByPredicate([](const FArcRmlTestLayoutElement& Element) -> bool
		{
			return Element.Tag == TEXT("svg");
		});
		ASSERT_THAT(IsNotNull(Svg, TEXT("the svg is not in the layout")));
		ASSERT_THAT(IsTrue(FMath::IsNearlyEqual(Svg->Rect.Size.X, 40.f, 0.5f) && FMath::IsNearlyEqual(Svg->Rect.Size.Y, 40.f, 0.5f),
			FString::Printf(TEXT("the svg is %.1f x %.1f, not 40 x 40"), Svg->Rect.Size.X, Svg->Rect.Size.Y)));
	}

	TEST_METHOD(SlotAndImportInsideATextarea_AreCompileErrorsAtTheirLines)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Comp/TextareaSlotTest.rml"));
		const FString Component = ArcRmlUITestHelpers::MakePath(TEXT("Comp/TextareaBox.rml"));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Path);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Component);
		ASSERT_THAT(IsFalse(CompileAndOpen(Path), TEXT("a document with a slot inside a textarea compiled and opened")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("<slot/> inside raw text"), Component, 3), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("<dc-import> inside raw text"), Path, 11), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}

	TEST_METHOD(AssetReferenceWarning_IsPersistedWithTheCompiledFile)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Diag/AssetWarningTest.rml"));
		const FString Asset = TEXT("/ArcRmlUITest/UI/Unscanned/T_Pending.T_Pending");
		const FString Warning = FString::Printf(TEXT("asset '%s' is not checked, the asset registry has not gathered it yet"), *Asset);
		FArcRmlTestView::ForceUnregisteredAsset(Asset, true);
		const bool bCompiled = CompileAndOpen(Path) && View.GetProbe().bHasDocument;
		FArcRmlTestView::ForceUnregisteredAsset(Asset, false);
		ASSERT_THAT(IsTrue(bCompiled, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, *Warning, Path, 10), ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		ASSERT_THAT(IsTrue(View.Reload() && View.Flush() && View.GetProbe().bHasDocument, TEXT("the compiled document did not load again")));
		ASSERT_THAT(AreEqual(0, ArcRmlUITestHelpers::FindDiagnostics(View, TEXT("does not exist")).Num(),
			FString::Printf(TEXT("the reload compiled the document again instead of loading its compiled file; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, *Warning, Path, 10),
			FString::Printf(TEXT("the compiled file did not keep the warning; %s"), *ArcRmlUITestHelpers::DescribeDiagnostics(View))));
	}
};
