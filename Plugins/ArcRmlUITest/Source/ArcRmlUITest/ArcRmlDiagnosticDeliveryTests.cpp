// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "Compile/ArcRmlDiagnostics.h"
#include "CQTest.h"

#if WITH_EDITOR
namespace ArcRmlDiagnosticDeliveryTestsPrivate
{
	FArcRmlDiagnostic MakeComponentError(const FString& File, int32 Line, const FString& Message)
	{
		FArcRmlDiagnostic Diagnostic;
		Diagnostic.Severity = EArcRmlDiagnosticSeverity::Error;
		Diagnostic.Kind = EArcRmlDiagnosticKind::Component;
		Diagnostic.File = File;
		Diagnostic.Line = Line;
		Diagnostic.Message = Message;
		return Diagnostic;
	}

	FArcRmlCompileDiagnostics MakeCompile(TArray<FString>&& Sources, TArray<FArcRmlDiagnostic>&& Diagnostics)
	{
		FArcRmlCompileDiagnostics Compile;
		Compile.Label = TEXT("test");
		Compile.Sources = MoveTemp(Sources);
		Compile.Diagnostics = MoveTemp(Diagnostics);
		return Compile;
	}

	const TCHAR* const Document = TEXT("/Test/Doc.rml");
	const TCHAR* const Component = TEXT("/Test/Comp.rml");
}

TEST_CLASS(ArcRmlDiagnosticDeliveryTests, "ArcRmlUI.DiagnosticDelivery")
{
	FArcRmlTestView View;
	FDelegateHandle CompileHandle;
	TArray<FArcRmlCompileDiagnostics> Received;

	AFTER_EACH()
	{
		FArcRmlUIModule::Get().OnCompileDiagnostics().Remove(CompileHandle);
		View.Close();
	}

	TEST_METHOD(ComponentErrorThatCompilesClean_IsResolvedAndReportedAgainLater)
	{
		using namespace ArcRmlDiagnosticDeliveryTestsPrivate;
		FArcRmlDiagnosticDelivery Delivery;
		TArray<FArcRmlDiagnostic> Broken;
		Broken.Add(MakeComponentError(Component, 3, TEXT("unknown prop")));
		Broken.Add(MakeComponentError(Component, 3, TEXT("unknown prop")));
		FArcRmlDeliveryUpdate First = Delivery.Update(MakeCompile({Document, Component}, CopyTemp(Broken)));
		ASSERT_THAT(IsTrue(First.Changed.Contains(Component)));
		ASSERT_THAT(AreEqual(1, First.Changed.Num()));
		ASSERT_THAT(IsTrue(First.Resolved.IsEmpty()));

		FArcRmlDeliveryUpdate Same = Delivery.Update(MakeCompile({Document, Component}, CopyTemp(Broken)));
		ASSERT_THAT(IsTrue(Same.Changed.IsEmpty(), TEXT("an unchanged diagnostic was delivered again")));

		FArcRmlDeliveryUpdate Clean = Delivery.Update(MakeCompile({Document, Component}, TArray<FArcRmlDiagnostic>()));
		ASSERT_THAT(IsTrue(Clean.Changed.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Clean.Resolved.Num()));
		ASSERT_THAT(AreEqual(FString(Component), Clean.Resolved[0]));

		FArcRmlDeliveryUpdate Again = Delivery.Update(MakeCompile({Document, Component}, CopyTemp(Broken)));
		ASSERT_THAT(IsTrue(Again.Changed.Contains(Component), TEXT("the same error after a clean compile was not reported again")));
	}

	TEST_METHOD(ChangedComponent_ReportsItAsCheckedSource)
	{
		const FString DocumentPath = ArcRmlUITestHelpers::MakePath(TEXT("Comp/CallTest.rml"));
		const FString ComponentPath = ArcRmlUITestHelpers::MakePath(TEXT("Comp/CallButton.rml"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, DocumentPath) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		CompileHandle = FArcRmlUIModule::Get().OnCompileDiagnostics().AddLambda([this](const FArcRmlCompileDiagnostics& Diagnostics)
		{
			Received.Add(Diagnostics);
		});
		TArray<FString> Changed;
		Changed.Add(ComponentPath);
		FArcRmlTestView::CompileChanged(MoveTemp(Changed));
		TestCommandBuilder
			.Until(TEXT("the compile reports"), [this, ComponentPath]() -> bool
			{
				View.Flush();
				return Received.ContainsByPredicate([&ComponentPath](const FArcRmlCompileDiagnostics& Diagnostics) -> bool
				{
					return Diagnostics.Sources.Contains(ComponentPath);
				});
			}, FTimespan::FromSeconds(20.0))
			.Then(TEXT("check"), [this, DocumentPath, ComponentPath]()
			{
				TArray<FString> Sources;
				for (const FArcRmlCompileDiagnostics& Diagnostics : Received)
				{
					Sources.Append(Diagnostics.Sources);
				}
				ASSERT_THAT(IsTrue(Sources.Contains(ComponentPath), FString::Join(Sources, TEXT(", "))));
				ASSERT_THAT(IsTrue(Sources.Contains(DocumentPath), FString::Join(Sources, TEXT(", "))));
			});
	}
};
#endif

TEST_CLASS(ArcRmlDiagnosticEntriesTests, "ArcRmlUI.DiagnosticEntries")
{
	TEST_METHOD(FailedOpenAndViewOfOnePath_AreSeparateEntries)
	{
		FArcRmlDiagnosticEntries Entries;
		FArcRmlDiagnostic Error;
		Error.Severity = EArcRmlDiagnosticSeverity::Error;
		Error.File = TEXT("/Test/A.rml");
		Error.Message = TEXT("broken");
		TArray<FArcRmlDiagnostic> Failed;
		Failed.Add(Error);
		Entries.SetFailedOpen(TEXT("/Test/A.rml"), MoveTemp(Failed));

		FArcRmlViewDiagnostics ViewDiagnostics;
		ViewDiagnostics.bHasDocument = true;
		ViewDiagnostics.Diagnostics.Add(Error);
		Entries.SetView(TEXT("/Test/A.rml"), ViewDiagnostics);
		Entries.SetView(TEXT("/Test/B.rml"), ViewDiagnostics);
		ASSERT_THAT(AreEqual(3, Entries.GetEntries().Num(), TEXT("a view entry replaced the failed open of the same path")));

		Entries.RemoveFailedOpens();
		ASSERT_THAT(AreEqual(2, Entries.GetEntries().Num(), TEXT("travel cleared a view entry")));
		ASSERT_THAT(IsFalse(Entries.GetEntries()[0].bFailedOpen));

		FArcRmlViewDiagnostics Clean;
		Clean.bHasDocument = true;
		Entries.SetView(TEXT("/Test/A.rml"), Clean);
		ASSERT_THAT(AreEqual(1, Entries.GetEntries().Num(), TEXT("a clean view kept its entry")));
		ASSERT_THAT(AreEqual(FString(TEXT("/Test/B.rml")), Entries.GetEntries()[0].Path));

		Entries.RemoveView(TEXT("/Test/B.rml"));
		ASSERT_THAT(IsTrue(Entries.GetEntries().IsEmpty()));
	}
};
