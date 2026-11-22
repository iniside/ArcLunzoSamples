// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Convert/ArcRmlHtmlConverter.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace ArcRmlConverterTestsPrivate
{
	FString NormalizeFull(const FString& Path)
	{
		FString Full = FPaths::ConvertRelativePathToFull(Path);
		FPaths::NormalizeFilename(Full);
		FPaths::CollapseRelativeDirectories(Full);
		while (Full.EndsWith(TEXT("/")))
		{
			Full.LeftChopInline(1);
		}
		return Full;
	}

	int32 FindLine(const TArray<FString>& Lines, const FString& Needle)
	{
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			if (Lines[Index].Contains(Needle))
			{
				return Index + 1;
			}
		}
		return INDEX_NONE;
	}

	int32 CountKind(const FArcRmlValidationResult& Report, EArcRmlDiagnosticKind Kind)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			Count += Diagnostic.Kind == Kind ? 1 : 0;
		}
		return Count;
	}

	int32 CountKindAt(const FArcRmlValidationResult& Report, EArcRmlDiagnosticKind Kind, const FString& File, int32 Line)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			if (Diagnostic.Kind == Kind && Diagnostic.Severity == EArcRmlDiagnosticSeverity::Warning && Diagnostic.File == File && Diagnostic.Line == Line)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountErrorsAt(const FArcRmlValidationResult& Report, EArcRmlDiagnosticKind Kind, int32 Line)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			Count += (Diagnostic.Severity == EArcRmlDiagnosticSeverity::Error && Diagnostic.Kind == Kind && Diagnostic.Line == Line) ? 1 : 0;
		}
		return Count;
	}

	int32 CountErrorsOtherThan(const FArcRmlValidationResult& Report, EArcRmlDiagnosticKind Kind)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			Count += (Diagnostic.Severity == EArcRmlDiagnosticSeverity::Error && Diagnostic.Kind != Kind) ? 1 : 0;
		}
		return Count;
	}

	const FArcRmlDiagnostic* FindAt(const FArcRmlValidationResult& Report, EArcRmlDiagnosticKind Kind, int32 Line)
	{
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			if (Diagnostic.Kind == Kind && Diagnostic.Line == Line)
			{
				return &Diagnostic;
			}
		}
		return nullptr;
	}

	FString Describe(const FArcRmlValidationResult& Report)
	{
		TArray<FString> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : Report.Diagnostics)
		{
			Lines.Add(ArcRmlValidation::FormatDiagnostic(Diagnostic));
		}
		return FString::Printf(TEXT("%d diagnostics [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}

	FString JoinLines(const TArray<FString>& Lines)
	{
		return FString::Join(Lines, TEXT("\n"));
	}

	FString OnePage(const FString& Head, const FString& Body)
	{
		return TEXT("<!DOCTYPE html><html><head><title>T</title>") + Head + TEXT("</head><body>") + Body + TEXT("</body></html>");
	}

	FString ExpectedRml(const FString& Links, const FString& Body)
	{
		return TEXT("<rml>\n<head>\n\t<title>T</title>\n") + Links + TEXT("</head>\n<body>") + Body + TEXT("</body>\n</rml>\n");
	}

	FString LinkLine(const FString& Href)
	{
		return TEXT("\t<link type=\"text/rcss\" href=\"") + Href + TEXT("\"/>\n");
	}

	TArray<FString> MakePage(const FString& HtmlTag, const TArray<FString>& HeadLines, const TArray<FString>& BodyLines)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<!DOCTYPE html>"));
		Lines.Add(HtmlTag);
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>T</title>"));
		Lines.Append(HeadLines);
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body>"));
		Lines.Append(BodyLines);
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</html>"));
		return Lines;
	}
}

TEST_CLASS(ArcRmlConverterTests, "ArcRmlUI.Converter")
{
	FString RunDir;
	FString SrcDir;
	FString OutDir;
	bool bExpectLogs = false;

	BEFORE_EACH()
	{
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / FString::Printf(TEXT("Convert/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		SrcDir = RunDir / TEXT("Src");
		OutDir = RunDir / TEXT("Out");
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::GetTestContentDir().IsEmpty(), TEXT("the test plugin content directory is unknown")));
	}

	AFTER_EACH()
	{
		if (RunDir.IsEmpty())
		{
			return;
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	bool WriteText(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool WritePng(const FString& Name)
	{
		TArray<uint8> Bytes;
		Bytes.Add(0);
		return FFileHelper::SaveArrayToFile(Bytes, *(RunDir / Name));
	}

	FString ReadOut(const FString& Name)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(OutDir / Name));
		return Text;
	}

	bool OutExists(const FString& Name)
	{
		return IFileManager::Get().FileExists(*(OutDir / Name));
	}

	FString SrcFile(const FString& Name)
	{
		return ArcRmlConverterTestsPrivate::NormalizeFull(SrcDir / Name);
	}

	bool Convert(const FString& Html, FArcRmlConvertResult& OutResult, bool bOverwrite = false, const FString& Name = TEXT("Page"), const FString& SourceName = TEXT("page.html"))
	{
		if (bExpectLogs)
		{
			TestRunner->AddExpectedErrorPlain(TEXT("LogArcRmlUI:"), EAutomationExpectedErrorFlags::Contains, 0);
		}
		if (!WriteText(TEXT("Src/") + SourceName, Html))
		{
			return false;
		}
		FArcRmlConvertRequest Request;
		Request.SourceFile = SrcDir / SourceName;
		Request.OutputFolder = OutDir;
		Request.Name = Name;
		Request.bOverwrite = bOverwrite;
		return ArcRmlHtmlConverter::Convert(Request, OutResult);
	}

	TEST_METHOD(A1A6A7_DoctypeVoidElementsUnquotedAndBooleanAttributes_AreWrittenWellFormed)
	{
		using namespace ArcRmlConverterTestsPrivate;
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(FString(), TEXT("<div>a<br>b<hr><input disabled value=a/b></div>")), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		const FString Expected = ExpectedRml(FString(), TEXT("<div>a<br/>b<hr/><input disabled=\"\" value=\"a/b\"/></div>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A8_OptionalEndTags_AreClosedWhereTheStandardImpliesThem)
	{
		using namespace ArcRmlConverterTestsPrivate;
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(FString(), TEXT("<ul><li>one<li>two</ul><p>x<p>y")), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		const FString Expected = ExpectedRml(FString(), TEXT("<ul><li>one</li><li>two</li></ul><p>x</p><p>y</p>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A6_TextAndValuesEscapeOnlyTheFourEntitiesAndNamedReferencesDecode)
	{
		using namespace ArcRmlConverterTestsPrivate;
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(FString(), TEXT("<p title='say \"hi\" <b>'>1 < 2 &amp; &copy; &hellip; 3</p>")), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		const FString Expected = ExpectedRml(FString(), TEXT("<p title=\"say &quot;hi&quot; &lt;b&gt;\">1 &lt; 2 &amp; © … 3</p>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A2A4C6_MetaAndBaseAreRemovedAndOnlyForeignMetaNamesAreReported)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>{TEXT("<meta charset=\"utf-8\">"), TEXT("<meta name=\"viewport\" content=\"width=device-width\">"),
			TEXT("<meta name=\"layer\" content=\"UI.Layer.Game\">"), TEXT("<base href=\"/x/\">")}, TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedMeta, File, FindLine(Lines, TEXT("charset"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedMeta, File, FindLine(Lines, TEXT("viewport"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(2, CountKind(Result.Report, EArcRmlDiagnosticKind::RemovedMeta), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedElement, File, FindLine(Lines, TEXT("<base"))), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsFalse(Rml.Contains(TEXT("<meta")) || Rml.Contains(TEXT("<base")), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A4_HtmlRootAttributesAreKeptOnTheRmlRoot)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html class=\"dark\" lang=\"en\" onload=\"go()\">"), TArray<FString>(), TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(0, CountKind(Result.Report, EArcRmlDiagnosticKind::RemovedAttribute), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedHandler, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("<html"))), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.StartsWith(TEXT("<rml class=\"dark\" lang=\"en\">\n<head>")), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A3A5_LinkThenStyle_KeepCascadeOrderAsTwoRcssLinks)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WriteText(TEXT("Src/base.css"), TEXT(".base { color: red; }"))));
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(TEXT("<link rel=\"stylesheet\" href=\"base.css\"><style>.btn{color:blue}</style>"), TEXT("<p>x</p>")), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		const FString Expected = ExpectedRml(LinkLine(TEXT("base.rcss")) + LinkLine(TEXT("Page.rcss")), TEXT("<p>x</p>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("base.rcss")) == TEXT(".base { color: red; }"), ReadOut(TEXT("base.rcss"))));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rcss")) == TEXT(".btn{color:blue}"), ReadOut(TEXT("Page.rcss"))));
		ASSERT_THAT(AreEqual(3, Result.Files.Num()));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A5_StyleRuns_ConsecutiveBlocksShareASheetAndALinkSplitsThem)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WriteText(TEXT("Src/base.css"), TEXT(".base { color: red; }"))));
		FArcRmlConvertResult Result;
		const FString Head = TEXT("<style>.a{color:red}</style><style>.b{color:red}</style><link rel=\"stylesheet\" href=\"base.css\"><style>.c{color:red}</style>");
		ASSERT_THAT(IsTrue(Convert(OnePage(Head, TEXT("<p>x</p>")), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		const FString Expected = ExpectedRml(LinkLine(TEXT("Page.rcss")) + LinkLine(TEXT("base.rcss")) + LinkLine(TEXT("Page-2.rcss")), TEXT("<p>x</p>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rcss")) == TEXT(".a{color:red}\n.b{color:red}"), ReadOut(TEXT("Page.rcss"))));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page-2.rcss")) == TEXT(".c{color:red}"), ReadOut(TEXT("Page-2.rcss"))));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A5_SheetNameCollision_StyleRunTakesTheNextFreeSuffix)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WriteText(TEXT("Src/index.css"), TEXT(".linked{color:red}"))));
		FArcRmlConvertResult Result;
		const FString Head = TEXT("<link rel=\"stylesheet\" href=\"index.css\"><style>.inline{color:blue}</style>");
		ASSERT_THAT(IsTrue(Convert(OnePage(Head, TEXT("<p>x</p>")), Result, false, TEXT("index"), TEXT("index.html")), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("index.rml"));
		const FString Expected = ExpectedRml(LinkLine(TEXT("index.rcss")) + LinkLine(TEXT("index-2.rcss")), TEXT("<p>x</p>"));
		ASSERT_THAT(IsTrue(Rml == Expected, FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("index.rcss")) == TEXT(".linked{color:red}"), ReadOut(TEXT("index.rcss"))));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("index-2.rcss")) == TEXT(".inline{color:blue}"), ReadOut(TEXT("index-2.rcss"))));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C8Q5_PrintMediaBlockAndPageRule_AreRemovedFromTheWrittenSheetAndReported)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>{TEXT("<style>"), TEXT(".a { color: red; }"), TEXT("@media print { .b { color: blue; } }"),
			TEXT("@page { margin: 1cm; }"), TEXT(".c { color: green; }"), TEXT("</style>")}, TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules, File, FindLine(Lines, TEXT("@media print"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules, File, FindLine(Lines, TEXT("@page"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(2, CountKind(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules), Describe(Result.Report)));
		const FString Sheet = ReadOut(TEXT("Page.rcss"));
		ASSERT_THAT(IsTrue(Sheet.Contains(TEXT(".a { color: red; }")) && Sheet.Contains(TEXT(".c { color: green; }")), Sheet));
		ASSERT_THAT(IsFalse(Sheet.Contains(TEXT("@media")) || Sheet.Contains(TEXT("@page")) || Sheet.Contains(TEXT(".b")) || Sheet.Contains(TEXT("margin")), Sheet));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C8Q5_PrintMediaLinkAndStyle_AreRemovedWithoutWritingASheet)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WriteText(TEXT("Src/print.css"), TEXT(".p { color: red; }"))));
		ASSERT_THAT(IsTrue(WriteText(TEXT("Src/base.css"), TEXT(".base { color: red; }"))));
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>{TEXT("<link rel=\"stylesheet\" media=\"print\" href=\"print.css\">"),
			TEXT("<style media=\"print\">.q{color:red}</style>"), TEXT("<link rel=\"stylesheet\" href=\"base.css\">")}, TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules, File, FindLine(Lines, TEXT("print.css"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules, File, FindLine(Lines, TEXT("<style media"))), Describe(Result.Report)));
		ASSERT_THAT(IsFalse(OutExists(TEXT("print.rcss")), TEXT("a print-only link wrote a sheet")));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(LinkLine(TEXT("base.rcss"))) && !Rml.Contains(TEXT("print")), Rml));
		ASSERT_THAT(AreEqual(2, Result.Files.Num()));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A9_EscapedSelectorBeforeUrlAndPrintBlock_DoesNotSwallowTheRestOfTheSheet)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WritePng(TEXT("Src/img/a.png"))));
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>{TEXT("<style>"), TEXT(".before\\:content-\\[\\'\\'\\] { color: red; }"),
			TEXT(".logo { background-image: url(img/a.png); }"), TEXT("@media print { .y { color: red; } }"), TEXT(".z { color: blue; }"), TEXT("</style>")},
			TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedPrintRules, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("@media print"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(0, CountKind(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied), Describe(Result.Report)));
		ASSERT_THAT(IsTrue(OutExists(TEXT("img/a.png")), TEXT("the url() image was not copied")));
		const FString Sheet = ReadOut(TEXT("Page.rcss"));
		ASSERT_THAT(IsTrue(Sheet.Contains(TEXT(".before\\:content-\\[\\'\\'\\] { color: red; }")) && Sheet.Contains(TEXT("url(img/a.png)")) && Sheet.Contains(TEXT(".z { color: blue; }")), Sheet));
		ASSERT_THAT(IsFalse(Sheet.Contains(TEXT("@media")) || Sheet.Contains(TEXT(".y")), Sheet));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A9_ImportInASheet_IsKeptAndReportedAsNotCopied)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>{TEXT("<style>"), TEXT("@import url(other.css);"), TEXT(".a { color: red; }"), TEXT("</style>")},
			TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FArcRmlDiagnostic* Found = FindAt(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied, FindLine(Lines, TEXT("@import")));
		ASSERT_THAT(IsNotNull(Found, Describe(Result.Report)));
		ASSERT_THAT(IsTrue(Found->File == SrcFile(TEXT("page.html")) && Found->Message.Contains(TEXT("@import")), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKind(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied), Describe(Result.Report)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rcss")).Contains(TEXT("@import url(other.css);")), ReadOut(TEXT("Page.rcss"))));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C1_Script_IsRemovedWithItsContentAndReported)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), TArray<FString>{TEXT("<script>var secret = 1;</script>"), TEXT("<p>kept</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedScript, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("<script"))), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(TEXT("<p>kept</p>")) && !Rml.Contains(TEXT("script")) && !Rml.Contains(TEXT("secret")), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C2_InlineHandlers_AreRemovedAndReportedPerAttribute)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), TArray<FString>{TEXT("<button onclick=\"go()\" onmouseover=\"hover()\" id=\"b\">go</button>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(2, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedHandler, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("<button"))), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(TEXT("<button id=\"b\">go</button>")) && !Rml.Contains(TEXT("onclick")), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C3C4_ElementsRmlCannotRun_AreRemovedWithTheirContentAndReportedEach)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Body = {TEXT("<iframe src=\"x.html\"></iframe>"), TEXT("<video src=\"v.mp4\"><source src=\"v.mp4\"></video>"), TEXT("<audio src=\"a.mp3\"></audio>"),
			TEXT("<canvas id=\"c\"></canvas>"), TEXT("<object data=\"o.swf\"></object>"), TEXT("<embed src=\"e.swf\">"), TEXT("<noscript><p>hidden text</p></noscript>"),
			TEXT("<template><p>template text</p></template>"), TEXT("<p>kept</p>")};
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), Body);
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		for (int32 Index = 0; Index < Body.Num() - 1; ++Index)
		{
			ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedElement, File, FindLine(Lines, Body[Index])), Body[Index] + TEXT(" ") + Describe(Result.Report)));
		}
		ASSERT_THAT(AreEqual(8, CountKind(Result.Report, EArcRmlDiagnosticKind::RemovedElement), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(TEXT("<p>kept</p>")) && !Rml.Contains(TEXT("hidden text")) && !Rml.Contains(TEXT("template text")) && !Rml.Contains(TEXT("iframe"))
			&& !Rml.Contains(TEXT("<source")) && !Rml.Contains(TEXT("<embed")), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C5_ExternalLinks_AreRemovedAndTheFontLinkCarriesTheFontHint)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Head = {TEXT("<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\">"), TEXT("<link rel=\"icon\" href=\"favicon.ico\">"),
			TEXT("<link rel=\"manifest\" href=\"site.webmanifest\">"), TEXT("<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Inter\">")};
		const TArray<FString> Lines = MakePage(TEXT("<html>"), Head, TArray<FString>{TEXT("<p>x</p>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		for (const FString& Link : Head)
		{
			ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedLink, File, FindLine(Lines, Link)), Link + TEXT(" ") + Describe(Result.Report)));
		}
		const FArcRmlDiagnostic* Font = FindAt(Result.Report, EArcRmlDiagnosticKind::RemovedLink, FindLine(Lines, TEXT("googleapis")));
		ASSERT_THAT(IsNotNull(Font, Describe(Result.Report)));
		ASSERT_THAT(IsTrue(Font->Message.Contains(TEXT(".ttf")) && Font->Message.Contains(TEXT("@font-face")), Font->Message));
		ASSERT_THAT(IsFalse(ReadOut(TEXT("Page.rml")).Contains(TEXT("<link")), ReadOut(TEXT("Page.rml"))));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(C7_HiddenInput_IsKept)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), TArray<FString>{TEXT("<input type=\"hidden\" name=\"t\" value=\"1\">"), TEXT("<input type=\"text\" name=\"q\">"), TEXT("<input type=\"HIDDEN\" name=\"u\" value=\"2\">")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(0, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedElement, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("hidden"))), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(TEXT("<input type=\"hidden\" name=\"t\" value=\"1\"/>")) && Rml.Contains(TEXT("<input type=\"text\" name=\"q\"/>")) && Rml.Contains(TEXT("<input type=\"hidden\" name=\"u\" value=\"2\"/>"), ESearchCase::CaseSensitive) && !Rml.Contains(TEXT("HIDDEN"), ESearchCase::CaseSensitive), Rml));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(Q2_ReservedDataAttributesAreRemovedAndOtherDataAttributesStay)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), TArray<FString>{TEXT("<div data-for=\"x : y\" data-value=\"a\" data-id=\"7\">t</div>")});
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(2, CountKindAt(Result.Report, EArcRmlDiagnosticKind::RemovedAttribute, SrcFile(TEXT("page.html")), FindLine(Lines, TEXT("<div"))), Describe(Result.Report)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rml")).Contains(TEXT("<div data-id=\"7\">t</div>")), ReadOut(TEXT("Page.rml"))));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(InlineSvg_KeepsTheCaseOfViewBoxAndLinearGradient)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const FString Svg = TEXT("<svg viewBox=\"0 0 10 10\"><defs><linearGradient id=\"g\"><stop offset=\"0\"/></linearGradient></defs><rect width=\"10\" height=\"10\" fill=\"url(#g)\"/></svg>");
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(FString(), Svg), Result), Describe(Result.Report)));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml == ExpectedRml(FString(), Svg), FString::Printf(TEXT("got [%s]"), *Rml)));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A9_LocalPng_IsCopiedBesideTheOutputAtItsRelativePath)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WritePng(TEXT("Src/img/a.png"))));
		ASSERT_THAT(IsTrue(WritePng(TEXT("Src/img/b.png"))));
		FArcRmlConvertResult Result;
		ASSERT_THAT(IsTrue(Convert(OnePage(FString(), TEXT("<img src=\"img/a.png\"><div style=\"background-image: url(img/b.png)\">t</div>")), Result), Describe(Result.Report)));
		ASSERT_THAT(IsTrue(OutExists(TEXT("img/a.png")), TEXT("the img src image was not copied")));
		ASSERT_THAT(IsTrue(OutExists(TEXT("img/b.png")), TEXT("the style attribute url() image was not copied")));
		ASSERT_THAT(AreEqual(0, CountKind(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied), Describe(Result.Report)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rml")).Contains(TEXT("<img src=\"img/a.png\"/>")), ReadOut(TEXT("Page.rml"))));
		ASSERT_THAT(AreEqual(3, Result.Files.Num()));
		ASSERT_THAT(AreEqual(0, Result.Report.GetErrorCount(), Describe(Result.Report)));
	}

	TEST_METHOD(A9_OutsideMissingNonCookedAndRemoteImages_AreReportedNotCopiedAndLeftAsWritten)
	{
		using namespace ArcRmlConverterTestsPrivate;
		ASSERT_THAT(IsTrue(WritePng(TEXT("Src/img/b.jpg"))));
		ASSERT_THAT(IsTrue(WritePng(TEXT("x.png"))));
		const TArray<FString> Body = {TEXT("<img src=\"../x.png\">"), TEXT("<img src=\"img/none.png\">"), TEXT("<img src=\"img/b.jpg\">"), TEXT("<img src=\"https://example.com/a.png\">")};
		const TArray<FString> Lines = MakePage(TEXT("<html>"), TArray<FString>(), Body);
		FArcRmlConvertResult Result;
		bExpectLogs = true;
		ASSERT_THAT(IsTrue(Convert(JoinLines(Lines), Result), Describe(Result.Report)));
		const FString File = SrcFile(TEXT("page.html"));
		for (const FString& Image : Body)
		{
			ASSERT_THAT(AreEqual(1, CountKindAt(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied, File, FindLine(Lines, Image)), Image + TEXT(" ") + Describe(Result.Report)));
		}
		ASSERT_THAT(AreEqual(4, CountKind(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied), Describe(Result.Report)));
		const FArcRmlDiagnostic* Outside = FindAt(Result.Report, EArcRmlDiagnosticKind::AssetNotCopied, FindLine(Lines, TEXT("../x.png")));
		ASSERT_THAT(IsNotNull(Outside, Describe(Result.Report)));
		ASSERT_THAT(IsTrue(Outside->Message.Contains(TEXT("outside")), Outside->Message));
		ASSERT_THAT(IsFalse(OutExists(TEXT("img/b.jpg")) || OutExists(TEXT("img/none.png")) || OutExists(TEXT("x.png")), TEXT("a reported image was copied")));
		ASSERT_THAT(AreEqual(1, Result.Files.Num()));
		const FString Rml = ReadOut(TEXT("Page.rml"));
		ASSERT_THAT(IsTrue(Rml.Contains(TEXT("<img src=\"../x.png\"/>")), Rml));
		TArray<FString> RmlLines;
		Rml.ParseIntoArrayLines(RmlLines, false);
		ASSERT_THAT(AreEqual(0, CountErrorsAt(Result.Report, EArcRmlDiagnosticKind::MissingImage, FindLine(RmlLines, TEXT("../x.png"))), TEXT("the existing ../x.png was reported missing ") + Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountErrorsAt(Result.Report, EArcRmlDiagnosticKind::MissingImage, FindLine(RmlLines, TEXT("img/none.png"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountErrorsAt(Result.Report, EArcRmlDiagnosticKind::MissingImage, FindLine(RmlLines, TEXT("img/b.jpg"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(1, CountErrorsAt(Result.Report, EArcRmlDiagnosticKind::MissingImage, FindLine(RmlLines, TEXT("https://example.com/a.png"))), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(3, CountKind(Result.Report, EArcRmlDiagnosticKind::MissingImage), Describe(Result.Report)));
		ASSERT_THAT(AreEqual(0, CountErrorsOtherThan(Result.Report, EArcRmlDiagnosticKind::MissingImage), Describe(Result.Report)));
	}

	TEST_METHOD(Overwrite_ExistingOutputWithoutTheFlag_WritesNothingAndFailsWithOneListedError)
	{
		using namespace ArcRmlConverterTestsPrivate;
		const FString Html = OnePage(FString(), TEXT("<p>x</p>"));
		FArcRmlConvertResult First;
		ASSERT_THAT(IsTrue(Convert(Html, First), Describe(First.Report)));
		ASSERT_THAT(IsTrue(WriteText(TEXT("Out/Page.rml"), TEXT("sentinel"))));

		FArcRmlConvertResult Refused;
		ASSERT_THAT(IsFalse(Convert(Html, Refused), TEXT("the conversion ran over an existing output")));
		ASSERT_THAT(AreEqual(1, Refused.Report.Diagnostics.Num(), Describe(Refused.Report)));
		const FArcRmlDiagnostic& Failure = Refused.Report.Diagnostics[0];
		ASSERT_THAT(IsTrue(Failure.Kind == EArcRmlDiagnosticKind::ConvertFailure && Failure.Severity == EArcRmlDiagnosticSeverity::Error && Failure.Message.Contains(TEXT("Page.rml")), Describe(Refused.Report)));
		ASSERT_THAT(AreEqual(1, Refused.ExistingFiles.Num()));
		ASSERT_THAT(IsTrue(Refused.ExistingFiles[0] == NormalizeFull(OutDir / TEXT("Page.rml")), Refused.ExistingFiles[0]));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rml")) == TEXT("sentinel"), TEXT("the refused run changed the existing file")));

		FArcRmlConvertResult Replaced;
		ASSERT_THAT(IsTrue(Convert(Html, Replaced, true), Describe(Replaced.Report)));
		ASSERT_THAT(IsTrue(ReadOut(TEXT("Page.rml")) == ExpectedRml(FString(), TEXT("<p>x</p>")), ReadOut(TEXT("Page.rml"))));
	}
};
