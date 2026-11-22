// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Localization/ArcRmlStringSync.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

namespace ArcRmlFormatTestsPrivate
{
	const TCHAR* const DocumentName = TEXT("FmtTest");
	const TCHAR* const PreviewJson = TEXT("{\n\t\"Dummy\": 0,\n\t\"Txt\": \"abc\",\n\t\"NegZero\": -0.0,\n\t\"NegOne\": -1,\n\t\"Big\": 1234567,\n\t\"Half\": 1234.5,\n\t\"C1\": 1,\n\t\"C2\": 2,\n\t\"C3\": 3,\n\t\"C5\": 5,\n\t\"C22\": 22,\n\t\"C15\": 1.5,\n\t\"C1000\": 1000,\n\t\"Queue\": [\n\t\t{ \"Progress\": 0.456, \"RemainingSeconds\": 61, \"bIsActive\": true },\n\t\t{ \"Progress\": 0, \"RemainingSeconds\": 3661, \"bIsActive\": false }\n\t]\n}\n");
	const int32 FirstBodyLine = 9;

	const TCHAR* const ItemsLeftEntry = TEXT("{0}|plural(one={0} item left,other={0} items left)");
	const TCHAR* const ItemsPlEntry = TEXT("{0}|plural(one={0} przedmiot,few={0} przedmioty,many={0} przedmiotów,other={0} przedmiotu)");
	const TCHAR* const QuotedEntry = TEXT("{0}|plural(one=You've {0} item,other=\"You've {0} items, total\")");
	const TCHAR* const LeetEntry = TEXT("{0}|plural(one={0} item,other={0} items)");
	const TCHAR* const NoOneEntry = TEXT("{0}|plural(other={0} things)");
	const TCHAR* const UnquotedParenEntry = TEXT("{0}|plural(one={0} item (x),other={0} items)");
	const TCHAR* const MaxEntry = TEXT("{0}|plural(one={0} item,other=\"{0} items (max)\")");
	const TCHAR* const MalformedEntry = TEXT("just text");

	struct FCase
	{
		const TCHAR* Id;
		const TCHAR* Expected;
	};

	FString MakeDocument(const TArray<FString>& BodyLines)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("<rml>"));
		Lines.Add(TEXT("<head>"));
		Lines.Add(TEXT("<title>Format Test</title>"));
		Lines.Add(TEXT("<meta name=\"layer\" content=\"UI.Layer.Menu\"/>"));
		Lines.Add(TEXT("<meta name=\"input-mode\" content=\"menu\"/>"));
		Lines.Add(TEXT("<viewmodel name=\"fm\" preview=\"Preview/Fmt.json\"/>"));
		Lines.Add(TEXT("</head>"));
		Lines.Add(TEXT("<body data-model=\"fm\">"));
		Lines.Append(BodyLines);
		Lines.Add(TEXT("</body>"));
		Lines.Add(TEXT("</rml>"));
		return FString::Join(Lines, TEXT("\n"));
	}

	FString MakeSpan(const TCHAR* Id, const TCHAR* Expression)
	{
		return FString::Printf(TEXT("<span id=\"%s\">{{ %s }}</span>"), Id, Expression);
	}

	const TCHAR* const StubLine = TEXT("<span id=\"dummy\" loc=\"Dummy\">Dummy</span>");

	// Spans whose expression is a constant, so the expected text depends only on the transform and the culture.
	TArray<FString> MakeTransformBody()
	{
		TArray<FString> Lines;
		Lines.Add(StubLine);
		Lines.Add(MakeSpan(TEXT("n0"), TEXT("0 | arcnum")));
		Lines.Add(MakeSpan(TEXT("n1"), TEXT("(0 - 5) | arcnum")));
		Lines.Add(MakeSpan(TEXT("n2"), TEXT("1234 | arcnum")));
		Lines.Add(MakeSpan(TEXT("n3"), TEXT("Big | arcnum")));
		Lines.Add(MakeSpan(TEXT("n4"), TEXT("1234.5 | arcnum")));
		Lines.Add(MakeSpan(TEXT("n5"), TEXT("1235.5 | arcnum")));
		Lines.Add(MakeSpan(TEXT("n6"), TEXT("Half | arcnum(1)")));
		Lines.Add(MakeSpan(TEXT("n7"), TEXT("1234 | arcnum(1)")));
		Lines.Add(MakeSpan(TEXT("p0"), TEXT("0 | arcpct")));
		Lines.Add(MakeSpan(TEXT("p1"), TEXT("0.005 | arcpct")));
		Lines.Add(MakeSpan(TEXT("p2"), TEXT("0.015 | arcpct")));
		Lines.Add(MakeSpan(TEXT("p3"), TEXT("0.025 | arcpct")));
		Lines.Add(MakeSpan(TEXT("p4"), TEXT("0.456 | arcpct")));
		Lines.Add(MakeSpan(TEXT("p5"), TEXT("0.456 | arcpct(1)")));
		Lines.Add(MakeSpan(TEXT("p6"), TEXT("1 | arcpct")));
		Lines.Add(MakeSpan(TEXT("t0"), TEXT("0 | arctime")));
		Lines.Add(MakeSpan(TEXT("t1"), TEXT("59 | arctime")));
		Lines.Add(MakeSpan(TEXT("t2"), TEXT("59.2 | arctime")));
		Lines.Add(MakeSpan(TEXT("t3"), TEXT("61 | arctime")));
		Lines.Add(MakeSpan(TEXT("t4"), TEXT("3599 | arctime")));
		Lines.Add(MakeSpan(TEXT("t5"), TEXT("3600 | arctime")));
		Lines.Add(MakeSpan(TEXT("t6"), TEXT("3661 | arctime")));
		Lines.Add(MakeSpan(TEXT("t7"), TEXT("36000 | arctime")));
		Lines.Add(MakeSpan(TEXT("t8"), TEXT("(0 - 5) | arctime")));
		return Lines;
	}

	TArray<FString> MakePluralBody()
	{
		TArray<FString> Lines;
		Lines.Add(StubLine);
		Lines.Add(MakeSpan(TEXT("e1"), TEXT("C1 | arcplural('ItemsLeft')")));
		Lines.Add(MakeSpan(TEXT("e2"), TEXT("C2 | arcplural('ItemsLeft')")));
		Lines.Add(MakeSpan(TEXT("e3"), TEXT("C1000 | arcplural('ItemsLeft')")));
		Lines.Add(MakeSpan(TEXT("eq"), TEXT("C3 | arcplural('Quoted')")));
		Lines.Add(MakeSpan(TEXT("zm"), TEXT("NegZero | arcplural('ItemsLeft')")));
		Lines.Add(MakeSpan(TEXT("zo"), TEXT("NegOne | arcplural('ItemsLeft')")));
		Lines.Add(MakeSpan(TEXT("pm"), TEXT("C3 | arcplural('Max')")));
		Lines.Add(MakeSpan(TEXT("l1"), TEXT("C1 | arcplural('ItemsPl')")));
		Lines.Add(MakeSpan(TEXT("l2"), TEXT("C2 | arcplural('ItemsPl')")));
		Lines.Add(MakeSpan(TEXT("l5"), TEXT("C5 | arcplural('ItemsPl')")));
		Lines.Add(MakeSpan(TEXT("l22"), TEXT("C22 | arcplural('ItemsPl')")));
		Lines.Add(MakeSpan(TEXT("l15"), TEXT("C15 | arcplural('ItemsPl')")));
		Lines.Add(MakeSpan(TEXT("leet"), TEXT("C5 | arcplural('Leet')")));
		return Lines;
	}

	const FCase EnglishCases[] = {
		{TEXT("n0"), TEXT("0")},
		{TEXT("n1"), TEXT("-5")},
		{TEXT("n2"), TEXT("1,234")},
		{TEXT("n3"), TEXT("1,234,567")},
		{TEXT("n4"), TEXT("1,234")},
		{TEXT("n5"), TEXT("1,236")},
		{TEXT("n6"), TEXT("1,234.5")},
		{TEXT("n7"), TEXT("1,234.0")},
		{TEXT("p0"), TEXT("0%")},
		{TEXT("p1"), TEXT("0%")},
		{TEXT("p2"), TEXT("2%")},
		{TEXT("p3"), TEXT("2%")},
		{TEXT("p4"), TEXT("46%")},
		{TEXT("p5"), TEXT("45.6%")},
		{TEXT("p6"), TEXT("100%")},
		{TEXT("t0"), TEXT("00:00")},
		{TEXT("t1"), TEXT("00:59")},
		{TEXT("t2"), TEXT("01:00")},
		{TEXT("t3"), TEXT("01:01")},
		{TEXT("t4"), TEXT("59:59")},
		{TEXT("t5"), TEXT("1:00:00")},
		{TEXT("t6"), TEXT("1:01:01")},
		{TEXT("t7"), TEXT("10:00:00")},
		{TEXT("t8"), TEXT("00:00")},
	};

	const FCase EnglishPluralCases[] = {
		{TEXT("e1"), TEXT("1 item left")},
		{TEXT("e2"), TEXT("2 items left")},
		{TEXT("e3"), TEXT("1,000 items left")},
		{TEXT("eq"), TEXT("You've 3 items, total")},
		{TEXT("zm"), TEXT("-0 items left")},
		{TEXT("zo"), TEXT("-1 item left")},
		{TEXT("pm"), TEXT("3 items (max)")},
	};

	const FCase PolishCases[] = {
		{TEXT("l1"), TEXT("1 przedmiot")},
		{TEXT("l2"), TEXT("2 przedmioty")},
		{TEXT("l5"), TEXT("5 przedmiotów")},
		{TEXT("l22"), TEXT("22 przedmioty")},
		{TEXT("l15"), TEXT("1,5 przedmiotu")},
	};

	FString DescribeErrors(const FArcRmlStringSyncFile& File)
	{
		TArray<FString> Lines;
		for (const FArcRmlStringSyncError& Error : File.Errors)
		{
			Lines.Add(FString::Printf(TEXT("%d: %s"), Error.Line, *Error.Message));
		}
		return FString::Printf(TEXT("%d errors [%s]"), Lines.Num(), *FString::Join(Lines, TEXT(" | ")));
	}

	bool HasErrorAt(const FArcRmlStringSyncFile& File, int32 Line, const TCHAR* MessagePart)
	{
		for (const FArcRmlStringSyncError& Error : File.Errors)
		{
			if (Error.Line == Line && Error.Message.Contains(MessagePart))
			{
				return true;
			}
		}
		return false;
	}
}

TEST_CLASS(ArcRmlFormatTests, "ArcRmlUI.Format")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	FString RunFolder;
	FString RunDir;
	FString DocumentFile;
	FString DocumentPath;
	FString TableObjectPath;
	FString TablePackageName;
	FString TableFile;
	FString SetupError;
	FString PreviousCulture;
	bool bCultureChanged = false;
	FString PathBefore;
	FString LastRead;

	AFTER_EACH()
	{
		if (!LastRead.IsEmpty())
		{
			TestRunner->AddInfo(FString::Printf(TEXT("last polled text: '%s'"), *LastRead));
			LastRead.Reset();
		}
		View.Close();
		if (bCultureChanged)
		{
			FInternationalization::Get().SetCurrentCulture(PreviousCulture);
			bCultureChanged = false;
		}
		if (RunDir.IsEmpty())
		{
			return;
		}
		IAssetRegistry& Registry = IAssetRegistry::GetChecked();
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
		TArray<FString> Files;
		Files.Add(TableFile);
		Registry.ScanModifiedAssetFiles(Files);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RunDir), false, false);
		RunDir.Reset();
	}

	bool WriteFile(const FString& Name, const FString& Text)
	{
		return FFileHelper::SaveStringToFile(Text, *(RunDir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	FArcRmlStringSyncFile SyncDocument()
	{
		TArray<FString> Paths;
		Paths.Add(DocumentPath);
		const FArcRmlStringSyncReport Report = ArcRmlStringSync::Sync(FArcRmlUIModule::Get().GetContentRoots(), Paths);
		return Report.Files.Num() == 1 ? Report.Files[0] : FArcRmlStringSyncFile();
	}

	// Writes the run folder with a loc-only stub document, lets Sync create the real String Table asset for it,
	// registers the asset and sets the plural entries, then writes the document under test.
	bool PrepareFixture(const TArray<FString>& BodyLines, const TMap<FString, FString>& Entries)
	{
		const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
		if (ContentDir.IsEmpty())
		{
			SetupError = TEXT("the test plugin content directory is unknown");
			return false;
		}
		RunFolder = FString::Printf(TEXT("FormatRuntime/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		RunDir = ContentDir / RunFolder;
		const FString DocumentFileName = FString(ArcRmlFormatTestsPrivate::DocumentName) + TEXT(".rml");
		DocumentFile = RunDir / DocumentFileName;
		DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/") + DocumentFileName);
		const FString TableName = FString(TEXT("ST_")) + ArcRmlFormatTestsPrivate::DocumentName;
		TablePackageName = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/") + TableName);
		TableObjectPath = TablePackageName + TEXT(".") + TableName;
		TableFile = RunDir / (FString(TEXT("Loc/")) + TableName + TEXT(".uasset"));

		TArray<FString> StubBody;
		StubBody.Add(ArcRmlFormatTestsPrivate::StubLine);
		if (!WriteFile(DocumentFileName, ArcRmlFormatTestsPrivate::MakeDocument(StubBody)) || !WriteFile(TEXT("Preview/Fmt.json"), ArcRmlFormatTestsPrivate::PreviewJson))
		{
			SetupError = TEXT("the fixture files could not be written");
			return false;
		}
		const FArcRmlStringSyncFile Stub = SyncDocument();
		if (!Stub.Errors.IsEmpty() || !Stub.bTableCreated)
		{
			SetupError = FString::Printf(TEXT("the stub sync did not create the table: %s"), *ArcRmlFormatTestsPrivate::DescribeErrors(Stub));
			return false;
		}
		const FStringTablePtr Table = FStringTableRegistry::Get().FindMutableStringTable(FName(*TableObjectPath));
		if (!Table.IsValid())
		{
			SetupError = FString::Printf(TEXT("the string table '%s' is not registered"), *TableObjectPath);
			return false;
		}
		for (const TPair<FString, FString>& Entry : Entries)
		{
			Table->SetSourceString(FTextKey(Entry.Key), Entry.Value, FString());
		}
		if (!WriteFile(DocumentFileName, ArcRmlFormatTestsPrivate::MakeDocument(BodyLines)))
		{
			SetupError = TEXT("the fixture document could not be written");
			return false;
		}
		return true;
	}

	TMap<FString, FString> MakeStandardEntries() const
	{
		TMap<FString, FString> Entries;
		Entries.Add(TEXT("ItemsLeft"), ArcRmlFormatTestsPrivate::ItemsLeftEntry);
		Entries.Add(TEXT("ItemsPl"), ArcRmlFormatTestsPrivate::ItemsPlEntry);
		Entries.Add(TEXT("Quoted"), ArcRmlFormatTestsPrivate::QuotedEntry);
		Entries.Add(TEXT("Leet"), ArcRmlFormatTestsPrivate::LeetEntry);
		Entries.Add(TEXT("Max"), ArcRmlFormatTestsPrivate::MaxEntry);
		return Entries;
	}

	bool IsTableRegistered() const
	{
		FAssetData Data;
		return IAssetRegistry::GetChecked().TryGetAssetByObjectPath(FSoftObjectPath(TableObjectPath), Data) == UE::AssetRegistry::EExists::Exists;
	}

	bool OpenAndRead()
	{
		return ArcRmlUITestHelpers::OpenView(View, DocumentPath) && View.Flush() && View.ParseLayout(Elements) && View.GetProbe().bHasDocument;
	}

	bool ReadText(const TCHAR* Id, const FString& Expected)
	{
		if (!View.Flush() || !View.ParseLayout(Elements))
		{
			return false;
		}
		LastRead = ArcRmlUITestHelpers::GetText(Elements, Id);
		return LastRead == Expected;
	}

	void SwitchCulture(const TCHAR* Name)
	{
		if (!bCultureChanged)
		{
			PreviousCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
			bCultureChanged = true;
		}
		FInternationalization::Get().SetCurrentCulture(Name);
	}

	FString CheckCases(TConstArrayView<ArcRmlFormatTestsPrivate::FCase> Cases)
	{
		FString Wrong;
		for (const ArcRmlFormatTestsPrivate::FCase& Case : Cases)
		{
			const FString Actual = ArcRmlUITestHelpers::GetText(Elements, Case.Id);
			if (Actual != Case.Expected)
			{
				Wrong += FString::Printf(TEXT("[%s: '%s' != '%s'] "), Case.Id, *Actual, Case.Expected);
			}
		}
		return Wrong;
	}

	TEST_METHOD(English_TransformsShowTheExactStrings)
	{
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(ArcRmlFormatTestsPrivate::MakeTransformBody(), MakeStandardEntries()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the open view formats with en"), [this]() -> bool
			{
				return ReadText(TEXT("n3"), TEXT("1,234,567"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("every transform shows its exact string"), [this]()
			{
				ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
				const FString Wrong = CheckCases(ArcRmlFormatTestsPrivate::EnglishCases);
				ASSERT_THAT(IsTrue(Wrong.IsEmpty(), Wrong));
			});
	}

	TEST_METHOD(English_PluralShowsTheExactStrings)
	{
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(ArcRmlFormatTestsPrivate::MakePluralBody(), MakeStandardEntries()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the open view formats with en"), [this]() -> bool
			{
				return ReadText(TEXT("e3"), TEXT("1,000 items left"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("every plural shows its exact string"), [this]()
			{
				ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
				const FString Wrong = CheckCases(ArcRmlFormatTestsPrivate::EnglishPluralCases);
				ASSERT_THAT(IsTrue(Wrong.IsEmpty(), Wrong));
			});
	}

	TEST_METHOD(French_RefreshesTheOpenViewInPlaceWithNarrowNoBreakSpaces)
	{
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(ArcRmlFormatTestsPrivate::MakeTransformBody(), MakeStandardEntries()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the open view formats with en"), [this]() -> bool
			{
				return ReadText(TEXT("n3"), TEXT("1,234,567"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("remember the element and switch to fr"), [this]()
			{
				const FArcRmlTestLayoutElement* Number = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("n3"));
				ASSERT_THAT(IsNotNull(Number));
				PathBefore = Number->Path;
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("fr")), TEXT("the fr culture could not be set")));
				bCultureChanged = true;
			})
			.Until(TEXT("the open view shows fr numbers"), [this]() -> bool
			{
				return ReadText(TEXT("n3"), TEXT("1 234 567"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the other numbers follow fr on the same element"), [this]()
			{
				ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
				ASSERT_THAT(AreEqual(FString(TEXT("1 234,5")), ArcRmlUITestHelpers::GetText(Elements, TEXT("n6"))));
				const FArcRmlTestLayoutElement* Number = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("n3"));
				ASSERT_THAT(IsNotNull(Number));
				ASSERT_THAT(AreEqual(PathBefore, Number->Path, TEXT("the element was rebuilt instead of updated in place")));
				ASSERT_THAT(AreEqual(FString(TEXT("01:01")), ArcRmlUITestHelpers::GetText(Elements, TEXT("t3"))));
				ASSERT_THAT(AreEqual(FString(TEXT("1:01:01")), ArcRmlUITestHelpers::GetText(Elements, TEXT("t6"))));
			})
			.Then(TEXT("switch back to en"), [this]()
			{
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("en"))));
			})
			.Until(TEXT("the open view shows en numbers again"), [this]() -> bool
			{
				return ReadText(TEXT("n3"), TEXT("1,234,567")) && ReadText(TEXT("n6"), TEXT("1,234.5"));
			}, FTimespan::FromSeconds(30));
	}

	TEST_METHOD(Polish_PluralFormsFollowTheCultureAfterTheRefresh)
	{
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(ArcRmlFormatTestsPrivate::MakePluralBody(), MakeStandardEntries()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the open view formats with en"), [this]() -> bool
			{
				return ReadText(TEXT("l2"), TEXT("2 przedmiotu"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("English picks one and other for the Polish entry, then switch to pl"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(TEXT("1 przedmiot")), ArcRmlUITestHelpers::GetText(Elements, TEXT("l1"))));
				ASSERT_THAT(AreEqual(FString(TEXT("5 przedmiotu")), ArcRmlUITestHelpers::GetText(Elements, TEXT("l5"))));
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("pl")), TEXT("the pl culture could not be set")));
				bCultureChanged = true;
			})
			.Until(TEXT("the open view shows the pl few form"), [this]() -> bool
			{
				return ReadText(TEXT("l2"), TEXT("2 przedmioty"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("every Polish plural case shows its exact string"), [this]()
			{
				ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements)));
				const FString Wrong = CheckCases(ArcRmlFormatTestsPrivate::PolishCases);
				ASSERT_THAT(IsTrue(Wrong.IsEmpty(), Wrong));
			});
	}

#if ENABLE_LOC_TESTING
	TEST_METHOD(Leet_PluralLeetifiesTheChosenFormBeforeTheNumberIsInserted)
	{
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(ArcRmlFormatTestsPrivate::MakePluralBody(), MakeStandardEntries()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the open view formats with en"), [this]() -> bool
			{
				return ReadText(TEXT("leet"), TEXT("5 items"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("switch to LEET"), [this]()
			{
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("LEET")), TEXT("the LEET culture could not be set")));
				bCultureChanged = true;
			})
			.Until(TEXT("the plural shows the leetified form"), [this]() -> bool
			{
				return View.Flush() && View.ParseLayout(Elements) && ArcRmlUITestHelpers::GetText(Elements, TEXT("leet")).StartsWith(TEXT("‡"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the string is what UE shows for a leetified format argument"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(TEXT("‡«5» !73m$‡")), ArcRmlUITestHelpers::GetText(Elements, TEXT("leet"))));
			});
	}
#endif

	TEST_METHOD(BadEntries_AreEachReportedOnceAndShowTheKey)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("missing"), TEXT("C2 | arcplural('NoSuchKey')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("malformed"), TEXT("C2 | arcplural('Malformed')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("noone"), TEXT("C1 | arcplural('NoOne')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("textarg"), TEXT("Txt | arcplural('NoOne')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("paren"), TEXT("C3 | arcplural('Paren')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("txt"), TEXT("Txt")));
		TMap<FString, FString> Entries;
		Entries.Add(TEXT("Paren"), ArcRmlFormatTestsPrivate::UnquotedParenEntry);
		Entries.Add(TEXT("Malformed"), ArcRmlFormatTestsPrivate::MalformedEntry);
		Entries.Add(TEXT("NoOne"), ArcRmlFormatTestsPrivate::NoOneEntry);

		TestCommandBuilder
			.Then(TEXT("write the fixture"), [this, Body, Entries]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(Body, Entries), SetupError));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view and read the shown texts"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));
				ASSERT_THAT(AreEqual(FString(TEXT("NoSuchKey")), ArcRmlUITestHelpers::GetText(Elements, TEXT("missing"))));
				ASSERT_THAT(AreEqual(FString(TEXT("Malformed")), ArcRmlUITestHelpers::GetText(Elements, TEXT("malformed"))));
				ASSERT_THAT(AreEqual(FString(TEXT("NoOne")), ArcRmlUITestHelpers::GetText(Elements, TEXT("noone"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("textarg"))));
				ASSERT_THAT(AreEqual(FString(TEXT("Paren")), ArcRmlUITestHelpers::GetText(Elements, TEXT("paren"))));
				ASSERT_THAT(AreEqual(FString(TEXT("abc")), ArcRmlUITestHelpers::GetText(Elements, TEXT("txt"))));
				View.SetModelValue(TEXT("fm"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Txt")), ArcRmlUITestHelpers::MakeString(TEXT("def")));
			})
			.Until(TEXT("the open view re-evaluates every span"), [this]() -> bool
			{
				return ReadText(TEXT("txt"), TEXT("def"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the problems still show the key or nothing after the re-evaluation"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(TEXT("NoSuchKey")), ArcRmlUITestHelpers::GetText(Elements, TEXT("missing"))));
				ASSERT_THAT(AreEqual(FString(TEXT("Malformed")), ArcRmlUITestHelpers::GetText(Elements, TEXT("malformed"))));
				ASSERT_THAT(AreEqual(FString(TEXT("NoOne")), ArcRmlUITestHelpers::GetText(Elements, TEXT("noone"))));
				ASSERT_THAT(AreEqual(FString(TEXT("Paren")), ArcRmlUITestHelpers::GetText(Elements, TEXT("paren"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("textarg"))));
			});
	}

	TEST_METHOD(NonNumberArguments_GiveEmptyTextAndWarnOnce)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("num"), TEXT("Txt | arcnum")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("pct"), TEXT("Txt | arcpct")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("time"), TEXT("Txt | arctime")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("digits"), TEXT("C5 | arcnum(Txt)")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("txt"), TEXT("Txt")));
		TestCommandBuilder
			.Then(TEXT("write the fixture"), [this, Body]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(Body, TMap<FString, FString>()), SetupError));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view and read the shown texts"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("num"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("pct"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("time"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("digits"))));
				View.SetModelValue(TEXT("fm"), ArcRmlUITestHelpers::MakeFieldPath(TEXT("Txt")), ArcRmlUITestHelpers::MakeString(TEXT("def")));
			})
			.Until(TEXT("the open view re-evaluates every span"), [this]() -> bool
			{
				return ReadText(TEXT("txt"), TEXT("def"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the spans stay empty after the re-evaluation"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("num"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("pct"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("time"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("digits"))));
			});
	}

	TEST_METHOD(BadKey_IsACompileErrorAtItsLine)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("bad"), TEXT("2 | arcplural('bad key')")));
		TestCommandBuilder.Then(TEXT("open the document with a key outside the character set"), [this, Body]()
		{
			ASSERT_THAT(IsTrue(PrepareFixture(Body, TMap<FString, FString>()), SetupError));
			const int32 Line = ArcRmlFormatTestsPrivate::FirstBodyLine + 1;
			ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, DocumentPath);
			ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::OpenView(View, DocumentPath), TEXT("a document with a bad arcplural key compiled and opened")));
			ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("arcplural key 'bad key' must be made of"), DocumentPath, Line), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		});
	}

	TEST_METHOD(Sync_ReportsAMissingAndAMalformedPluralKey)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("a"), TEXT("2 | arcplural('Absent')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("b"), TEXT("2 | arcplural('Broken')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("c"), TEXT("C2 | arcplural('ItemsLeft')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("d"), TEXT("C2 | arcplural('Paren')")));
		TMap<FString, FString> Entries;
		Entries.Add(TEXT("Paren"), ArcRmlFormatTestsPrivate::UnquotedParenEntry);
		Entries.Add(TEXT("Broken"), ArcRmlFormatTestsPrivate::MalformedEntry);
		Entries.Add(TEXT("ItemsLeft"), ArcRmlFormatTestsPrivate::ItemsLeftEntry);

		TestCommandBuilder.Then(TEXT("sync the document and read the errors"), [this, Body, Entries]()
		{
			ASSERT_THAT(IsTrue(PrepareFixture(Body, Entries), SetupError));
			const FArcRmlStringSyncFile File = SyncDocument();
			const FString Described = ArcRmlFormatTestsPrivate::DescribeErrors(File);
			ASSERT_THAT(AreEqual(3, File.Errors.Num(), Described));
			ASSERT_THAT(IsTrue(ArcRmlFormatTestsPrivate::HasErrorAt(File, ArcRmlFormatTestsPrivate::FirstBodyLine + 4, TEXT("arcplural entry 'Paren'")), Described));
			ASSERT_THAT(IsTrue(ArcRmlFormatTestsPrivate::HasErrorAt(File, ArcRmlFormatTestsPrivate::FirstBodyLine + 1, TEXT("arcplural key 'Absent' has no entry")), Described));
			ASSERT_THAT(IsTrue(ArcRmlFormatTestsPrivate::HasErrorAt(File, ArcRmlFormatTestsPrivate::FirstBodyLine + 2, TEXT("arcplural entry 'Broken'")), Described));
			ASSERT_THAT(IsTrue(File.Added.IsEmpty(), TEXT("Sync wrote an entry for a plural key")));
			FString Text;
			const FStringTablePtr Table = FStringTableRegistry::Get().FindMutableStringTable(FName(*TableObjectPath));
			ASSERT_THAT(IsTrue(Table.IsValid()));
			ASSERT_THAT(IsFalse(Table->GetSourceString(FTextKey(TEXT("Absent")), Text), TEXT("Sync wrote the missing plural entry")));
		});
	}

	TEST_METHOD(Sync_AcceptsWellFormedPluralEntries)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("a"), TEXT("C2 | arcplural('ItemsLeft')")));
		Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("b"), TEXT("C3 | arcplural('Quoted')")));
		TestCommandBuilder.Then(TEXT("sync a document whose plural entries parse"), [this, Body]()
		{
			ASSERT_THAT(IsTrue(PrepareFixture(Body, MakeStandardEntries()), SetupError));
			const FArcRmlStringSyncFile File = SyncDocument();
			ASSERT_THAT(IsTrue(File.Errors.IsEmpty(), ArcRmlFormatTestsPrivate::DescribeErrors(File)));
			ASSERT_THAT(IsTrue(File.Added.IsEmpty() && File.Changed.IsEmpty()));
		});
	}

	TEST_METHOD(CraftQueueBinding_ShowsMmSsAndThePercentLabel)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(TEXT("<div data-for=\"q, i : Queue\">"));
		Body.Add(TEXT("<span data-attr-id=\"'q-sub-' + i\">{{ q.bIsActive ? 'Crafting' : (q.RemainingSeconds | arctime) + ' Queued' }}</span>"));
		Body.Add(TEXT("<span data-attr-id=\"'q-label-' + i\">{{ q.bIsActive ? (q.Progress | arcpct) : '' }}</span>"));
		Body.Add(TEXT("<span data-attr-id=\"'q-right-' + i\">{{ (q.RemainingSeconds | arctime) + ' left' }}</span>"));
		Body.Add(TEXT("</div>"));
		TestCommandBuilder
			.Then(TEXT("write the fixture and switch to en"), [this, Body]()
			{
				ASSERT_THAT(IsTrue(PrepareFixture(Body, TMap<FString, FString>()), SetupError));
				SwitchCulture(TEXT("en"));
			})
			.Until(TEXT("the asset registry knows the string table"), [this]() -> bool
			{
				return IsTableRegistered();
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view"), [this]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			})
			.Until(TEXT("the queue rows show"), [this]() -> bool
			{
				return ReadText(TEXT("q-right-0"), TEXT("01:01 left"));
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the active row shows mm:ss and the percent label, the queued row hours"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(TEXT("Crafting")), ArcRmlUITestHelpers::GetText(Elements, TEXT("q-sub-0"))));
				ASSERT_THAT(AreEqual(FString(TEXT("46%")), ArcRmlUITestHelpers::GetText(Elements, TEXT("q-label-0"))));
				ASSERT_THAT(AreEqual(FString(TEXT("1:01:01 Queued")), ArcRmlUITestHelpers::GetText(Elements, TEXT("q-sub-1"))));
				ASSERT_THAT(AreEqual(FString(TEXT("1:01:01 left")), ArcRmlUITestHelpers::GetText(Elements, TEXT("q-right-1"))));
				ASSERT_THAT(AreEqual(FString(), ArcRmlUITestHelpers::GetText(Elements, TEXT("q-label-1"))));
			});
	}

	FString MakeBoxPath() const
	{
		return ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Box"));
	}

	// A bad key inside a branch the expansion does not reach is not an error and no use; the same markup live is an error at its line.
	TEST_METHOD(DeadDataIf_ReportsNothingAndLiveDataIfReportsTheBadKey)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		Body.Add(TEXT("<div data-if=\"false\"><span>{{ C2 | arcplural('bad key') }}</span></div>"));
		Body.Add(TEXT("<div data-if=\"false\"><span>{{ C2 | arcplural('Dead') }}</span></div>"));
		TestCommandBuilder.Then(TEXT("sync the dead branches, then the live one"), [this, Body]()
		{
			ASSERT_THAT(IsTrue(PrepareFixture(Body, TMap<FString, FString>()), SetupError));
			const FArcRmlStringSyncFile Dead = SyncDocument();
			ASSERT_THAT(IsTrue(Dead.Errors.IsEmpty(), ArcRmlFormatTestsPrivate::DescribeErrors(Dead)));

			TArray<FString> Live = Body;
			Live[1] = TEXT("<div data-if=\"true\"><span>{{ C2 | arcplural('bad key') }}</span></div>");
			ASSERT_THAT(IsTrue(WriteFile(ArcRmlFormatTestsPrivate::DocumentName + FString(TEXT(".rml")), ArcRmlFormatTestsPrivate::MakeDocument(Live))));
			ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, DocumentPath);
			const FArcRmlStringSyncFile Report = SyncDocument();
			ASSERT_THAT(IsTrue(ArcRmlFormatTestsPrivate::HasErrorAt(Report, ArcRmlFormatTestsPrivate::FirstBodyLine + 1, TEXT("arcplural key 'bad key' must be made of")), ArcRmlFormatTestsPrivate::DescribeErrors(Report)));
		});
	}

	TEST_METHOD(DeadImportAndLiveImport_EachReportTheBadKeyInTheComponent)
	{
		TArray<FString> Body;
		Body.Add(ArcRmlFormatTestsPrivate::StubLine);
		TestCommandBuilder.Then(TEXT("sync a dead import of a component with a bad key, then the live import"), [this, Body]()
		{
			ASSERT_THAT(IsTrue(PrepareFixture(Body, TMap<FString, FString>()), SetupError));
			ASSERT_THAT(IsTrue(WriteFile(TEXT("Box.rml"), TEXT("<component>\n<span>{{ C2 | arcplural('bad key') }}</span>\n</component>"))));
			TestRunner->AddExpectedErrorPlain(FString(TEXT("LogArcRmlUI: ")) + ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Box.rml")) + TEXT("(2): arcplural key 'bad key' must be made of"),
				EAutomationExpectedErrorFlags::Contains, 2);
			const FString Name = FString(ArcRmlFormatTestsPrivate::DocumentName) + TEXT(".rml");
			TArray<FString> Dead = Body;
			Dead.Add(FString::Printf(TEXT("<dc-import name=\"%s\" data-if=\"false\"/>"), *MakeBoxPath()));
			ASSERT_THAT(IsTrue(WriteFile(Name, ArcRmlFormatTestsPrivate::MakeDocument(Dead))));
			SyncDocument();

			TArray<FString> Live = Body;
			Live.Add(FString::Printf(TEXT("<dc-import name=\"%s\"/>"), *MakeBoxPath()));
			ASSERT_THAT(IsTrue(WriteFile(Name, ArcRmlFormatTestsPrivate::MakeDocument(Live))));
			SyncDocument();
		});
	}

	TEST_METHOD(PathWithAnApostrophe_IsACompileErrorAtTheCallLine)
	{
		TestCommandBuilder.Then(TEXT("open a document whose folder name holds an apostrophe"), [this]()
		{
			const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
			ASSERT_THAT(IsFalse(ContentDir.IsEmpty()));
			RunFolder = FString::Printf(TEXT("FormatRuntime/R%s'q"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
			RunDir = ContentDir / RunFolder;
			TablePackageName = ArcRmlUITestHelpers::MakePath(TEXT("FormatRuntime/NoTable"));
			TableFile = RunDir / TEXT("Loc/ST_FmtTest.uasset");
			DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/FmtTest.rml"));
			TArray<FString> Body;
			Body.Add(ArcRmlFormatTestsPrivate::MakeSpan(TEXT("a"), TEXT("C2 | arcplural('K')")));
			ASSERT_THAT(IsTrue(WriteFile(TEXT("FmtTest.rml"), ArcRmlFormatTestsPrivate::MakeDocument(Body)) && WriteFile(TEXT("Preview/Fmt.json"), ArcRmlFormatTestsPrivate::PreviewJson)));
			ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, DocumentPath);
			ASSERT_THAT(IsFalse(ArcRmlUITestHelpers::OpenView(View, DocumentPath), TEXT("a path an arcplural key literal cannot carry compiled and opened")));
			ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountHeadDiagnosticsAt(View, TEXT("an arcplural key literal cannot carry"), DocumentPath, ArcRmlFormatTestsPrivate::FirstBodyLine), ArcRmlUITestHelpers::DescribeHeadDiagnostics(View)));
		});
	}
};
