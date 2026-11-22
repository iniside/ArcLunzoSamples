// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITags.h"
#include "ArcRmlUITestHelpers.h"
#include "Compile/ArcRmlCompiledFile.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "MVVM/ArcRmlViewModelDeclarations.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace ArcRmlCompileTestsPrivate
{
	const TCHAR* const StaleComponent = TEXT("Comp/StaleComponent.rml");

	const TCHAR* const HeadOnlyDocument = TEXT(
		"<rml>\n"
		"<head>\n"
		"\t<title>Compiled Head Test</title>\n"
		"\t<meta name=\"layer\" content=\"UI.Layer.Menu\"/>\n"
		"\t<meta name=\"input-mode\" content=\"menu\"/>\n"
		"\t<meta name=\"hide-lower\" content=\"false\"/>\n"
		"\t<viewmodel name=\"Probe\" class=\"/Script/ArcRmlUITest.ArcRmlHeadProbe\" context=\"ProbeContext\"/>\n"
		"</head>\n"
		"<body>\n"
		"\t<div id=\"compiled-head\">compiled head</div>\n"
		"</body>\n"
		"</rml>\n");

	const TCHAR* const GoodLink = TEXT("/ArcRmlUITest/UI/Rml/Test/Common.rcss");
	const TCHAR* const WrongCaseLink = TEXT("/ArcRmlUITest/UI/Rml/Test/common.rcss");

	FString MakeReloadDocument(const FString& HeadLines, const FString& Link, const FString& BodyLines)
	{
		return FString::Printf(TEXT("<rml>\n<head>\n\t<title>Reload Test</title>\n%s\t<link type=\"text/rcss\" href=\"%s\"/>\n</head>\n<body>\n%s</body>\n</rml>\n"),
			*HeadLines, *Link, *BodyLines);
	}

	FString MakeTextBody(const FString& Id, const FString& Text)
	{
		return FString::Printf(TEXT("\t<div id=\"%s\">%s</div>\n"), *Id, *Text);
	}

	FString GetStaleBinary()
	{
		return FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), FString(StaleComponent) + TEXT("c"));
	}
}

TEST_CLASS(ArcRmlCompileTests, "ArcRmlUI.Compile")
{
	FArcRmlTestView View;
	FString RuntimeDir;

	AFTER_EACH()
	{
		View.Close();
		IFileManager::Get().Delete(*ArcRmlCompileTestsPrivate::GetStaleBinary(), false, false, true);
		if (!RuntimeDir.IsEmpty())
		{
			IFileManager::Get().DeleteDirectory(*RuntimeDir, false, true);
			IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RuntimeDir), false, false);
			RuntimeDir.Reset();
		}
	}

	TEST_METHOD(LinkWithWrongCase_IsCompileError)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Links/LinkCaseTest.rml"));
		const FString WrongCase = ArcRmlUITestHelpers::MakePath(TEXT("common.rcss"));
		const FString OnDisk = ArcRmlUITestHelpers::MakePath(TEXT("Common.rcss"));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Path);
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path), TEXT("the compiled document could not be dropped")));
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::OpenView(View, Path), TEXT("a document with a wrong-case link compiled and opened")));

		const FString Expected = FString::Printf(TEXT("<link href> '%s' differs in case from the file '%s'"), *WrongCase, *OnDisk);
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, Expected, Path, 5), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("differs in case")), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}

	TEST_METHOD(RelativeLinkWithMatchingCase_Compiles)
	{
		const FString Path = ArcRmlUITestHelpers::MakePath(TEXT("Links/LinkCaseOkTest.rml"));
		ASSERT_THAT(IsTrue(FArcRmlTestView::DropCompiledDocument(Path), TEXT("the compiled document could not be dropped")));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush(), TEXT("the view did not open and answer a probe")));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument));
	}

	FString MakeRunFolder()
	{
		const FString RunFolder = FString::Printf(TEXT("HeadRuntime/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RuntimeDir = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), RunFolder);
		return RunFolder;
	}

	bool WriteRuntimeFile(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *FPaths::Combine(RuntimeDir, Name));
	}

	FString ReadText(const FString& DomId)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		return View.Flush() && View.ParseLayout(Elements) ? ArcRmlUITestHelpers::GetText(Elements, DomId) : FString();
	}

	TEST_METHOD(CompiledDocumentWithoutSource_ReadsItsHead)
	{
		using namespace ArcRmlCompileTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		const FString SourceFile = FPaths::Combine(RuntimeDir, TEXT("CompiledHeadTest.rml"));
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/CompiledHeadTest.rml"));
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(FString(HeadOnlyDocument), *SourceFile), TEXT("the document source could not be written")));
		ASSERT_THAT(IsTrue(ArcRmlViewModelDeclarations::ReadDocumentHead(Path).IsSet(), TEXT("the document did not compile")));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*(SourceFile + TEXT("c"))), TEXT("the compile wrote no compiled file")));

		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*SourceFile), TEXT("the document source could not be removed")));
		TArray<uint8> CompiledBytes;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToArray(CompiledBytes, *(SourceFile + TEXT("c"))), TEXT("the compiled file could not be read")));
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::PrepareForCook(EArcRmlCompiledKind::Document, CompiledBytes), TEXT("the compiled file could not be stripped like a cooked one")));
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(CompiledBytes, *(SourceFile + TEXT("c"))), TEXT("the stripped compiled file could not be written")));
		const TOptional<FArcRmlDocumentHead> Head = ArcRmlViewModelDeclarations::ReadCompiledHead(Path);
		ASSERT_THAT(IsTrue(Head.IsSet(), TEXT("the head was not read from the compiled file alone")));
		ASSERT_THAT(IsTrue(Head->Meta.Layer == ArcRmlUITags::UI_Layer_Menu, Head->Meta.Layer.ToString()));
		ASSERT_THAT(IsTrue(Head->Meta.InputMode == EArcRmlInputMode::Menu, TEXT("the input mode is not 'menu'")));
		ASSERT_THAT(IsFalse(Head->Meta.bHideLower, TEXT("hide-lower is not 'false'")));
		ASSERT_THAT(AreEqual(1, Head->Declarations.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("Probe")), Head->Declarations[0].Name));
		ASSERT_THAT(AreEqual(FString(TEXT("/Script/ArcRmlUITest.ArcRmlHeadProbe")), Head->Declarations[0].ClassPath));
		ASSERT_THAT(AreEqual(FName(TEXT("ProbeContext")), Head->Declarations[0].Context));
	}

	TEST_METHOD(CompiledHeadLargerThanTheFirstRead_IsReadWhole)
	{
		using namespace ArcRmlCompileTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		const FString SourceFile = FPaths::Combine(RuntimeDir, TEXT("LargeHeadTest.rml"));
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LargeHeadTest.rml"));
		const FString Padding = FString::ChrN(6000, TEXT('x'));
		const FString HeadLines = FString::Printf(TEXT("\t<!-- %s -->\n\t<meta name=\"input-mode\" content=\"menu\"/>\n"), *Padding);
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("LargeHeadTest.rml"), MakeReloadDocument(HeadLines, GoodLink, MakeTextBody(TEXT("large-head"), TEXT("large head"))))));
		ASSERT_THAT(IsTrue(ArcRmlViewModelDeclarations::ReadDocumentHead(Path).IsSet(), TEXT("the document did not compile")));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileSize(*(SourceFile + TEXT("c"))) > Padding.Len(), TEXT("the compiled file does not hold the large head")));

		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*SourceFile), TEXT("the document source could not be removed")));
		const TOptional<FArcRmlDocumentHead> Head = ArcRmlViewModelDeclarations::ReadCompiledHead(Path);
		ASSERT_THAT(IsTrue(Head.IsSet(), TEXT("a head larger than the first header read was not read")));
		ASSERT_THAT(IsTrue(Head->Meta.InputMode == EArcRmlInputMode::Menu, TEXT("the meta after the large comment was not read")));
	}

	TEST_METHOD(BrokenDocumentOnReload_KeepsTheViewUntilItCompiles)
	{
		using namespace ArcRmlCompileTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/ReloadDocument.rml"));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Path);
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadDocument.rml"), MakeReloadDocument(FString(), GoodLink, MakeTextBody(TEXT("reload-text"), TEXT("first"))))));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("first")), ReadText(TEXT("reload-text"))));

		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadDocument.rml"), MakeReloadDocument(FString(), WrongCaseLink, MakeTextBody(TEXT("reload-text"), TEXT("broken"))))));
		ASSERT_THAT(IsFalse(View.Reload(), TEXT("a document that does not compile was reloaded")));
		ASSERT_THAT(IsTrue(View.IsOpen(), TEXT("the reload of a broken document closed its view")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("differs in case")), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("first")), ReadText(TEXT("reload-text")), TEXT("the view lost its last good document")));

		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadDocument.rml"), MakeReloadDocument(FString(), GoodLink, MakeTextBody(TEXT("reload-text"), TEXT("fixed"))))));
		ASSERT_THAT(IsTrue(View.Reload(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("fixed")), ReadText(TEXT("reload-text")), TEXT("the fixed document was not reloaded")));
	}

	TEST_METHOD(BrokenTabOnReload_KeepsItsFrameUntilItCompiles)
	{
		using namespace ArcRmlCompileTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		const FString FramePath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/ReloadFrame.rml"));
		const FString TabPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/ReloadTab.rml"));
		const FString FrameMeta = FString::Printf(TEXT("\t<meta name=\"frame\" content=\"%s\"/>\n"), *FramePath);
		const FString FrameBody = MakeTextBody(TEXT("frame-text"), TEXT("frame")) + TEXT("\t<div class=\"embed\" arc-embed=\"\"></div>\n");
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, TabPath);
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadFrame.rml"), MakeReloadDocument(FString(), GoodLink, FrameBody))));
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadTab.rml"), MakeReloadDocument(FrameMeta, GoodLink, MakeTextBody(TEXT("tab-text"), TEXT("first"))))));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, TabPath) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath));
		ASSERT_THAT(AreEqual(FString(TEXT("first")), ReadText(TEXT("tab-text"))));

		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadTab.rml"), MakeReloadDocument(FrameMeta, WrongCaseLink, MakeTextBody(TEXT("tab-text"), TEXT("broken"))))));
		ASSERT_THAT(IsFalse(View.Reload(), TEXT("a tab that does not compile was reloaded")));
		ASSERT_THAT(IsTrue(View.IsOpen(), TEXT("the reload of a broken tab closed its frame")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnostics(View, TEXT("differs in case")), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("first")), ReadText(TEXT("tab-text")), TEXT("the frame lost its last good tab")));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath));

		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("ReloadTab.rml"), MakeReloadDocument(FrameMeta, GoodLink, MakeTextBody(TEXT("tab-text"), TEXT("fixed"))))));
		ASSERT_THAT(IsTrue(View.Reload(), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("fixed")), ReadText(TEXT("tab-text")), TEXT("the fixed tab was not reloaded")));
		ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath));
	}

	TEST_METHOD(ChangedComponentSource_DropsItsStaleBinary)
	{
		const FString DocumentPath = ArcRmlUITestHelpers::MakePath(TEXT("Links/LinkCaseOkTest.rml"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, DocumentPath) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));

		const FString DocumentBinary = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Links/LinkCaseOkTest.rmlc"));
		const FString StaleBinary = ArcRmlCompileTestsPrivate::GetStaleBinary();
		ASSERT_THAT(IsTrue(IFileManager::Get().Copy(*StaleBinary, *DocumentBinary) == COPY_OK, TEXT("the stale binary could not be planted")));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*StaleBinary)));

		TArray<FString> Changed;
		Changed.Add(ArcRmlUITestHelpers::MakePath(ArcRmlCompileTestsPrivate::StaleComponent));
		FArcRmlTestView::CompileChanged(MoveTemp(Changed));
		ASSERT_THAT(IsTrue(View.Flush(), TEXT("the compile of the changed component did not finish")));
		ASSERT_THAT(IsFalse(IFileManager::Get().FileExists(*StaleBinary), TEXT("the binary of a source whose root is a component was kept")));
	}
};
