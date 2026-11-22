// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "ArcRmlValidateTestTypes.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CQTest.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Localization/ArcRmlStringSync.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Validation/ArcRmlValidator.h"

namespace ArcRmlValidatorTestsPrivate
{
	const TCHAR* const ModelClass = TEXT("/Script/ArcRmlUITest.ArcRmlValidateTestViewModel");
	const TCHAR* const ModelAttribute = TEXT(" data-model=\"vm\"");
	const TCHAR* const LocDocumentName = TEXT("LocDoc");

	FString MakeModelTag(const FString& Extra)
	{
		return FString::Printf(TEXT("<viewmodel name=\"vm\" class=\"%s\" context=\"VM_Validate\"%s/>"), ModelClass, *Extra);
	}

	TArray<FString> MakeDocument(const TArray<FString>& HeadLines, const TArray<FString>& BodyLines, const FString& BodyAttributes)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>V</title>"));
		Lines.Append(HeadLines);
		Lines.Add(TEXT("</head>"));
		Lines.Add(FString::Printf(TEXT("<body%s>"), *BodyAttributes));
		Lines.Append(BodyLines);
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return Lines;
	}

	TArray<FString> MakeLiveDocument(const TArray<FString>& BodyLines)
	{
		return MakeDocument(TArray<FString>{MakeModelTag(FString())}, BodyLines, ModelAttribute);
	}

	TArray<FString> MakeComponent(const TArray<FString>& BodyLines)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<component>"));
		Lines.Append(BodyLines);
		Lines.Add(TEXT("</component>"));
		return Lines;
	}

	int32 FindLine(const TArray<FString>& Lines, const FString& Needle, int32 Occurrence = 0)
	{
		int32 Seen = 0;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			if (Lines[Index].Contains(Needle) && Seen++ == Occurrence)
			{
				return Index + 1;
			}
		}
		return INDEX_NONE;
	}

	int32 CountKind(const FArcRmlValidationResult& Result, EArcRmlDiagnosticKind Kind)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Result.Diagnostics)
		{
			if (Diagnostic.Kind == Kind)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountAt(const FArcRmlValidationResult& Result, EArcRmlDiagnosticKind Kind, EArcRmlDiagnosticSeverity Severity, const FString& File, int32 Line)
	{
		int32 Count = 0;
		for (const FArcRmlDiagnostic& Diagnostic : Result.Diagnostics)
		{
			if (Diagnostic.Kind == Kind && Diagnostic.Severity == Severity && Diagnostic.File == File && Diagnostic.Line == Line)
			{
				++Count;
			}
		}
		return Count;
	}

	bool HasOnly(const FArcRmlValidationResult& Result, EArcRmlDiagnosticKind Kind, EArcRmlDiagnosticSeverity Severity, const FString& File, int32 Line)
	{
		return Line > 0 && Result.Diagnostics.Num() == 1 && CountAt(Result, Kind, Severity, File, Line) == 1;
	}

	FString Describe(const FArcRmlValidationResult& Result)
	{
		TArray<FString> Lines;
		for (const FArcRmlDiagnostic& Diagnostic : Result.Diagnostics)
		{
			Lines.Add(ArcRmlValidation::FormatDiagnostic(Diagnostic));
		}
		return FString::Printf(TEXT("%d diagnostics [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}

	FArcRmlDiagnostic MakeDiagnostic(EArcRmlDiagnosticSeverity Severity, EArcRmlDiagnosticKind Kind, const FString& File, int32 Line, const FString& Message)
	{
		FArcRmlDiagnostic Diagnostic;
		Diagnostic.Severity = Severity;
		Diagnostic.Kind = Kind;
		Diagnostic.File = File;
		Diagnostic.Line = Line;
		Diagnostic.Message = Message;
		return Diagnostic;
	}

	FArcRmlValidationResult MakeResult(int32 Errors, int32 Warnings)
	{
		FArcRmlValidationResult Result;
		Result.Files = 3;
		for (int32 Index = 0; Index < Errors; ++Index)
		{
			Result.Diagnostics.Add(MakeDiagnostic(EArcRmlDiagnosticSeverity::Error, EArcRmlDiagnosticKind::UnknownField, TEXT("/ArcRmlUITest/UI/Rml/Test/Common.rcss"), 7, TEXT("bad field")));
		}
		for (int32 Index = 0; Index < Warnings; ++Index)
		{
			Result.Diagnostics.Add(MakeDiagnostic(EArcRmlDiagnosticSeverity::Warning, EArcRmlDiagnosticKind::DuplicateId, TEXT("/ArcRmlUITest/UI/Rml/Test/Common.rcss"), 0, TEXT("same id")));
		}
		return Result;
	}
}

TEST_CLASS(ArcRmlValidatorTests, "ArcRmlUI.Validator")
{
	FString RunFolder;
	FString RunDir;
	FString TablePackageName;

	BEFORE_EACH()
	{
		RunFolder = FString::Printf(TEXT("Validate/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ArcRmlUITestHelpers::GetTestContentDir() / RunFolder;
		TablePackageName = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/ST_") + ArcRmlValidatorTestsPrivate::LocDocumentName);
		ASSERT_THAT(IsFalse(RunDir.IsEmpty(), TEXT("the test plugin content directory is unknown")));
	}

	AFTER_EACH()
	{
		if (RunDir.IsEmpty())
		{
			return;
		}
		IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		TArray<FString> Files;
		Files.Add(FPackageName::LongPackageNameToFilename(TablePackageName, FPackageName::GetAssetPackageExtension()));
		if (UPackage* Package = FindPackage(nullptr, *TablePackageName))
		{
			if (UObject* Asset = FindObject<UObject>(Package, *FPackageName::GetShortName(TablePackageName)))
			{
				Asset->ClearFlags(RF_Public | RF_Standalone);
			}
			ResetLoaders(Package);
			Registry.PackageDeleted(Package);
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		Registry.ScanModifiedAssetFiles(Files);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	FString PathOf(const FString& Name)
	{
		return ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + Name);
	}

	bool WriteText(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool Write(const FString& Name, const TArray<FString>& Lines)
	{
		return WriteText(Name, FString::Join(Lines, TEXT("\n")));
	}

	bool ValidatePaths(TArray<FString>&& Paths, FArcRmlValidationResult& OutResult, bool bExpectLogs)
	{
		if (bExpectLogs)
		{
			TestRunner->AddExpectedErrorPlain(TEXT("LogArcRmlUI:"), EAutomationExpectedErrorFlags::Contains, 0);
		}
		FArcRmlValidator Validator;
		return Validator.Run(MoveTemp(Paths), OutResult);
	}

	bool Validate(FArcRmlValidationResult& OutResult, bool bExpectLogs = true)
	{
		TArray<FString> Paths;
		Paths.Add(PathOf(FString()));
		return ValidatePaths(MoveTemp(Paths), OutResult, bExpectLogs);
	}

	bool SyncDocument(const FString& Name)
	{
		TArray<FString> Paths;
		Paths.Add(PathOf(Name));
		const FArcRmlStringSyncReport Report = ArcRmlStringSync::Sync(FArcRmlUIModule::Get().GetContentRoots(), Paths);
		TArray<FString> TableFiles;
		TableFiles.Add(RunDir / TEXT("Loc") / (FString(TEXT("ST_")) + ArcRmlValidatorTestsPrivate::LocDocumentName + TEXT(".uasset")));
		IAssetRegistry::GetChecked().ScanModifiedAssetFiles(TableFiles);
		return Report.Files.Num() == 1 && Report.Files[0].Errors.IsEmpty() && IFileManager::Get().FileExists(*TableFiles[0]);
	}

	TEST_METHOD(HeadFrame_NameClashWithFrame_ReportsHeadAtTheDeclarationLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(Write(TEXT("Frame.rml"), MakeLiveDocument(TArray<FString>{TEXT("<div>f</div>")}))));
		const TArray<FString> Tab = MakeDocument(TArray<FString>{FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *PathOf(TEXT("Frame.rml"))), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div>t</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Tab.rml"), Tab)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const int32 Line = FindLine(Tab, TEXT("<viewmodel"));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::Head, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Tab.rml")), Line), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("is also declared by its frame")), Describe(Result)));
	}

	TEST_METHOD(HeadFrame_FrameDeclaringAFrame_ReportsHeadOnTheTab)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(Write(TEXT("Outer.rml"), MakeLiveDocument(TArray<FString>{TEXT("<div>o</div>")}))));
		const TArray<FString> Inner = MakeDocument(TArray<FString>{FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *PathOf(TEXT("Outer.rml")))},
			TArray<FString>{TEXT("<div>i</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Inner.rml"), Inner)));
		const TArray<FString> Tab = MakeDocument(TArray<FString>{FString::Printf(TEXT("<meta name=\"frame\" content=\"%s\"/>"), *PathOf(TEXT("Inner.rml")))},
			TArray<FString>{TEXT("<div>t</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Tab.rml"), Tab)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::Head, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Tab.rml")), 0), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("declares a frame itself")), Describe(Result)));
	}

	TEST_METHOD(UnknownClass_ReportsErrorAtTheDeclarationLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<viewmodel name=\"vm\" class=\"/Script/ArcRmlUITest.NoSuchViewModel\" context=\"VM_Validate\"/>")},
			TArray<FString>{TEXT("<div>x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownClass, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("<viewmodel"))), Describe(Result)));
	}

	TEST_METHOD(UnknownResolver_ReportsErrorAtTheDeclarationLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>{FString::Printf(TEXT("<viewmodel name=\"vm\" class=\"%s\" resolver=\"/Script/ArcRmlUITest.NoSuchResolver\"/>"), ModelClass)},
			TArray<FString>{TEXT("<div>x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownResolver, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("<viewmodel"))), Describe(Result)));
	}

	TEST_METHOD(UnknownResolverProperty_ReportsWarningForTheBogusPropertyOnly)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(
			TArray<FString>{FString::Printf(TEXT("<viewmodel name=\"vm\" class=\"%s\" resolver=\"/Script/ArcRmlUITest.ArcRmlValidateTestResolver\" ViewModelTag=\"a\" Bogus=\"b\"/>"), ModelClass)},
			TArray<FString>{TEXT("<div>x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownResolverProperty, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("<viewmodel"))),
			Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("'Bogus'")), Describe(Result)));
	}

	void ExpectOnlyAtViewModel(const TCHAR* Attributes, EArcRmlDiagnosticKind Kind, EArcRmlDiagnosticSeverity Severity, const TCHAR* MessagePart)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>{FString::Printf(TEXT("<viewmodel name=\"vm\" %s/>"), Attributes)}, TArray<FString>{TEXT("<div>x</div>")},
			ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, Kind, Severity, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("<viewmodel"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(MessagePart), Describe(Result)));
	}

	TEST_METHOD(CreateWithAnAbstractClass_ReportsViewModelCreateErrorAtTheDeclarationLine)
	{
		ExpectOnlyAtViewModel(TEXT("class=\"/Script/ModelViewViewModel.MVVMViewModelBase\" create"), EArcRmlDiagnosticKind::ViewModelCreate, EArcRmlDiagnosticSeverity::Error,
			TEXT("abstract"));
	}

	TEST_METHOD(TwoSourcesOnOneDeclaration_ReportsHeadErrorAtTheDeclarationLine)
	{
		ExpectOnlyAtViewModel(*FString::Printf(TEXT("class=\"%s\" context=\"VM_Validate\" create"), ArcRmlValidatorTestsPrivate::ModelClass), EArcRmlDiagnosticKind::Head,
			EArcRmlDiagnosticSeverity::Error, TEXT("exactly one of"));
	}

	TEST_METHOD(CreateWithOptional_ReportsHeadErrorAtTheDeclarationLine)
	{
		ExpectOnlyAtViewModel(*FString::Printf(TEXT("class=\"%s\" create optional=\"true\""), ArcRmlValidatorTestsPrivate::ModelClass), EArcRmlDiagnosticKind::Head,
			EArcRmlDiagnosticSeverity::Error, TEXT("'optional'"));
	}

	TEST_METHOD(PathWhoseLastTypeIsUnrelatedToTheClass_ReportsViewModelPathError)
	{
		ExpectOnlyAtViewModel(*FString::Printf(TEXT("class=\"%s\" path=\"GetOwningPlayer\""), ArcRmlValidatorTestsPrivate::ModelClass), EArcRmlDiagnosticKind::ViewModelPath,
			EArcRmlDiagnosticSeverity::Error, TEXT("unrelated"));
	}

	TEST_METHOD(PathSegmentTheStaticTypeLacks_ReportsViewModelPathWarning)
	{
		ExpectOnlyAtViewModel(*FString::Printf(TEXT("class=\"%s\" path=\"GetOwningPlayerPawn.Inventory\""), ArcRmlValidatorTestsPrivate::ModelClass),
			EArcRmlDiagnosticKind::ViewModelPath, EArcRmlDiagnosticSeverity::Warning, TEXT("'Inventory'"));
	}

	TEST_METHOD(PathFirstSegmentOnAGivenWidgetClass_ReportsNothingAndWithoutItReportsViewModelPathWarning)
	{
		FArcRmlViewModelDeclaration Declaration;
		Declaration.Name = TEXT("vm");
		Declaration.PropertyPath = TEXT("Stats");
		Declaration.Source = EArcRmlViewModelSource::Path;
		Declaration.Line = 3;
		const FString DocumentPath = PathOf(TEXT("Doc.rml"));
		ASSERT_THAT(IsTrue(FArcRmlModelBinding::MissesFirstPathSegment(Declaration)));
		{
			FArcRmlDiagnosticCollector Collector(EArcRmlDiagnosticEcho::Quiet);
			const UClass* const RootClasses[] = {UArcRmlValidateTestWidget::StaticClass()};
			FArcRmlModelBinding::CheckPropertyPath(Declaration, *UArcRmlValidateTestViewModel::StaticClass(), DocumentPath, RootClasses);
			ASSERT_THAT(IsTrue(Collector.GetDiagnostics().IsEmpty(), TEXT("a path that resolves on the given widget class reported a diagnostic")));
		}
		FArcRmlDiagnosticCollector Collector(EArcRmlDiagnosticEcho::Quiet);
		FArcRmlModelBinding::CheckPropertyPath(Declaration, *UArcRmlValidateTestViewModel::StaticClass(), DocumentPath);
		const TArray<FArcRmlDiagnostic>& Diagnostics = Collector.GetDiagnostics();
		ASSERT_THAT(AreEqual(1, Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Diagnostics[0].Kind == EArcRmlDiagnosticKind::ViewModelPath && Diagnostics[0].Severity == EArcRmlDiagnosticSeverity::Warning
			&& Diagnostics[0].Line == 3 && Diagnostics[0].Message.Contains(TEXT("'Stats'"))));
	}

	TEST_METHOD(ManualAndSettableDeclarations_ReportNothing)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(
			TArray<FString>{
				FString::Printf(TEXT("<viewmodel name=\"vm\" class=\"%s\" manual/>"), ModelClass),
				FString::Printf(TEXT("<viewmodel name=\"other\" class=\"%s\" context=\"VM_Validate\" settable/>"), ModelClass)},
			TArray<FString>{TEXT("<span>{{ Title }}</span>"), TEXT("<div data-model=\"other\"><span>{{ Count }}</span></div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
	}

	TEST_METHOD(UnknownField_ReportsErrorsAtTheReadLines)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<span>{{ Title }}</span>"),
			TEXT("<span>{{ Info.Label }}</span>"),
			TEXT("<span>{{ NoSuchField }}</span>"),
			TEXT("<span>{{ Info.NoSuchMember }}</span>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Doc.rml"));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("NoSuchField"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("NoSuchMember"))), Describe(Result)));
	}

	TEST_METHOD(UnknownFunction_ReportsErrorAtTheCallLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div data-event-click=\"call('', 'Ping')\">ok</div>"),
			TEXT("<div data-event-click=\"call('', 'NoSuchFunction')\">bad</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownFunction, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("NoSuchFunction"))),
			Describe(Result)));
	}

	TEST_METHOD(CallArity_ReportsErrorsForTooFewAndTooManyArguments)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div data-event-click=\"call('', 'Pick', 1)\">ok</div>"),
			TEXT("<div data-event-click=\"call('', 'Pick')\">few</div>"),
			TEXT("<div data-event-click=\"call('', 'Ping', 1)\">many</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Doc.rml"));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::CallArity, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT(">few<"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::CallArity, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT(">many<"))), Describe(Result)));
	}

	TEST_METHOD(MissingImage_ReportsErrorForAnAbsentFileOnly)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<img src=\"Present.png\"/>"),
			TEXT("<img src=\"Absent.png\"/>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));
		TArray<uint8> Bytes;
		Bytes.Add(0);
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Present.png")))));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("Absent.png"))),
			Describe(Result)));
	}

	TEST_METHOD(InputMapping_ReportsErrorAtTheMetaLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<meta name=\"input-mode\" content=\"menu\"/>"),
			TEXT("<meta name=\"input-mapping\" content=\"/ArcRmlUITest/NoSuchFolder/IMC_NoSuchContext\"/>")}, TArray<FString>{TEXT("<div>x</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::InputMapping, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("input-mapping"))),
			Describe(Result)));
	}

	TEST_METHOD(MissingLocKey_KeyAbsentFromTheTable_ReportsErrorAtTheKeyLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const FString Name = FString(LocDocumentName) + TEXT(".rml");
		ASSERT_THAT(IsTrue(Write(Name, MakeLiveDocument(TArray<FString>{TEXT("<span loc=\"Alpha\">Alpha text</span>")}))));
		ASSERT_THAT(IsTrue(SyncDocument(Name), TEXT("the String Table could not be created")));
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{TEXT("<span loc=\"Alpha\">Alpha text</span>"), TEXT("<span loc=\"Gamma\">Gamma text</span>")});
		ASSERT_THAT(IsTrue(Write(Name, Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::MissingLocKey, EArcRmlDiagnosticSeverity::Error, PathOf(Name), FindLine(Lines, TEXT("Gamma"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("'Gamma'")), Describe(Result)));
	}

	TEST_METHOD(MissingLocKey_TableOfAnotherNamespace_StillReportsTheMissingKeyButNotTheNamespace)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const FString Name = FString(LocDocumentName) + TEXT(".rml");
		ASSERT_THAT(IsTrue(Write(Name, MakeLiveDocument(TArray<FString>{TEXT("<span loc=\"Alpha\">Alpha text</span>")}))));
		ASSERT_THAT(IsTrue(SyncDocument(Name), TEXT("the String Table could not be created")));
		UStringTable* Table = FindObject<UStringTable>(nullptr, *(TablePackageName + TEXT(".ST_") + LocDocumentName));
		ASSERT_THAT(IsNotNull(Table, TEXT("the String Table was not created")));
		Table->GetMutableStringTable()->SetNamespace(FTextKey(TEXT("Wrong.Namespace")));
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{TEXT("<span loc=\"Alpha\">Alpha text</span>"), TEXT("<span loc=\"Ghost\"/>")});
		ASSERT_THAT(IsTrue(Write(Name, Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::MissingLocKey, EArcRmlDiagnosticSeverity::Error, PathOf(Name), FindLine(Lines, TEXT("Ghost"))), Describe(Result)));
		ASSERT_THAT(IsFalse(Result.Diagnostics[0].Message.Contains(TEXT("Wrong.Namespace")), Describe(Result)));
	}

	TEST_METHOD(StructuralConflict_DataIfWithDataFor_ReportsErrorAtTheElement)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div data-if=\"Count\"><span>ok</span></div>"),
			TEXT("<div data-if=\"Count\" data-for=\"n : Names\"><span>{{ n }}</span></div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::StructuralConflict, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("data-for"))),
			Describe(Result)));
	}

	TEST_METHOD(StructuralConflict_DataActionWithoutABodyModel_ReportsErrorAtTheElement)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>(), TArray<FString>{TEXT("<div>plain</div>"), TEXT("<div data-action=\"go\">go</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::StructuralConflict, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("data-action"))),
			Describe(Result)));
	}

	TEST_METHOD(DocumentActionsWithoutABodyModel_AreNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(Write(TEXT("Second.rml"), MakeDocument(TArray<FString>(), TArray<FString>{TEXT("<div>second</div>")}, FString()))));
		const TArray<FString> Lines = MakeDocument(TArray<FString>(), TArray<FString>{FString::Printf(TEXT("<div data-action=\"open:%s\">open</div>"), *PathOf(TEXT("Second.rml"))),
			TEXT("<div data-action=\"close\">close</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
	}

	TEST_METHOD(StructuralConflict_ActionPassedToAComponentWithoutABodyModel_ReportsErrorInTheComponentFile)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Button = MakeComponent(TArray<FString>{TEXT("<prop name=\"label\"/>"), TEXT("<prop name=\"action\" default=\"\"/>"),
			TEXT("<div class=\"b\" data-attr-data-action=\"$action\">{{ $label }}</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Button.rml"), Button)));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{MakeModelTag(FString())},
			TArray<FString>{FString::Printf(TEXT("<dc-import name=\"%s\" data-model=\"vm\" label=\"Go\" action=\"close\"/>"), *PathOf(TEXT("Button")))}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::StructuralConflict, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Button.rml")), FindLine(Button, TEXT("data-attr-data-action"))),
			Describe(Result)));
	}

	TEST_METHOD(EmptyDataAttrDataAction_FromAComponentWithoutAnActionProp_IsNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Button = MakeComponent(TArray<FString>{TEXT("<prop name=\"label\"/>"), TEXT("<prop name=\"action\" default=\"\"/>"),
			TEXT("<div class=\"b\" data-attr-data-action=\"$action\">{{ $label }}</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Button.rml"), Button)));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{MakeModelTag(FString())},
			TArray<FString>{FString::Printf(TEXT("<dc-import name=\"%s\" data-model=\"vm\" label=\"Go\"/>"), *PathOf(TEXT("Button")))}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1 + ArcRmlValidation::GetValidFontSheets().Num(), Result.Files));
	}

	TEST_METHOD(UndefinedVariable_ReportsErrorForAnUndeclaredCustomProperty)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div style=\"--tint: red; color: var(--tint)\">ok</div>"),
			TEXT("<div style=\"color: var(--no-such-variable)\">bad</div>"),
			TEXT("<div style=\"color: var(--no-such-variable, blue)\">fallback</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UndefinedVariable, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT(">bad<"))), Describe(Result)));
	}

	TEST_METHOD(DuplicateId_ReportsWarningAtTheSecondElement)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div id=\"same\">a</div>"),
			TEXT("<div id=\"other\">b</div>"),
			TEXT("<div id=\"same\">c</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::DuplicateId, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("id=\"same\""), 1)),
			Describe(Result)));
		ASSERT_THAT(AreEqual(0, Result.GetErrorCount()));
		ASSERT_THAT(AreEqual(1, Result.GetWarningCount()));
	}

	TEST_METHOD(MonospaceFontSize_WarnsOncePerMonospaceSubtreeWithNoAuthorFontSize)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<p>Call <code>parse(x)</code> here</p>"),
			TEXT("<pre>block</pre>"),
			TEXT("<p style=\"font-size: 14px\"><code>sized</code></p>"),
			TEXT("<pre>outer <code>inner</code></pre>"),
			TEXT("<div style=\"font-family: monospace\">styled</div>"),
			TEXT("<div style=\"font-family: 'JetBrains Mono', monospace\">listed</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Doc.rml"));
		ASSERT_THAT(AreEqual(4, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MonospaceFontSize, EArcRmlDiagnosticSeverity::Warning, File, FindLine(Lines, TEXT("parse(x)"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MonospaceFontSize, EArcRmlDiagnosticSeverity::Warning, File, FindLine(Lines, TEXT(">block<"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MonospaceFontSize, EArcRmlDiagnosticSeverity::Warning, File, FindLine(Lines, TEXT("outer"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MonospaceFontSize, EArcRmlDiagnosticSeverity::Warning, File, FindLine(Lines, TEXT(">styled<"))), Describe(Result)));
	}

	TEST_METHOD(MonospaceFontSize_LinkedSheetWithAFontSize_IsNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(Write(TEXT("Sized.rcss"), TArray<FString>{TEXT("body {"), TEXT("	font-size: 16px;"), TEXT("}")})));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sized.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<p>Call <code>parse(x)</code> here</p>"), TEXT("<pre>block</pre>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
	}

	TEST_METHOD(CleanFixtures_ReportNoDiagnosticsAndListTheRoot)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Rows = MakeComponent(TArray<FString>{TEXT("<prop name=\"items\" default=\"[]\"/>"), TEXT("<prop name=\"caption\" default=\"\"/>"),
			TEXT("<div><span>{{ $caption }}</span><div data-for=\"x : $items\"><span>{{ x }}</span></div></div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Rows.rml"), Rows)));
		const TArray<FString> Live = MakeLiveDocument(TArray<FString>{
			TEXT("<span>{{ Title }} {{ Count }} {{ Info.Label }} {{ DerivedOnly }}</span>"),
			TEXT("<div data-for=\"Names\"><span>{{ it }} {{ it_index }}</span></div>"),
			TEXT("<div data-for=\"n, i : Names\" data-event-click=\"call('', 'Pick', i)\"><span>{{ n }}</span></div>"),
			TEXT("<div data-event-click=\"call('', 'Pick', ev.offset_x)\">p</div>"),
			TEXT("<div data-if=\"ArcResolved\">{{ Info.Level }}</div>"),
			TEXT("<div style=\"--tint: red; color: var(--tint)\">v</div>"),
			TEXT("<div id=\"one\">1</div>"),
			TEXT("<div id=\"two\">2</div>"),
			FString::Printf(TEXT("<dc-import name=\"%s\" caption=\"Hi\"/>"), *PathOf(TEXT("Rows")))});
		ASSERT_THAT(IsTrue(Write(TEXT("Live.rml"), Live)));
		const TArray<FString> Keys = MakeDocument(TArray<FString>(), TArray<FString>{TEXT("<span>{{ ArcKeys.confirm }}</span>"), TEXT("<div data-if=\"ArcResolved\">r</div>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Keys.rml"), Keys)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(2 + ArcRmlValidation::GetValidFontSheets().Num(), Result.Files));
		ASSERT_THAT(IsTrue(Result.Roots.Contains(TEXT("/ArcRmlUITest/")), TEXT("the roots list lacks the compiled content root")));
	}

	TEST_METHOD(Severity_UnknownMembersAreErrorsOnAClassAndWarningsOnOptionalAndPreviewModels)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Live = MakeLiveDocument(TArray<FString>{TEXT("<span>{{ NoSuchField }}</span>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Live.rml"), Live)));
		const TArray<FString> Optional = MakeDocument(TArray<FString>{MakeModelTag(TEXT(" optional=\"true\""))},
			TArray<FString>{TEXT("<span>{{ NoSuchField }}</span>"), TEXT("<div data-event-click=\"call('', 'NoSuchFunction')\">x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Optional.rml"), Optional)));
		const TArray<FString> Preview = MakeDocument(TArray<FString>{TEXT("<viewmodel name=\"vm\" preview=\"Preview/P.json\"/>")},
			TArray<FString>{TEXT("<span>{{ NoSuchField }}</span>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Preview.rml"), Preview)));
		ASSERT_THAT(IsTrue(WriteText(TEXT("Preview/P.json"), TEXT("{ \"Title\": \"x\" }"))));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(4, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Live.rml")), FindLine(Live, TEXT("NoSuchField"))),
			Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Optional.rml")), FindLine(Optional, TEXT("NoSuchField"))),
			Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownFunction, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Optional.rml")), FindLine(Optional, TEXT("NoSuchFunction"))),
			Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Preview.rml")), FindLine(Preview, TEXT("NoSuchField"))),
			Describe(Result)));
	}

	TEST_METHOD(ComponentLine_MapsToTheComponentFile)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Row = MakeComponent(TArray<FString>{TEXT("<prop name=\"label\"/>"), TEXT("<span>{{ $label }}</span>"), TEXT("<span>{{ NoSuchInComponent }}</span>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Row.rml"), Row)));
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{TEXT("<div>pad</div>"), TEXT("<div>pad</div>"),
			FString::Printf(TEXT("<dc-import name=\"%s\" label=\"x\"/>"), *PathOf(TEXT("Row")))});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Row.rml")), FindLine(Row, TEXT("NoSuchInComponent"))),
			Describe(Result)));
	}

	TEST_METHOD(PathLimitedRun_KeepsTheBinaryOfAnOrphanedFile)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Main = MakeLiveDocument(TArray<FString>{TEXT("<span>{{ NoSuchField }}</span>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Main.rml"), Main)));
		ASSERT_THAT(IsTrue(Write(TEXT("Other.rml"), MakeLiveDocument(TArray<FString>{TEXT("<span>{{ Title }}</span>")}))));
		ASSERT_THAT(IsTrue(Write(TEXT("Kept.rml"), MakeLiveDocument(TArray<FString>{TEXT("<span>{{ Title }}</span>")}))));

		FArcRmlValidationResult First;
		ASSERT_THAT(IsTrue(Validate(First), TEXT("the validator did not run")));
		const FString OtherBinary = RunDir / TEXT("Other.rmlc");
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*OtherBinary), TEXT("the first run did not write the other binary")));
		ASSERT_THAT(IsTrue(IFileManager::Get().Delete(*(RunDir / TEXT("Other.rml"))), TEXT("the other source could not be removed")));
		const FDateTime Stamp = IFileManager::Get().GetTimeStamp(*OtherBinary);
		const FString KeptBinary = RunDir / TEXT("Kept.rmlc");
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*KeptBinary), TEXT("the first run did not write the kept binary")));
		const FDateTime KeptStamp = IFileManager::Get().GetTimeStamp(*KeptBinary);
		ASSERT_THAT(IsTrue(Write(TEXT("Kept.rml"), MakeLiveDocument(TArray<FString>{TEXT("<span>{{ Title }}</span>"), TEXT("<span>{{ Count }}</span>")}))));

		TArray<FString> Paths;
		Paths.Add(PathOf(TEXT("Main.rml")));
		FArcRmlValidationResult Second;
		ASSERT_THAT(IsTrue(ValidatePaths(MoveTemp(Paths), Second, true), TEXT("the path-limited validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Second, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Main.rml")), FindLine(Main, TEXT("NoSuchField"))),
			Describe(Second)));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*OtherBinary), TEXT("the path-limited run removed the binary of a file it was not given")));
		ASSERT_THAT(IsTrue(Stamp == IFileManager::Get().GetTimeStamp(*OtherBinary), TEXT("the path-limited run rewrote the binary of a file it was not given")));
		ASSERT_THAT(IsTrue(KeptStamp == IFileManager::Get().GetTimeStamp(*KeptBinary), TEXT("the path-limited run recompiled a changed file it was not given")));
	}

	TEST_METHOD(UiThreadStarted_ValidatorStillCompilesAndChecks)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(FArcRmlUIModule::Get().GetOrStartUIThread().IsValid(), TEXT("the UI thread did not start")));
		ASSERT_THAT(IsTrue(FArcRmlUIModule::Get().GetUIThread().IsValid(), TEXT("the UI thread does not exist")));
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{TEXT("<span>{{ NoSuchField }}</span>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator refused to run with the UI thread started")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("NoSuchField"))), Describe(Result)));
	}

	TEST_METHOD(LinkedSheetVariable_DeclaredInTheSheet_IsNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(IsTrue(Write(TEXT("Tokens.rcss"), TArray<FString>{TEXT(":root {"), TEXT("	--tok: red;"), TEXT("}"), TEXT(".x {"), TEXT("	color: var(--tok);"), TEXT("}")})));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Tokens.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div style=\"color: var(--tok)\">x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(2 + ArcRmlValidation::GetValidFontSheets().Num(), Result.Files));
	}

	TEST_METHOD(LinkedSheet_UndefinedVariableAndMissingImages_AreReportedInTheSheetFile)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Sheet = {TEXT(":root { --tok: red; }"), TEXT(".a {"), TEXT("color: red;"), TEXT("background-color: var(--missing);"), TEXT("}"), TEXT(".b {"), TEXT("color: red;"), TEXT("decorator: image(nope.png);"), TEXT("}"),
			TEXT("@spritesheet icons { src: nosheet.png; icon: 0px 0px 8px 8px; }")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheet.rcss"), Sheet)));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sheet.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div style=\"color: var(--tok)\">x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Sheet.rcss"));
		ASSERT_THAT(AreEqual(3, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UndefinedVariable, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("var(--missing)"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("nope.png"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("nosheet.png"))), Describe(Result)));
	}

	TEST_METHOD(LinkedSheet_UrlSources_ReportOnlyAbsentFilesJoinedToTheSheet)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Sheet = {TEXT(".a {"), TEXT("decorator: url(nourl.png);"), TEXT("}"), TEXT(".b {"), TEXT("background-image: url(nobackground.png);"), TEXT("}"),
			TEXT(".c {"), TEXT("mask-image: url(Present.png);"), TEXT("}"), TEXT(".d {"), TEXT("decorator: url('Present.png');"), TEXT("}"),
			TEXT(".e {"), TEXT("background-image: url(\"Present.png\");"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheets/Sheet.rcss"), Sheet)));
		TArray<uint8> Bytes;
		Bytes.Add(0);
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Sheets/Present.png")))));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sheets/Sheet.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div class=\"a b c d e\">x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Sheets/Sheet.rcss"));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("nourl.png"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("nobackground.png"))), Describe(Result)));
	}

	TEST_METHOD(InlineStyleImages_ReportOnlyAbsentFilesJoinedToTheDocument)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeLiveDocument(TArray<FString>{
			TEXT("<div style=\"decorator: url(Present.png)\">ok</div>"),
			TEXT("<div style=\"decorator: url('Present.png')\">quoted</div>"),
			TEXT("<div style=\"decorator: url(inlineurl.png)\">url</div>"),
			TEXT("<div style=\"mask-image: image(inlineimage.png)\">image</div>")});
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));
		TArray<uint8> Bytes;
		Bytes.Add(0);
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Present.png")))));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Doc.rml"));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("inlineurl.png"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("inlineimage.png"))), Describe(Result)));
	}

	TEST_METHOD(FillImage_JoinsToTheDocumentNotTheSheetFolder)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Sheet = {TEXT(".p {"), TEXT("fill-image: nofill.png;"), TEXT("}"), TEXT(".q {"), TEXT("fill-image: docfill.png;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheets/Bar.rcss"), Sheet)));
		TArray<uint8> Bytes;
		Bytes.Add(0);
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Sheets/nofill.png")))));
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("docfill.png")))));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sheets/Bar.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<progress class=\"p\" value=\"0.5\"/>"), TEXT("<progress class=\"q\" value=\"0.5\"/>"),
				TEXT("<progress value=\"0.5\" style=\"fill-image: inlinenofill.png\"/>"), TEXT("<progress value=\"0.5\" style=\"fill-image: docfill.png\"/>")},
			ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Sheets/Bar.rcss")),
			FindLine(Sheet, TEXT("nofill.png"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")),
			FindLine(Lines, TEXT("inlinenofill.png"))), Describe(Result)));
	}

	TEST_METHOD(Decorator_DefinedInTokensUsedFromAnotherFolder_JoinsToTheUsingSheet)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Tokens = {TEXT("@decorator present-deco : image {"), TEXT("image-src: deco.png;"), TEXT("}"), TEXT("@decorator absent-deco : url {"),
			TEXT("image-src: nodeco.png;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheets/Tokens.rcss"), Tokens)));
		const TArray<FString> Panel = {TEXT(".a {"), TEXT("decorator: present-deco;"), TEXT("}"), TEXT(".b {"), TEXT("background: absent-deco;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Panels/Panel.rcss"), Panel)));
		TArray<uint8> Bytes;
		Bytes.Add(0);
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Panels/deco.png")))));
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Sheets/deco.png")))));
		ASSERT_THAT(IsTrue(FFileHelper::SaveArrayToFile(Bytes, *(RunDir / TEXT("Sheets/nodeco.png")))));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sheets/Tokens.rcss\"/>"),
			TEXT("<link type=\"text/rcss\" href=\"Panels/Panel.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div class=\"a b\">x</div>"), TEXT("<div style=\"mask-image: absent-deco\">y</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Panels/Panel.rcss")),
			FindLine(Panel, TEXT("absent-deco"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::MissingImage, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")),
			FindLine(Lines, TEXT("absent-deco"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("absent-deco")) && Result.Diagnostics[0].Message.Contains(PathOf(TEXT("Sheets/Tokens.rcss"))),
			Describe(Result)));
	}

	TEST_METHOD(Decorator_UnusedWithAMissingFile_IsNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Tokens = {TEXT("@decorator unused-deco : image {"), TEXT("image-src: unused.png;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheets/Tokens.rcss"), Tokens)));
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<link type=\"text/rcss\" href=\"Sheets/Tokens.rcss\"/>"), MakeModelTag(FString())},
			TArray<FString>{TEXT("<div>x</div>")}, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
	}

	TEST_METHOD(StatementAtRules_AreSkippedAndOnlyImportWarnsAtItsLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Sheet = {TEXT("@charset \"UTF-8\";"), TEXT("@import url(other.css);"), TEXT("@namespace svg url(http://www.w3.org/2000/svg);"),
			TEXT("@layer base, utilities;"), TEXT(".a {"), TEXT("color: red;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheet.rcss"), Sheet)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Sheet.rcss"));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownAtRule, EArcRmlDiagnosticSeverity::Warning, File, FindLine(Sheet, TEXT("@import"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("`@import` is not followed; link the sheet from the document.")), Describe(Result)));
	}

	TEST_METHOD(SheetThatDoesNotParse_ReportsTheCauseAtTheFailingLine)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Sheet = {TEXT("/* nothing before */"), TEXT("@bad }"), TEXT(".a {"), TEXT("color: red;"), TEXT("}")};
		ASSERT_THAT(IsTrue(Write(TEXT("Sheet.rcss"), Sheet)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Sheet.rcss"));
		ASSERT_THAT(AreEqual(0, CountKind(Result, EArcRmlDiagnosticKind::CompileFailure), Describe(Result)));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::InvalidAtRule, EArcRmlDiagnosticSeverity::Error, File, FindLine(Sheet, TEXT("@bad"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("Unexpected '}' in the at-rule '@bad'")), Describe(Result)));
	}

	TEST_METHOD(OnClose_UnknownFunctionAndWrongArity_FollowTheModelSeverity)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Body = {TEXT("<div>x</div>")};
		const TArray<FString> Unknown = MakeDocument(TArray<FString>{MakeModelTag(TEXT(" on-close=\"NoSuchFunction\""))}, Body, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Unknown.rml"), Unknown)));
		const TArray<FString> Arity = MakeDocument(TArray<FString>{MakeModelTag(TEXT(" on-close=\"Pick\""))}, Body, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Arity.rml"), Arity)));
		const TArray<FString> Optional = MakeDocument(TArray<FString>{MakeModelTag(TEXT(" optional=\"true\" on-close=\"NoSuchFunction\""))}, Body, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Optional.rml"), Optional)));
		const TArray<FString> Valid = MakeDocument(TArray<FString>{MakeModelTag(TEXT(" on-close=\"Ping\""))}, Body, ModelAttribute);
		ASSERT_THAT(IsTrue(Write(TEXT("Valid.rml"), Valid)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(3, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownFunction, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Unknown.rml")), FindLine(Unknown, TEXT("<viewmodel"))),
			Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::CallArity, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Arity.rml")), FindLine(Arity, TEXT("<viewmodel"))),
			Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownFunction, EArcRmlDiagnosticSeverity::Warning, PathOf(TEXT("Optional.rml")), FindLine(Optional, TEXT("<viewmodel"))),
			Describe(Result)));
	}

	TEST_METHOD(ArcDocumentModel_UnknownFieldIsReportedAndArcKeysIsNot)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>(), TArray<FString>{TEXT("<span>{{ ArcKeys.confirm }}</span>"), TEXT("<span>{{ Bogus }}</span>")}, FString());
		ASSERT_THAT(IsTrue(Write(TEXT("Doc.rml"), Lines)));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		ASSERT_THAT(IsTrue(HasOnly(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, PathOf(TEXT("Doc.rml")), FindLine(Lines, TEXT("Bogus"))), Describe(Result)));
	}

	TEST_METHOD(CrossModelFixture_ReadsOfOtherModelsAreNotReported)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		FArcRmlValidationResult Result;
		TArray<FString> Paths;
		Paths.Add(ArcRmlUITestHelpers::MakePath(TEXT("Expression/CrossModelTest.rml")));
		ASSERT_THAT(IsTrue(ValidatePaths(MoveTemp(Paths), Result, false), TEXT("the validator did not run")));
		ASSERT_THAT(AreEqual(0, Result.Diagnostics.Num(), Describe(Result)));
	}

	TEST_METHOD(CrossModel_MissingFieldOfTheOtherModelIsReportedWithItsSeverity)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const TArray<FString> Lines = MakeDocument(TArray<FString>{TEXT("<viewmodel name=\"own\" preview=\"Preview/Own.json\"/>"), MakeModelTag(FString())},
			TArray<FString>{
				TEXT("<span>{{ vm.Title }} {{ vm.Info.Label }} {{ Title }}</span>"),
				TEXT("<div data-for=\"n : vm.Names\"><span>{{ n }}</span></div>"),
				TEXT("<span>{{ vm.Inptu }}</span>"),
				TEXT("<span>{{ vm.Info.NoSuchMember }}</span>")},
			TEXT(" data-model=\"own\""));
		ASSERT_THAT(IsTrue(Write(TEXT("Cross.rml"), Lines)));
		ASSERT_THAT(IsTrue(WriteText(TEXT("Preview/Own.json"), TEXT("{ \"Title\": \"x\" }"))));

		FArcRmlValidationResult Result;
		ASSERT_THAT(IsTrue(Validate(Result), TEXT("the validator did not run")));
		const FString File = PathOf(TEXT("Cross.rml"));
		ASSERT_THAT(AreEqual(2, Result.Diagnostics.Num(), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("Inptu"))), Describe(Result)));
		ASSERT_THAT(AreEqual(1, CountAt(Result, EArcRmlDiagnosticKind::UnknownField, EArcRmlDiagnosticSeverity::Error, File, FindLine(Lines, TEXT("NoSuchMember"))), Describe(Result)));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Message.Contains(TEXT("on model 'vm'")), Describe(Result)));
	}

	TEST_METHOD(ToJson_WritesSummaryRootsAndDiagnostics)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		FArcRmlValidationResult Result = MakeResult(1, 1);
		Result.Roots.Add(TEXT("/ArcRmlUITest/"));

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ArcRmlValidation::ToJson(Result));
		ASSERT_THAT(IsTrue(FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid(), TEXT("the JSON does not parse")));
		const TSharedPtr<FJsonObject>* Summary = nullptr;
		ASSERT_THAT(IsTrue(Root->TryGetObjectField(TEXT("summary"), Summary), TEXT("no summary object")));
		ASSERT_THAT(AreEqual(3, static_cast<int32>((*Summary)->GetNumberField(TEXT("files")))));
		ASSERT_THAT(AreEqual(1, static_cast<int32>((*Summary)->GetNumberField(TEXT("errors")))));
		ASSERT_THAT(AreEqual(1, static_cast<int32>((*Summary)->GetNumberField(TEXT("warnings")))));
		const TArray<TSharedPtr<FJsonValue>>* Roots = nullptr;
		ASSERT_THAT(IsTrue(Root->TryGetArrayField(TEXT("roots"), Roots) && Roots->Num() == 1 && (*Roots)[0]->AsString() == TEXT("/ArcRmlUITest/"), TEXT("roots wrong")));
		const TArray<TSharedPtr<FJsonValue>>* Diagnostics = nullptr;
		ASSERT_THAT(IsTrue(Root->TryGetArrayField(TEXT("diagnostics"), Diagnostics) && Diagnostics->Num() == 2, TEXT("diagnostics wrong")));
		const TSharedPtr<FJsonObject> Entry = (*Diagnostics)[0]->AsObject();
		ASSERT_THAT(IsTrue(Entry.IsValid()));
		ASSERT_THAT(AreEqual(FString(TEXT("/ArcRmlUITest/UI/Rml/Test/Common.rcss")), Entry->GetStringField(TEXT("packagePath"))));
		ASSERT_THAT(IsTrue(FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(ArcRmlUITestHelpers::GetTestContentDir() / TEXT("Common.rcss")), Entry->GetStringField(TEXT("file")))));
		ASSERT_THAT(AreEqual(7, static_cast<int32>(Entry->GetNumberField(TEXT("line")))));
		ASSERT_THAT(AreEqual(FString(TEXT("error")), Entry->GetStringField(TEXT("severity"))));
		ASSERT_THAT(AreEqual(FString(TEXT("unknown-field")), Entry->GetStringField(TEXT("code"))));
		ASSERT_THAT(AreEqual(FString(TEXT("bad field")), Entry->GetStringField(TEXT("message"))));
		const TSharedPtr<FJsonObject> Second = (*Diagnostics)[1]->AsObject();
		ASSERT_THAT(IsTrue(Second.IsValid()));
		ASSERT_THAT(AreEqual(FString(TEXT("warning")), Second->GetStringField(TEXT("severity"))));
		ASSERT_THAT(AreEqual(FString(TEXT("duplicate-id")), Second->GetStringField(TEXT("code"))));
		ASSERT_THAT(AreEqual(0, static_cast<int32>(Second->GetNumberField(TEXT("line")))));
	}

	TEST_METHOD(GetExitCode_MapsErrorsAndWarningsAsErrors)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		ASSERT_THAT(AreEqual(2, ArcRmlValidation::FailedExitCode));
		ASSERT_THAT(AreEqual(0, ArcRmlValidation::GetExitCode(MakeResult(0, 0), false)));
		ASSERT_THAT(AreEqual(0, ArcRmlValidation::GetExitCode(MakeResult(0, 0), true)));
		ASSERT_THAT(AreEqual(0, ArcRmlValidation::GetExitCode(MakeResult(0, 2), false)));
		ASSERT_THAT(AreEqual(2, ArcRmlValidation::GetExitCode(MakeResult(0, 2), true)));
		ASSERT_THAT(AreEqual(2, ArcRmlValidation::GetExitCode(MakeResult(1, 0), false)));
		ASSERT_THAT(AreEqual(2, ArcRmlValidation::GetExitCode(MakeResult(1, 2), true)));
	}

	TEST_METHOD(Format_WritesTheDiagnosticLineAndTheSummary)
	{
		using namespace ArcRmlValidatorTestsPrivate;
		const FArcRmlValidationResult Result = MakeResult(1, 1);
		const FString Disk = ArcRmlValidation::ToDiskPath(Result.Diagnostics[0].File);
		ASSERT_THAT(AreEqual(FString::Printf(TEXT("%s(7): error unknown-field: bad field"), *Disk), ArcRmlValidation::FormatDiagnostic(Result.Diagnostics[0])));
		ASSERT_THAT(AreEqual(FString::Printf(TEXT("%s: warning duplicate-id: same id"), *Disk), ArcRmlValidation::FormatDiagnostic(Result.Diagnostics[1])));
		ASSERT_THAT(AreEqual(FString(TEXT("ArcRmlValidate: 3 files, 1 errors, 1 warnings")), ArcRmlValidation::FormatSummary(Result)));
	}
};
