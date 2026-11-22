// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Curves/CurveFloat.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Localization/ArcRmlStringSync.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace ArcRmlStringSyncTestsPrivate
{
	const TCHAR* const DocumentName = TEXT("SyncDoc");
	const int32 DuplicateLine = 8;
	const int32 NoTextLine = 9;
	const int32 BoundLine = 10;

	FString MakeDocumentText(const FString& GammaText)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>Sync Test</title>"));
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body data-model=\"m\">"));
		Lines.Add(TEXT("<span loc=\"Alpha\">Alpha text</span>"));
		Lines.Add(TEXT("<span loc=\"Dup\">One</span>"));
		Lines.Add(TEXT("<span loc=\"Dup\">Two</span>"));
		Lines.Add(TEXT("<span loc=\"NoText\"/>"));
		Lines.Add(TEXT("<dc-import name=\"/ArcRmlUITest/UI/Rml/Test/Comp/CallButton\" loc-label=\"Bound\" label=\"{{ x }}\"/>"));
		Lines.Add(FString::Printf(TEXT("<dc-import name=\"/ArcRmlUITest/UI/Rml/Test/Comp/CallButton\" loc-label=\"Gamma\" label=\"%s\"/>"), *GammaText));
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	const FArcRmlStringSyncError* FindErrorAt(const FArcRmlStringSyncFile& File, int32 Line, const TCHAR* MessagePart)
	{
		for (const FArcRmlStringSyncError& Error : File.Errors)
		{
			if (Error.Line == Line && Error.Message.Contains(MessagePart))
			{
				return &Error;
			}
		}
		return nullptr;
	}

	bool ContainsEntry(const TArray<FArcRmlStringSyncEntry>& Entries, const FString& Key, const FString& Text)
	{
		for (const FArcRmlStringSyncEntry& Entry : Entries)
		{
			if (Entry.Key == Key && Entry.Text == Text)
			{
				return true;
			}
		}
		return false;
	}

	FString MakeCleanDocumentText(const FString& BodyLines)
	{
		return FString::Join(TArray<FString>{TEXT("<rml>"), TEXT("<head>"), TEXT("<title>Sync Test</title>"), TEXT("</head>"), TEXT("<body data-model=\"m\">"), BodyLines, TEXT("</body>"), TEXT("</rml>")}, TEXT("\n"));
	}

	FString DescribeErrors(const FArcRmlStringSyncFile& File)
	{
		TArray<FString> Lines;
		for (const FArcRmlStringSyncError& Error : File.Errors)
		{
			Lines.Add(FString::Printf(TEXT("%d: %s"), Error.Line, *Error.Message));
		}
		return FString::Printf(TEXT("%d errors [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}
}

TEST_CLASS(ArcRmlStringSyncTests, "ArcRmlUI.StringSync")
{
	FString RunDir;
	FString RunFolder;
	FString DocumentFile;
	FString DocumentPath;
	FString TableObjectPath;
	FString TablePackageName;
	FString TableFile;
	TArray<FString> ExtraTablePackages;

	BEFORE_EACH()
	{
		const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
		RunFolder = FString::Printf(TEXT("StringSync/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ContentDir / RunFolder;
		DocumentFile = RunDir / (FString(ArcRmlStringSyncTestsPrivate::DocumentName) + TEXT(".rml"));
		DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + ArcRmlStringSyncTestsPrivate::DocumentName + TEXT(".rml"));
		const FString TableName = FString(TEXT("ST_")) + ArcRmlStringSyncTestsPrivate::DocumentName;
		TablePackageName = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/") + TableName);
		TableObjectPath = TablePackageName + TEXT(".") + TableName;
		TableFile = RunDir / (FString(TEXT("Loc/")) + TableName + TEXT(".uasset"));
		ASSERT_THAT(IsFalse(ContentDir.IsEmpty(), TEXT("the test plugin content directory is unknown")));
		ASSERT_THAT(IsTrue(WriteDocument(TEXT("Gamma text")), TEXT("the fixture document could not be written")));
	}

	AFTER_EACH()
	{
		if (RunDir.IsEmpty())
		{
			return;
		}
		IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		TArray<FString> PackageNames;
		PackageNames.Add(TablePackageName);
		PackageNames.Append(ExtraTablePackages);
		TArray<FString> Files;
		Files.Add(TableFile);
		for (const FString& PackageName : PackageNames)
		{
			Files.Add(FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()));
			if (UPackage* Package = FindPackage(nullptr, *PackageName))
			{
				if (UObject* Asset = FindObject<UObject>(Package, *FPackageName::GetShortName(PackageName)))
				{
					Asset->ClearFlags(RF_Public | RF_Standalone);
				}
				ResetLoaders(Package);
				Registry.PackageDeleted(Package);
			}
		}
		IFileManager::Get().DeleteDirectory(*RunDir, false, true);
		Registry.ScanModifiedAssetFiles(Files);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	bool WriteDocument(const FString& GammaText)
	{
		return FFileHelper::SaveStringToFile(ArcRmlStringSyncTestsPrivate::MakeDocumentText(GammaText), *DocumentFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool WriteRaw(const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *DocumentFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	void ExpectLogged(int32 Line, const FString& Message)
	{
		ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, FString::Printf(TEXT("%s(%d): %s"), *DocumentPath, Line, *Message));
	}

	FArcRmlStringSyncFile SyncOnce()
	{
		ExpectLogged(ArcRmlStringSyncTestsPrivate::BoundLine, TEXT("prop 'label' has a loc key and a {{ }} binding"));
		return SyncPlain();
	}

	FArcRmlStringSyncReport SyncReport(const FString& Path)
	{
		TArray<FString> Paths;
		Paths.Add(Path);
		return ArcRmlStringSync::Sync(FArcRmlUIModule::Get().GetContentRoots(), Paths);
	}

	FArcRmlStringSyncFile SyncPlain()
	{
		const FArcRmlStringSyncReport Report = SyncReport(DocumentPath);
		return Report.Files.Num() == 1 ? Report.Files[0] : FArcRmlStringSyncFile();
	}

	void ExpectLoggedPart(const FString& Part)
	{
		TestRunner->AddExpectedErrorPlain(Part, EAutomationExpectedErrorFlags::Contains, 0);
	}

	FString MakeRunPath(const FString& Name)
	{
		return ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + Name);
	}

	bool WriteRunFile(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	FString GetNote(UStringTable& Table, const FString& Key)
	{
		FString Note;
		Table.GetStringTable()->EnumerateSourceStrings([&Key, &Note](const FString& EntryKey, const FString& SourceString, const FString& Notes) -> bool
		{
			if (EntryKey == Key)
			{
				Note = Notes;
			}
			return true;
		});
		return Note;
	}

	UStringTable* FindTable()
	{
		return FindObject<UStringTable>(nullptr, *TableObjectPath);
	}

	TEST_METHOD(FindDocuments_ListsTheFixtureDocument)
	{
		TArray<FString> Paths;
		ArcRmlStringSync::FindDocuments(FArcRmlUIModule::Get().GetContentRoots(), RunDir, Paths);
		ASSERT_THAT(AreEqual(1, Paths.Num()));
		ASSERT_THAT(AreEqual(DocumentPath, Paths[0]));
	}

	TEST_METHOD(FirstSync_CreatesTableWithNamespaceAndKeys)
	{
		const FArcRmlStringSyncFile File = SyncOnce();
		ASSERT_THAT(AreEqual(DocumentPath, File.Path));
		ASSERT_THAT(AreEqual(TableObjectPath, File.TableObjectPath));
		ASSERT_THAT(IsTrue(File.bTableCreated));
		ASSERT_THAT(AreEqual(2, File.Added.Num()));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Added, TEXT("Alpha"), TEXT("Alpha text"))));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Added, TEXT("Gamma"), TEXT("Gamma text"))));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));

		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table, TEXT("the String Table was not created")));
		ASSERT_THAT(AreEqual(DocumentPath, Table->GetStringTable()->GetNamespace()));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Alpha text")), Text));
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Gamma")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Gamma text")), Text));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Dup")), Text), TEXT("a rejected key was written")));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("NoText")), Text), TEXT("a key without text was written")));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Bound")), Text), TEXT("a bound prop key was written")));
		ASSERT_THAT(IsTrue(FPaths::FileExists(TableFile), TEXT("the String Table package was not saved")));
	}

	TEST_METHOD(SecondSyncWithoutEdits_ReportsNothingToWrite)
	{
		SyncOnce();
		ExpectLogged(ArcRmlStringSyncTestsPrivate::BoundLine, TEXT("prop 'label' has a loc key and a {{ }} binding"));
		const FArcRmlStringSyncReport Report = SyncReport(DocumentPath);
		ASSERT_THAT(AreEqual(1, Report.Files.Num()));
		const FArcRmlStringSyncFile File = Report.Files[0];
		ASSERT_THAT(AreEqual(DocumentPath, File.Path));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
	}

	TEST_METHOD(ChangedSourceText_UpdatesTheEntry)
	{
		SyncOnce();
		ASSERT_THAT(IsTrue(WriteDocument(TEXT("Gamma edited")), TEXT("the fixture document could not be rewritten")));
		const FArcRmlStringSyncFile File = SyncOnce();
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(AreEqual(1, File.Changed.Num()));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Changed, TEXT("Gamma"), TEXT("Gamma edited"))));

		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Gamma")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Gamma edited")), Text));
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Alpha text")), Text));
	}

	TEST_METHOD(KeyAbsentFromTheDocument_IsListedUnusedAndKept)
	{
		SyncOnce();
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		Table->GetMutableStringTable()->SetSourceString(FTextKey(TEXT("Stray")), TEXT("Stray text"), FString());

		const FArcRmlStringSyncFile File = SyncOnce();
		ASSERT_THAT(AreEqual(1, File.Unused.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("Stray")), File.Unused[0]));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Stray")), Text), TEXT("the unused key was deleted")));
	}

	TEST_METHOD(Errors_AreReportedWithTheirLine)
	{
		const FArcRmlStringSyncFile File = SyncOnce();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(3, File.Errors.Num(), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, ArcRmlStringSyncTestsPrivate::DuplicateLine, TEXT("'Dup'")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, ArcRmlStringSyncTestsPrivate::NoTextLine, TEXT("'NoText'")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, ArcRmlStringSyncTestsPrivate::BoundLine, TEXT("prop 'label' has a loc key and a {{ }} binding")), Described));
	}

	TEST_METHOD(ExistingTableWithAnotherNamespace_IsReportedAndNotWritten)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>")))));
		ASSERT_THAT(IsTrue(SyncPlain().bTableCreated));
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		Table->GetMutableStringTable()->SetNamespace(FTextKey(TEXT("Wrong.Namespace")));
		Table->GetMutableStringTable()->SetSourceString(FTextKey(TEXT("Stray")), TEXT("Stray text"), FString());
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha edited</span>\n<span loc=\"Beta\">Beta text</span>")))));

		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(1, File.Errors.Num(), Described));
		ASSERT_THAT(AreEqual(0, File.Errors[0].Line, Described));
		ASSERT_THAT(IsTrue(File.Errors[0].Message.Contains(TablePackageName) && File.Errors[0].Message.Contains(TEXT("Wrong.Namespace")) && File.Errors[0].Message.Contains(DocumentPath), Described));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Alpha text")), Text));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Beta")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Wrong.Namespace")), Table->GetStringTable()->GetNamespace()));
	}

	TEST_METHOD(InvalidKeyName_IsReportedAtItsLineAndNotWritten)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>\n<span loc=\"bad key\">Bad</span>")))));
		ExpectLogged(7, TEXT("loc key 'bad key' on <span> must be made of"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(1, File.Errors.Num(), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("must be made of")), Described));
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("bad key")), Text)));
	}

	TEST_METHOD(LocOnElementWithChildren_IsReportedWithoutANoTextError)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>\n<span loc=\"Kids\"><b>Bold</b></span>")))));
		ExpectLogged(7, TEXT("<span loc=\"Kids\"> holds a child element; loc needs plain text or no children"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(1, File.Errors.Num(), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("holds a child element")), Described));
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(IsFalse(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Kids")), Text)));
	}

	TEST_METHOD(UnterminatedTag_ReportsItsLineAndSyncsNothing)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>\n<div class=\"open")))));
		ExpectLogged(7, TEXT("unterminated tag"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(2, File.Errors.Num(), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("unterminated tag")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 0, TEXT("nothing was synced")), Described));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
		ASSERT_THAT(IsNull(FindTable()));
		ASSERT_THAT(IsFalse(FPaths::FileExists(TableFile)));
	}

	TEST_METHOD(OmittedLocProp_TakesTheComponentPropDefaultAsSourceText)
	{
		const FString ComponentFile = RunDir / TEXT("DefaultBox.rml");
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("<component>\n<prop name=\"label\" default=\"Hello\"/>\n<div>{{ $label }}</div>\n</component>"), *ComponentFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));
		const FString ComponentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/DefaultBox"));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(FString::Printf(TEXT("<dc-import name=\"%s\" loc-label=\"FromDefault\"/>"), *ComponentPath)))));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(IsTrue(File.Errors.IsEmpty(), Described));
		ASSERT_THAT(AreEqual(1, File.Added.Num()));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Added, TEXT("FromDefault"), TEXT("Hello"))));
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("FromDefault")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Hello")), Text));
	}

	TEST_METHOD(ComponentFile_SyncsIntoItsOwnTableWithLiteralPropText)
	{
		ASSERT_THAT(IsTrue(WriteRunFile(TEXT("Box.rml"), TEXT("<component>\n<prop name=\"title\"/>\n<span loc=\"Own\">Own text</span>\n<dc-import name=\"/ArcRmlUITest/UI/Rml/Test/Comp/CallButton\" loc-label=\"Inner\" label=\"$title\"/>\n</component>"))));
		const FString BoxPath = MakeRunPath(TEXT("Box.rml"));
		ExtraTablePackages.Add(MakeRunPath(TEXT("Loc/ST_Box")));
		const FArcRmlStringSyncReport Report = SyncReport(BoxPath);
		ASSERT_THAT(AreEqual(1, Report.Files.Num()));
		const FArcRmlStringSyncFile& File = Report.Files[0];
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(IsTrue(File.Errors.IsEmpty(), Described));
		ASSERT_THAT(AreEqual(MakeRunPath(TEXT("Loc/ST_Box.ST_Box")), File.TableObjectPath));
		ASSERT_THAT(IsTrue(File.bTableCreated));
		ASSERT_THAT(AreEqual(2, File.Added.Num()));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Added, TEXT("Own"), TEXT("Own text"))));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Added, TEXT("Inner"), TEXT("$title"))));
		UStringTable* Table = FindObject<UStringTable>(nullptr, *File.TableObjectPath);
		ASSERT_THAT(IsNotNull(Table));
		ASSERT_THAT(AreEqual(BoxPath, Table->GetStringTable()->GetNamespace()));
	}

	TEST_METHOD(BrokenImportedComponent_ForwardsItsErrorAndSyncsNothing)
	{
		ASSERT_THAT(IsTrue(WriteRunFile(TEXT("Panel.rml"), TEXT("<component>\n<prop name=\"title\"/>\n<div>{{ $title }}</div>\n</component>"))));
		const FString PanelImport = FString::Printf(TEXT("<dc-import name=\"%s\" loc-title=\"Panel.Title\" title=\"Panel title\"/>"), *MakeRunPath(TEXT("Panel")));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(PanelImport))));
		const FArcRmlStringSyncFile First = SyncPlain();
		ASSERT_THAT(IsTrue(First.Errors.IsEmpty(), ArcRmlStringSyncTestsPrivate::DescribeErrors(First)));
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(First.Added, TEXT("Panel.Title"), TEXT("Panel title"))));

		ASSERT_THAT(IsTrue(WriteRunFile(TEXT("Panel.rml"), TEXT("<component>\n<prop name=\"title\"/>\n<div class=\"open"))));
		ExpectLoggedPart(TEXT("unterminated tag"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 6, TEXT("unterminated tag")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 0, TEXT("nothing was synced")), Described));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Panel.Title")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Panel title")), Text));
	}

	TEST_METHOD(ImportWithoutComponentRoot_ReportsAtTheImportLineAndSyncsNothing)
	{
		ASSERT_THAT(IsTrue(WriteRunFile(TEXT("Broken.rml"), TEXT("<component>\n<div>never closed</div>"))));
		const FString Import = FString::Printf(TEXT("<span loc=\"Alpha\">Alpha text</span>\n<dc-import name=\"%s\"/>"), *MakeRunPath(TEXT("Broken")));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(Import))));
		ExpectLoggedPart(TEXT("missing <component> root or its closing tag"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("Broken")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("(1): missing <component> root or its closing tag")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 0, TEXT("nothing was synced")), Described));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
		ASSERT_THAT(IsNull(FindTable()));
	}

	TEST_METHOD(ImportOfUnknownComponent_ReportsAtTheImportLineAndSyncsNothing)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>\n<dc-import name=\"/ArcRmlUITest/UI/Rml/Components/Missing\"/>")))));
		ExpectLoggedPart(TEXT("unknown component '/ArcRmlUITest/UI/Rml/Components/Missing'"));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 7, TEXT("unknown component '/ArcRmlUITest/UI/Rml/Components/Missing': /ArcRmlUITest/UI/Rml/Components/Missing.rml does not exist")), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 0, TEXT("nothing was synced")), Described));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsNull(FindTable()));
	}

	TEST_METHOD(TableLoadedFromDisk_KeepsNamespaceAndKeysAndIsNotRecreated)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>")))));
		ASSERT_THAT(IsTrue(SyncPlain().bTableCreated));
		UStringTable* Created = FindTable();
		ASSERT_THAT(IsNotNull(Created));
		UPackage* Package = Created->GetPackage();
		Created->ClearFlags(RF_Public | RF_Standalone);
		ResetLoaders(Package);
		Created->MarkAsGarbage();
		Package->MarkAsGarbage();
		Created = nullptr;
		Package = nullptr;
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		ASSERT_THAT(IsNull(FindTable(), TEXT("the table is still in memory")));
		ASSERT_THAT(IsTrue(FPaths::FileExists(TableFile)));

		const FArcRmlStringSyncFile File = SyncPlain();
		ASSERT_THAT(IsTrue(File.Errors.IsEmpty(), ArcRmlStringSyncTestsPrivate::DescribeErrors(File)));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Changed.IsEmpty()));
		ASSERT_THAT(IsTrue(File.Unused.IsEmpty()));
		UStringTable* Loaded = FindTable();
		ASSERT_THAT(IsNotNull(Loaded, TEXT("the table was not loaded from disk")));
		ASSERT_THAT(AreEqual(DocumentPath, Loaded->GetStringTable()->GetNamespace()));
		FString Text;
		ASSERT_THAT(IsTrue(Loaded->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Alpha text")), Text));
	}

	TEST_METHOD(ChangedSourceText_KeepsTheDevNote)
	{
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>")))));
		SyncPlain();
		UStringTable* Table = FindTable();
		ASSERT_THAT(IsNotNull(Table));
		Table->GetMutableStringTable()->SetSourceString(FTextKey(TEXT("Alpha")), TEXT("Alpha text"), TEXT("translator note"));
		ASSERT_THAT(AreEqual(FString(TEXT("translator note")), GetNote(*Table, TEXT("Alpha"))));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha edited</span>")))));

		const FArcRmlStringSyncFile File = SyncPlain();
		ASSERT_THAT(IsTrue(ArcRmlStringSyncTestsPrivate::ContainsEntry(File.Changed, TEXT("Alpha"), TEXT("Alpha edited"))));
		FString Text;
		ASSERT_THAT(IsTrue(Table->GetStringTable()->GetSourceString(FTextKey(TEXT("Alpha")), Text)));
		ASSERT_THAT(AreEqual(FString(TEXT("Alpha edited")), Text));
		ASSERT_THAT(AreEqual(FString(TEXT("translator note")), GetNote(*Table, TEXT("Alpha"))));
	}

	TEST_METHOD(PropDefaultWithBinding_GivesNoSourceText)
	{
		ASSERT_THAT(IsTrue(WriteRunFile(TEXT("BoundBox.rml"), TEXT("<component>\n<prop name=\"label\" default=\"{{ x }}\"/>\n<div>{{ $label }}</div>\n</component>"))));
		const FString Import = FString::Printf(TEXT("<dc-import name=\"%s\" loc-label=\"FromBinding\"/>"), *MakeRunPath(TEXT("BoundBox")));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(Import))));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(1, File.Errors.Num(), Described));
		ASSERT_THAT(IsNotNull(ArcRmlStringSyncTestsPrivate::FindErrorAt(File, 6, TEXT("'FromBinding' has no source text")), Described));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsNull(FindTable()));
	}

	TEST_METHOD(PackageHoldingAnotherAssetAtTheTablePath_IsReportedAsNotAStringTable)
	{
		UPackage* Package = CreatePackage(*TablePackageName);
		UCurveFloat* Curve = NewObject<UCurveFloat>(Package, FName(*FPackageName::GetShortName(TablePackageName)), RF_Public | RF_Standalone);
		ASSERT_THAT(IsNotNull(Curve));
		ASSERT_THAT(IsTrue(WriteRaw(ArcRmlStringSyncTestsPrivate::MakeCleanDocumentText(TEXT("<span loc=\"Alpha\">Alpha text</span>")))));
		const FArcRmlStringSyncFile File = SyncPlain();
		const FString Described = ArcRmlStringSyncTestsPrivate::DescribeErrors(File);
		ASSERT_THAT(AreEqual(1, File.Errors.Num(), Described));
		ASSERT_THAT(AreEqual(0, File.Errors[0].Line, Described));
		ASSERT_THAT(IsTrue(File.Errors[0].Message.Contains(TEXT("CurveFloat")) && File.Errors[0].Message.Contains(TEXT("not a String Table")), Described));
		ASSERT_THAT(IsFalse(File.bTableCreated));
		ASSERT_THAT(IsTrue(File.Added.IsEmpty()));
		ASSERT_THAT(IsNull(FindTable()));
	}
};
