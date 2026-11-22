// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "Compile/ArcRmlCompiledFile.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "MVVM/ArcRmlPreviewData.h"
#include "MVVM/ArcRmlViewModelDeclarations.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"

namespace ArcRmlPreviewSchemaTestsPrivate
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

	const TCHAR* const SchemaDocument = TEXT(
		"<rml>\n"
		"<head>\n"
		"\t<title>Preview Schema Test</title>\n"
		"\t<link type=\"text/rcss\" href=\"/ArcRmlUITest/UI/Rml/Test/Common.rcss\"/>\n"
		"\t<viewmodel name=\"pv\" preview=\"Preview/SchemaTest.json\"/>\n"
		"</head>\n"
		"<body data-model=\"pv\">\n"
		"\t<div id=\"static-part\">static</div>\n"
		"\t<div id=\"preview-part\" data-if=\"ArcResolved &amp;&amp; showPreview\"><div id=\"preview-child\">{{ label }}</div></div>\n"
		"\t<div id=\"live-part\" data-if=\"!ArcResolved || showLive\"><div id=\"live-child\">live</div></div>\n"
		"\t<div id=\"styled-part\" data-style-color=\"tint\">styled</div>\n"
		"\t<div id=\"nested-part\">{{ stats.hp }}</div>\n"
		"\t<div id=\"label-part\">{{ label }}</div>\n"
		"\t<div id=\"rows-part\"><div class=\"row\" data-for=\"row : rows\">{{ row.name }}</div></div>\n"
		"</body>\n"
		"</rml>\n");

	const TCHAR* const SchemaJson = TEXT(
		"{\n"
		"\t\"showPreview\": true,\n"
		"\t\"showLive\": false,\n"
		"\t\"label\": \"Preview\",\n"
		"\t\"tint\": { \"$colour\": \"#ff0000ff\" },\n"
		"\t\"stats\": { \"hp\": 7, \"speed\": 1.5 },\n"
		"\t\"rows\": [ { \"name\": \"a\" }, { \"name\": \"b\" } ]\n"
		"}\n");

	const TCHAR* const GateDocument = TEXT(
		"<rml>\n"
		"<head>\n"
		"\t<title>Missing Name Gate Test</title>\n"
		"\t<link type=\"text/rcss\" href=\"/ArcRmlUITest/UI/Rml/Test/Common.rcss\"/>\n"
		"\t<viewmodel name=\"pv\" preview=\"Preview/Absent.json\"/>\n"
		"</head>\n"
		"<body data-model=\"pv\">\n"
		"\t<div id=\"static-part\">static</div>\n"
		"\t<div id=\"gate\" data-if=\"showGate\"><div id=\"gate-child\">child</div></div>\n"
		"</body>\n"
		"</rml>\n");

	void SetPreviewHidden(bool bHidden)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("ArcRml.PreviewHidden")))
		{
			Variable->Set(bHidden ? 1 : 0, ECVF_SetByCode);
		}
	}

	const FArcRmlValue* FindMember(const FArcRmlValue& Value, const TCHAR* Name)
	{
		const int32 Index = ArcRmlPreviewData::FindMember(Value.MemberNames, Name);
		return Index == INDEX_NONE ? nullptr : &Value.Children[Index];
	}
}

TEST_CLASS(ArcRmlPreviewSchemaTests, "ArcRmlUI.PreviewSchema")
{
	FArcRmlTestView View;
	FString RuntimeDir;

	AFTER_EACH()
	{
		View.Close();
		ArcRmlPreviewSchemaTestsPrivate::SetPreviewHidden(false);
		if (!RuntimeDir.IsEmpty())
		{
			IFileManager::Get().DeleteDirectory(*RuntimeDir, false, true);
			IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RuntimeDir), false, false);
			RuntimeDir.Reset();
		}
	}

	FString MakeRunFolder()
	{
		const FString RunFolder = FString::Printf(TEXT("SchemaRuntime/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RuntimeDir = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), RunFolder);
		return RunFolder;
	}

	bool WriteRuntimeFile(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *FPaths::Combine(RuntimeDir, Name));
	}

	FString WriteSchemaDocument()
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		if (!WriteRuntimeFile(TEXT("SchemaTest.rml"), SchemaDocument) || !WriteRuntimeFile(TEXT("Preview/SchemaTest.json"), SchemaJson))
		{
			return FString();
		}
		return ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/SchemaTest.rml"));
	}

	TEST_METHOD(ForkBehavior_FailedDataIfNeverBuildsItsChildren)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("GateTest.rml"), GateDocument)));
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/GateTest.rml"));
		TestRunner->AddExpectedMessagePlain(TEXT("Could not find data variable with name 'showGate'"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
		TestRunner->AddExpectedMessagePlain(TEXT("Could not add data-if view"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));

		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadLayout(View, Elements)));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("static-part"))));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("gate"))));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("gate-child")), TEXT("a data-if that failed to initialize instanced its children")));
	}

	void ExpectSchemaOnlyView(const ArcRmlPreviewSchemaTestsPrivate::FLogLineCounter& FailedExpressions, const ArcRmlPreviewSchemaTestsPrivate::FLogLineCounter& FailedViews)
	{
		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadLayout(View, Elements)));
		GLog->Flush();
		ASSERT_THAT(AreEqual(0, FailedExpressions.GetCount(), TEXT("an expression named a preview member the schema does not bind")));
		ASSERT_THAT(AreEqual(0, FailedViews.GetCount(), TEXT("a data view failed without preview data")));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(IsNotNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("live-child")), TEXT("a live section gated on !ArcResolved did not build its children")));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("preview-child")), TEXT("a preview-only section shows without preview data")));
		ASSERT_THAT(AreEqual(FString(TEXT("0")), ArcRmlUITestHelpers::GetText(Elements, TEXT("nested-part")), TEXT("a nested member did not bind its schema default")));
		ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("label-part")), TEXT("a preview value was bound without preview data")));
	}

	TEST_METHOD(HiddenPreviewData_BindsOnlyTheSchema)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString Path = WriteSchemaDocument();
		ASSERT_THAT(IsFalse(Path.IsEmpty(), TEXT("the runtime files could not be written")));
		SetPreviewHidden(true);
		FLogLineCounter FailedExpressions(TEXT("Error in data expression"));
		FLogLineCounter FailedViews(TEXT("Could not add data-"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ExpectSchemaOnlyView(FailedExpressions, FailedViews);
	}

	TEST_METHOD(PackagedShape_AbsentPreviewData_BindsTheCompiledSchema)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString Path = WriteSchemaDocument();
		ASSERT_THAT(IsFalse(Path.IsEmpty(), TEXT("the runtime files could not be written")));
		const FString CompiledFile = FPaths::Combine(RuntimeDir, TEXT("SchemaTest.rmlc"));
		ASSERT_THAT(IsTrue(ArcRmlViewModelDeclarations::ReadDocumentHead(Path).IsSet(), TEXT("the document did not compile with its preview data")));
		FArcRmlCompiledHeader WithData;
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::LoadHeader(CompiledFile, EArcRmlCompiledKind::Document, WithData) == EArcRmlCompiledLoad::Loaded));
		ASSERT_THAT(AreEqual(1, WithData.PreviewSchemas.Num()));

		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*FPaths::Combine(RuntimeDir, TEXT("Preview/SchemaTest.json"))), TEXT("the preview data could not be removed")));
		ASSERT_THAT(IsTrue(ArcRmlViewModelDeclarations::ReadDocumentHead(Path).IsSet(), TEXT("the document did not compile without its preview data")));
		FArcRmlCompiledHeader WithoutData;
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::LoadHeader(CompiledFile, EArcRmlCompiledKind::Document, WithoutData) == EArcRmlCompiledLoad::Loaded));
		FArcRmlCompiledFileData Compiled;
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::Load(CompiledFile, EArcRmlCompiledKind::Document, WithoutData.RegistryHash, Compiled) == EArcRmlCompiledLoad::Loaded));

		// A packaged binary is compiled with the preview data and read without it; its source hash must stay fresh so the editor keeps this binary.
		FArcRmlCompiledHeader Packaged = Compiled.Header;
		Packaged.PreviewSchemas = WithData.PreviewSchemas;
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::SaveAtomic(CompiledFile, EArcRmlCompiledKind::Document, Packaged, Compiled.GetBody(), TConstArrayView<uint8>())));

		SetPreviewHidden(false);
		FLogLineCounter FailedExpressions(TEXT("Error in data expression"));
		FLogLineCounter FailedViews(TEXT("Could not add data-"));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		ExpectSchemaOnlyView(FailedExpressions, FailedViews);
	}

	TEST_METHOD(UnparsablePreviewData_FailsTheCompileAtItsLine)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString RunFolder = MakeRunFolder();
		const FString Path = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/SchemaTest.rml"));
		const FString PreviewPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Preview/SchemaTest.json"));
		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("SchemaTest.rml"), SchemaDocument) && WriteRuntimeFile(TEXT("Preview/SchemaTest.json"), TEXT("{\n\t\"label\": \"x\",\n\t\"broken\": tru\n}\n"))));
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, PreviewPath);
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, Path);
		ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::OpenView(View, Path), TEXT("a document whose preview data does not parse compiled and opened")));
		ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("preview model 'pv' cannot parse its preview data"), PreviewPath, 3),
			ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
	}

	TEST_METHOD(PreviewWithData_OverlaysTheSchema)
	{
		const FString Path = WriteSchemaDocument();
		ASSERT_THAT(IsFalse(Path.IsEmpty(), TEXT("the runtime files could not be written")));
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));

		TArray<FArcRmlTestLayoutElement> Elements;
		ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::ReadLayout(View, Elements)));
		ASSERT_THAT(AreEqual(0, View.GetViewDiagnostics().Diagnostics.Num(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
		ASSERT_THAT(AreEqual(FString(TEXT("Preview")), ArcRmlUITestHelpers::GetText(Elements, TEXT("preview-child"))));
		ASSERT_THAT(IsNull(ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("live-child")), TEXT("showLive=false did not hide the live section with preview data")));
		ASSERT_THAT(AreEqual(FString(TEXT("7")), ArcRmlUITestHelpers::GetText(Elements, TEXT("nested-part"))));
	}

	TEST_METHOD(StrippedCompiledDocument_KeepsThePreviewSchema)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString Path = WriteSchemaDocument();
		ASSERT_THAT(IsFalse(Path.IsEmpty(), TEXT("the runtime files could not be written")));
		const FString SourceFile = FPaths::Combine(RuntimeDir, TEXT("SchemaTest.rml"));
		ASSERT_THAT(IsTrue(ArcRmlViewModelDeclarations::ReadDocumentHead(Path).IsSet(), TEXT("the document did not compile")));

		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*SourceFile), TEXT("the document source could not be removed")));
		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*FPaths::Combine(RuntimeDir, TEXT("Preview/SchemaTest.json"))), TEXT("the preview data could not be removed")));
		TArray<uint8> CompiledBytes;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToArray(CompiledBytes, *(SourceFile + TEXT("c")))));
		ASSERT_THAT(IsTrue(ArcRmlCompiledFile::PrepareForCook(EArcRmlCompiledKind::Document, CompiledBytes), TEXT("the compiled file could not be stripped like a cooked one")));
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(CompiledBytes, *(SourceFile + TEXT("c")))));

		const TOptional<FArcRmlDocumentHead> Head = ArcRmlViewModelDeclarations::ReadCompiledHead(Path);
		ASSERT_THAT(IsTrue(Head.IsSet(), TEXT("the head was not read from the stripped compiled file")));
		ASSERT_THAT(AreEqual(1, Head->Declarations.Num()));
		const FArcRmlValue& Schema = Head->Declarations[0].PreviewSchema;
		ASSERT_THAT(IsTrue(Schema.Kind == EArcRmlValueKind::Struct));
		ASSERT_THAT(AreEqual(6, Schema.MemberNames.Num(), FString::Join(Schema.MemberNames, TEXT(", "))));

		const FArcRmlValue* ShowPreview = FindMember(Schema, TEXT("showPreview"));
		ASSERT_THAT(IsTrue(ShowPreview && ShowPreview->Kind == EArcRmlValueKind::Bool && !ShowPreview->bBool, TEXT("the bool member is missing or carries its value")));
		const FArcRmlValue* Label = FindMember(Schema, TEXT("label"));
		ASSERT_THAT(IsTrue(Label && Label->Kind == EArcRmlValueKind::String && Label->String.IsEmpty(), TEXT("the string member is missing or carries its value")));
		const FArcRmlValue* Tint = FindMember(Schema, TEXT("tint"));
		ASSERT_THAT(IsTrue(Tint && Tint->Kind == EArcRmlValueKind::Colour && Tint->Colour == FColor::Transparent, TEXT("the colour member is missing or carries its value")));
		const FArcRmlValue* Rows = FindMember(Schema, TEXT("rows"));
		ASSERT_THAT(IsTrue(Rows && Rows->Kind == EArcRmlValueKind::Array && Rows->Children.IsEmpty(), TEXT("the array member is missing or carries rows")));
		const FArcRmlValue* Stats = FindMember(Schema, TEXT("stats"));
		ASSERT_THAT(IsTrue(Stats && Stats->Kind == EArcRmlValueKind::Struct, TEXT("the nested struct member is missing")));
		const FArcRmlValue* Hp = FindMember(*Stats, TEXT("hp"));
		ASSERT_THAT(IsTrue(Hp && Hp->Kind == EArcRmlValueKind::Int && Hp->Int == 0, TEXT("the nested int member is missing or carries its value")));
		const FArcRmlValue* Speed = FindMember(*Stats, TEXT("speed"));
		ASSERT_THAT(IsTrue(Speed && Speed->Kind == EArcRmlValueKind::Float && Speed->Float == 0.0, TEXT("the nested float member is missing or carries its value")));
	}

	TEST_METHOD(PreviewJsonEdit_RecompilesTheSchema)
	{
		using namespace ArcRmlPreviewSchemaTestsPrivate;
		const FString Path = WriteSchemaDocument();
		ASSERT_THAT(IsFalse(Path.IsEmpty(), TEXT("the runtime files could not be written")));
		const TOptional<FArcRmlDocumentHead> First = ArcRmlViewModelDeclarations::ReadDocumentHead(Path);
		ASSERT_THAT(IsTrue(First.IsSet(), TEXT("the document did not compile")));
		ASSERT_THAT(IsNull(FindMember(First->Declarations[0].PreviewSchema, TEXT("extra"))));

		ASSERT_THAT(IsTrue(WriteRuntimeFile(TEXT("Preview/SchemaTest.json"), TEXT("{ \"label\": \"x\", \"extra\": 3 }"))));
		const TOptional<FArcRmlDocumentHead> Second = ArcRmlViewModelDeclarations::ReadDocumentHead(Path);
		ASSERT_THAT(IsTrue(Second.IsSet(), TEXT("the document did not compile again")));
		const FArcRmlValue& Schema = Second->Declarations[0].PreviewSchema;
		ASSERT_THAT(AreEqual(2, Schema.MemberNames.Num(), TEXT("the compiled schema did not follow the edited preview data")));
		const FArcRmlValue* Extra = FindMember(Schema, TEXT("extra"));
		ASSERT_THAT(IsTrue(Extra && Extra->Kind == EArcRmlValueKind::Int));
	}
};
