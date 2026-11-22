// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUITestHelpers.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Host/ArcRmlKeyLabels.h"
#include "InputCoreTypes.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace ArcRmlLocalizationTestsPrivate
{
	const TCHAR* const TableText = TEXT("Section from table");
	const TCHAR* const TitleText = TEXT("Loc Test title [ST]");
	const TCHAR* const DeferredClass = TEXT("deferred-probe");

	FString MakeTableObjectPath(const FString& Folder)
	{
		return ArcRmlUITestHelpers::MakePath(Folder + TEXT("/Loc/ST_LocTest.ST_LocTest"));
	}

	FString Squash(const FString& Text)
	{
		FString Result = Text;
		Result.ReplaceInline(TEXT("\r"), TEXT(""));
		Result.ReplaceInline(TEXT("\n"), TEXT(""));
		Result.ReplaceInline(TEXT("\t"), TEXT(""));
		Result.ReplaceInline(TEXT(" "), TEXT(""));
		return Result;
	}

	FString MakeRunFolder()
	{
		return FString::Printf(TEXT("LocRuntime/R%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	const TCHAR* DescribeState(EArcRmlStringTableState State)
	{
		switch (State)
		{
		case EArcRmlStringTableState::Loaded:
			return TEXT("Loaded");
		case EArcRmlStringTableState::Loading:
			return TEXT("Loading");
		case EArcRmlStringTableState::Failed:
			return TEXT("Failed");
		case EArcRmlStringTableState::Missing:
			return TEXT("Missing");
		}
		return TEXT("Unknown");
	}
}

TEST_CLASS(ArcRmlLocalizationTests, "ArcRmlUI.Localization")
{
	FArcRmlTestView View;
	TArray<FArcRmlTestLayoutElement> Elements;
	FString ForcedTablePath;
	FString RuntimeDir;
	FString RuntimePackageName;
	FString RuntimeTablePath;
	FString RuntimeTableFile;
	FDelegateHandle AssetAddedHandle;
	bool bTableAdded = false;
	EArcRmlStringTableState StateWhenAdded = EArcRmlStringTableState::Missing;
	bool bAcceptedWhileLoading = false;
	bool bFlushedWhileLoading = false;
	bool bWaitedWhileLoading = false;
	bool bShownWhileLoading = false;
	int32 DeferredWhileLoading = 0;
	FString EmbeddedWhileLoading;
	FString FirstShownText;
	FString PreviousCulture;
	bool bCultureChanged = false;
	FString KeyLabelBefore;
	FString SectionPathBefore;
	FString BoundPathBefore;
	FString PlainAreaPathBefore;

	AFTER_EACH()
	{
		if (bCultureChanged)
		{
			FInternationalization::Get().SetCurrentCulture(PreviousCulture);
			bCultureChanged = false;
		}
		if (AssetAddedHandle.IsValid())
		{
			IAssetRegistry::GetChecked().OnAssetAdded().Remove(AssetAddedHandle);
			AssetAddedHandle.Reset();
		}
		View.Close();
		if (!ForcedTablePath.IsEmpty())
		{
			FArcRmlTestView::ClearForcedStringTableState(ForcedTablePath);
			ForcedTablePath.Reset();
		}
		if (!RuntimeDir.IsEmpty())
		{
			IAssetRegistry& Registry = IAssetRegistry::GetChecked();
			if (UPackage* Package = FindPackage(nullptr, *RuntimePackageName))
			{
				ResetLoaders(Package);
				Registry.PackageDeleted(Package);
			}
			IFileManager::Get().DeleteDirectory(*RuntimeDir, false, true);
			TArray<FString> Files;
			Files.Add(RuntimeTableFile);
			Registry.ScanModifiedAssetFiles(Files);
			FStringTableRegistry::Get().UnregisterStringTable(FName(*RuntimeTablePath));
			IFileManager::Get().DeleteDirectory(*FPaths::GetPath(RuntimeDir), false, false);
			RuntimeDir.Reset();
			RuntimePackageName.Reset();
			RuntimeTablePath.Reset();
			RuntimeTableFile.Reset();
		}
	}

	void WaitForSettledTables()
	{
		TestCommandBuilder.Until(TEXT("ArcRmlUI string tables settled"), []() -> bool
		{
			return FArcRmlTestView::AreStringTablesSettled();
		}, FTimespan::FromSeconds(60));
	}

	bool OpenAndRead(const FString& Path)
	{
		return ArcRmlUITestHelpers::OpenView(View, Path) && View.Flush() && View.ParseLayout(Elements);
	}

	bool CopyRuntimeFiles(const FString& RunFolder, const FString& DocumentTemplate)
	{
		const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
		if (ContentDir.IsEmpty())
		{
			return false;
		}
		RuntimeDir = ContentDir / RunFolder;
		RuntimePackageName = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/Loc/ST_LocTest"));
		RuntimeTablePath = ArcRmlLocalizationTestsPrivate::MakeTableObjectPath(RunFolder);
		RuntimeTableFile = RuntimeDir / TEXT("Loc/ST_LocTest.uasset");
		IFileManager& FileManager = IFileManager::Get();
		return FileManager.MakeDirectory(*(RuntimeDir / TEXT("Loc")), true)
			&& FileManager.Copy(*(RuntimeDir / TEXT("LocTest.rml")), *(ContentDir / DocumentTemplate)) == COPY_OK
			&& FileManager.Copy(*RuntimeTableFile, *(ContentDir / TEXT("LocLoaded/Loc/ST_LocTest.uasset"))) == COPY_OK;
	}

	void ScanRuntimeTable()
	{
		TArray<FString> Files;
		Files.Add(RuntimeTableFile);
		IAssetRegistry::GetChecked().ScanFilesSynchronous(Files, false);
	}

	void OnRuntimeTableAdded(TFunction<void()>&& Action)
	{
		const FName PackageName(*RuntimePackageName);
		AssetAddedHandle = IAssetRegistry::GetChecked().OnAssetAdded().AddLambda([this, PackageName, Action = MoveTemp(Action)](const FAssetData& Asset)
		{
			if (bTableAdded || Asset.PackageName != PackageName)
			{
				return;
			}
			bTableAdded = true;
			StateWhenAdded = FArcRmlTestView::GetStringTableState(RuntimeTablePath);
			Action();
		});
	}

	TEST_METHOD(LoadedTable_ShowsTableText)
	{
		WaitForSettledTables();
		TestCommandBuilder.Then(TEXT("open the document while its table is loaded"), [this]()
		{
			const FString TablePath = ArcRmlLocalizationTestsPrivate::MakeTableObjectPath(TEXT("LocLoaded"));
			const EArcRmlStringTableState State = FArcRmlTestView::GetStringTableState(TablePath);
			ASSERT_THAT(IsTrue(State == EArcRmlStringTableState::Loaded, FString::Printf(TEXT("%s is %s"), *TablePath, ArcRmlLocalizationTestsPrivate::DescribeState(State))));

			ASSERT_THAT(IsTrue(OpenAndRead(ArcRmlUITestHelpers::MakePath(TEXT("LocLoaded/LocTest.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(IsFalse(View.GetProbe().bWaitingForStringTables));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-section"))));
		});
	}

	TEST_METHOD(OpenDuringTableLoad_WaitsThenShowsTableTextAndReplaysCommands)
	{
		WaitForSettledTables();
		const FString RunFolder = ArcRmlLocalizationTestsPrivate::MakeRunFolder();
		const FString DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LocTest.rml"));

		TestCommandBuilder
			.Then(TEXT("add a new string table and open a document that lists it while it loads"), [this, RunFolder, DocumentPath]()
			{
				ASSERT_THAT(IsTrue(CopyRuntimeFiles(RunFolder, TEXT("LocLoaded/LocTest.rml")), TEXT("the runtime files could not be copied")));
				OnRuntimeTableAdded([this, DocumentPath]()
				{
					bAcceptedWhileLoading = ArcRmlUITestHelpers::OpenView(View, DocumentPath);
					bFlushedWhileLoading = bAcceptedWhileLoading && View.Flush();
					bWaitedWhileLoading = View.GetProbe().bWaitingForStringTables;
					bShownWhileLoading = View.GetProbe().bHasDocument;
					View.SetRootClass(ArcRmlLocalizationTestsPrivate::DeferredClass, true);
					DeferredWhileLoading = View.Flush() ? View.GetProbe().DeferredCommands : INDEX_NONE;
				});
				ScanRuntimeTable();
			})
			.Until(TEXT("the asset registry adds the new string table"), [this]() -> bool
			{
				return bTableAdded;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the document opened while its table loaded waits and defers commands"), [this]()
			{
				ASSERT_THAT(IsTrue(StateWhenAdded == EArcRmlStringTableState::Loading,
					FString::Printf(TEXT("%s was %s when the registry added it"), *RuntimeTablePath, ArcRmlLocalizationTestsPrivate::DescribeState(StateWhenAdded))));
				ASSERT_THAT(IsTrue(bAcceptedWhileLoading && bFlushedWhileLoading, TEXT("the view did not open and answer a probe")));
				ASSERT_THAT(IsTrue(bWaitedWhileLoading, TEXT("the view did not wait for its loading table")));
				ASSERT_THAT(IsFalse(bShownWhileLoading, TEXT("the document was shown while its table loaded")));
				ASSERT_THAT(AreEqual(1, DeferredWhileLoading, TEXT("the root class change sent during the wait was not deferred")));
			})
			.Until(TEXT("the new string table settles"), [this]() -> bool
			{
				return FArcRmlTestView::GetStringTableState(RuntimeTablePath) != EArcRmlStringTableState::Loading;
			}, FTimespan::FromSeconds(30))
			.Until(TEXT("the waiting document is shown"), [this]() -> bool
			{
				if (!View.Flush() || !View.GetProbe().bHasDocument)
				{
					return false;
				}
				View.ParseLayout(Elements);
				FirstShownText = ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-section"));
				return true;
			}, FTimespan::FromSeconds(10))
			.Then(TEXT("the first shown probe has table text and the deferred command ran"), [this]()
			{
				ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), FirstShownText));
				ASSERT_THAT(IsFalse(View.GetProbe().bWaitingForStringTables));
				ASSERT_THAT(AreEqual(0, View.GetProbe().DeferredCommands));
				const FArcRmlTestLayoutElement* Body = Elements.FindByPredicate([](const FArcRmlTestLayoutElement& Element) -> bool
				{
					return Element.Tag == TEXT("body");
				});
				ASSERT_THAT(IsNotNull(Body));
				ASSERT_THAT(IsTrue(Body->Classes.Contains(ArcRmlLocalizationTestsPrivate::DeferredClass),
					FString::Printf(TEXT("body classes '%s' miss the class set during the wait"), *Body->Classes)));
			});
	}

	TEST_METHOD(TitleWithLoc_IsTranslated)
	{
		WaitForSettledTables();
		TestCommandBuilder.Then(TEXT("open the document and read its title"), [this]()
		{
			ASSERT_THAT(IsTrue(OpenAndRead(ArcRmlUITestHelpers::MakePath(TEXT("LocLoaded/LocTest.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TitleText), View.GetProbe().Title));
		});
	}

	TEST_METHOD(MissingListedTable_ReportsDoesNotExistAndStillShows)
	{
		WaitForSettledTables();
		TestCommandBuilder.Then(TEXT("force the table to missing and open the document"), [this]()
		{
			const FString TablePath = ArcRmlLocalizationTestsPrivate::MakeTableObjectPath(TEXT("LocMissing"));
			const FString DocumentPath = ArcRmlUITestHelpers::MakePath(TEXT("LocMissing/LocTest.rml"));
			ForcedTablePath = TablePath;
			FArcRmlTestView::ForceStringTableState(TablePath, EArcRmlStringTableState::Missing);
			const EArcRmlStringTableState State = FArcRmlTestView::GetStringTableState(TablePath);
			ASSERT_THAT(IsTrue(State == EArcRmlStringTableState::Missing, FString::Printf(TEXT("%s is %s"), *TablePath, ArcRmlLocalizationTestsPrivate::DescribeState(State))));

			ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, DocumentPath + TEXT("(3): string table"));
			ASSERT_THAT(IsTrue(OpenAndRead(DocumentPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, FString::Printf(TEXT("string table '%s' does not exist"), *TablePath), DocumentPath, 3),
				ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(FString(TEXT("Section")), ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-section"))));
		});
	}

	TEST_METHOD(FailedLoadEndsTheWait_ReportsFailedToLoadAndShows)
	{
		WaitForSettledTables();
		TestCommandBuilder.Then(TEXT("open the document while its table loads, then fail the load"), [this]()
		{
			const FString TablePath = ArcRmlLocalizationTestsPrivate::MakeTableObjectPath(TEXT("LocMissing"));
			const FString DocumentPath = ArcRmlUITestHelpers::MakePath(TEXT("LocMissing/LocTest.rml"));
			ForcedTablePath = TablePath;
			FArcRmlTestView::ForceStringTableState(TablePath, EArcRmlStringTableState::Loading);
			ASSERT_THAT(IsFalse(FArcRmlTestView::AreStringTablesSettled(), TEXT("a table forced to loading must keep the tables unsettled")));

			ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::OpenView(View, DocumentPath) && View.Flush(), TEXT("the view did not open and answer a probe")));
			ASSERT_THAT(IsTrue(View.GetProbe().bWaitingForStringTables, TEXT("the view did not wait for its loading table")));
			ASSERT_THAT(IsFalse(View.GetProbe().bHasDocument, TEXT("the document was shown while its table loaded")));

			ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, DocumentPath + TEXT("(3): string table"));
			FArcRmlTestView::ForceStringTableState(TablePath, EArcRmlStringTableState::Failed);
			ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements), TEXT("the flush after the failed load failed")));
			ASSERT_THAT(IsFalse(View.GetProbe().bWaitingForStringTables, TEXT("the failed load did not end the wait")));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, FString::Printf(TEXT("string table '%s' failed to load"), *TablePath), DocumentPath, 3),
				ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(FString(TEXT("Section")), ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-section"))));
		});
	}

	TEST_METHOD(EmbeddedTabWithUnloadedTable_ReportsMissingThenFailed)
	{
		WaitForSettledTables();
		const FString RunFolder = ArcRmlLocalizationTestsPrivate::MakeRunFolder();
		const FString TabPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LocTest.rml"));

		TestCommandBuilder
			.Then(TEXT("open the frame and add a tab with its own string table"), [this, RunFolder]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(ArcRmlUITestHelpers::MakePath(TEXT("LocEmbed/LocFrame.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(CopyRuntimeFiles(RunFolder, TEXT("LocEmbed/LocTabTemplate.rml")), TEXT("the runtime files could not be copied")));
				ScanRuntimeTable();
			})
			.Until(TEXT("the tab's string table loads"), [this]() -> bool
			{
				return FArcRmlTestView::GetStringTableState(RuntimeTablePath) == EArcRmlStringTableState::Loaded;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the tab embedded with its table missing, then failed, reports each"), [this, TabPath]()
			{
				ArcRmlUITestHelpers::ExpectLoggedError(*TestRunner, TabPath + TEXT("(8): string table"));
				ForcedTablePath = RuntimeTablePath;
				FArcRmlTestView::ForceStringTableState(RuntimeTablePath, EArcRmlStringTableState::Missing);
				ASSERT_THAT(IsTrue(View.Embed(TabPath), TEXT("the frame did not accept the tab")));
				ASSERT_THAT(IsTrue(View.Flush() && View.ParseLayout(Elements), TEXT("the flush after the embed failed")));
				ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, FString::Printf(TEXT("string table '%s' does not exist"), *RuntimeTablePath), TabPath, 8),
					ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(AreEqual(FString(TEXT("Section")), ArcRmlUITestHelpers::GetText(Elements, TEXT("tab-section"))));

				View.ClearEmbed();
				ASSERT_THAT(IsTrue(View.Flush(), TEXT("the flush after clearing the tab failed")));
				FArcRmlTestView::ForceStringTableState(RuntimeTablePath, EArcRmlStringTableState::Failed);
				ASSERT_THAT(IsTrue(View.Embed(TabPath), TEXT("the frame did not accept the tab again")));
				ASSERT_THAT(IsTrue(View.Flush(), TEXT("the flush after the second embed failed")));
				ASSERT_THAT(AreEqual(TabPath, View.GetProbe().EmbeddedPath, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(AreEqual(1, ArcRmlUITestHelpers::CountDiagnosticsAt(View, FString::Printf(TEXT("string table '%s' failed to load"), *RuntimeTablePath), TabPath, 8),
					ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			});
	}

	TEST_METHOD(EmbeddedTab_WaitsForItsTables)
	{
		WaitForSettledTables();
		const FString RunFolder = ArcRmlLocalizationTestsPrivate::MakeRunFolder();
		const FString TabPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LocTest.rml"));

		TestCommandBuilder
			.Then(TEXT("add a new string table and embed a tab that lists it while it loads"), [this, RunFolder, TabPath]()
			{
				ASSERT_THAT(IsTrue(OpenAndRead(ArcRmlUITestHelpers::MakePath(TEXT("LocEmbed/LocFrame.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(CopyRuntimeFiles(RunFolder, TEXT("LocEmbed/LocTabTemplate.rml")), TEXT("the runtime files could not be copied")));
				OnRuntimeTableAdded([this, TabPath]()
				{
					bAcceptedWhileLoading = View.Embed(TabPath);
					bFlushedWhileLoading = bAcceptedWhileLoading && View.Flush();
					bWaitedWhileLoading = View.GetProbe().bWaitingForStringTables;
					EmbeddedWhileLoading = View.GetProbe().EmbeddedPath;
				});
				ScanRuntimeTable();
			})
			.Until(TEXT("the asset registry adds the new string table"), [this]() -> bool
			{
				return bTableAdded;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("the tab embedded while its table loaded waits"), [this]()
			{
				ASSERT_THAT(IsTrue(StateWhenAdded == EArcRmlStringTableState::Loading,
					FString::Printf(TEXT("%s was %s when the registry added it"), *RuntimeTablePath, ArcRmlLocalizationTestsPrivate::DescribeState(StateWhenAdded))));
				ASSERT_THAT(IsTrue(bAcceptedWhileLoading, TEXT("the tab was not accepted by its frame")));
				ASSERT_THAT(IsTrue(bFlushedWhileLoading, TEXT("the flush after the embed failed")));
				ASSERT_THAT(IsTrue(bWaitedWhileLoading, TEXT("the embed did not wait for its loading table")));
				ASSERT_THAT(IsTrue(EmbeddedWhileLoading.IsEmpty(), FString::Printf(TEXT("the tab was embedded before its table loaded: '%s'"), *EmbeddedWhileLoading)));
			})
			.Until(TEXT("the new string table settles"), [this]() -> bool
			{
				return FArcRmlTestView::GetStringTableState(RuntimeTablePath) != EArcRmlStringTableState::Loading;
			}, FTimespan::FromSeconds(30))
			.Until(TEXT("the waiting tab is embedded"), [this, TabPath]() -> bool
			{
				return View.Flush() && View.GetProbe().EmbeddedPath == TabPath;
			}, FTimespan::FromSeconds(10))
			.Then(TEXT("the embedded tab shows table text"), [this]()
			{
				const EArcRmlStringTableState State = FArcRmlTestView::GetStringTableState(RuntimeTablePath);
				ASSERT_THAT(IsTrue(State == EArcRmlStringTableState::Loaded, FString::Printf(TEXT("%s is %s"), *RuntimeTablePath, ArcRmlLocalizationTestsPrivate::DescribeState(State))));
				ASSERT_THAT(IsFalse(View.GetProbe().bWaitingForStringTables));
				ASSERT_THAT(IsTrue(View.ParseLayout(Elements)));
				ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("tab-section"))));
			});
	}

	TEST_METHOD(ConstantTextOutsideDataModel_IsTranslated)
	{
		WaitForSettledTables();
		TestCommandBuilder.Then(TEXT("open the document without a data model"), [this]()
		{
			ASSERT_THAT(IsTrue(OpenAndRead(ArcRmlUITestHelpers::MakePath(TEXT("LocUnbound/LocUnboundTest.rml"))), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
			ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("loc-unbound"))));
			ASSERT_THAT(AreEqual(FString(TEXT("Constant text")), ArcRmlUITestHelpers::GetText(Elements, TEXT("const-unbound"))));
			ASSERT_THAT(AreEqual(FString(TEXT("{{ count }} left")), ArcRmlUITestHelpers::GetText(Elements, TEXT("variable-unbound"))));
		});
	}

	TEST_METHOD(CultureChange_RefreshesTheOpenViewInPlace)
	{
		WaitForSettledTables();
		const FString RunFolder = ArcRmlLocalizationTestsPrivate::MakeRunFolder();
		const FString DocumentPath = ArcRmlUITestHelpers::MakePath(RunFolder + TEXT("/LocTest.rml"));
		const FString EditedText = TEXT("typed");

		TestCommandBuilder
			.Then(TEXT("copy the culture fixture next to its string table"), [this, RunFolder]()
			{
				ASSERT_THAT(IsTrue(CopyRuntimeFiles(RunFolder, TEXT("LocCulture/LocCultureTemplate.rml")), TEXT("the runtime files could not be copied")));
				const FString ContentDir = ArcRmlUITestHelpers::GetTestContentDir();
				ASSERT_THAT(IsTrue(IFileManager::Get().MakeDirectory(*(RuntimeDir / TEXT("Preview")), true)
					&& IFileManager::Get().Copy(*(RuntimeDir / TEXT("Preview/LocCulture.json")), *(ContentDir / TEXT("LocCulture/Preview/LocCulture.json"))) == COPY_OK, TEXT("the preview data could not be copied")));
				ScanRuntimeTable();
			})
			.Until(TEXT("the fixture's string table loads"), [this]() -> bool
			{
				return FArcRmlTestView::GetStringTableState(RuntimeTablePath) == EArcRmlStringTableState::Loaded;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("open the view, edit one text area and read the texts before the change"), [this, DocumentPath, EditedText]()
			{
				FArcRmlInputBindings Bindings;
				Bindings.Back.Name = TEXT("back");
				Bindings.Back.Keys.Add(EKeys::Enter);
				View.SetInputBindings(Bindings);
				const FStringTablePtr RunTable = FStringTableRegistry::Get().FindMutableStringTable(FName(*RuntimeTablePath));
				ASSERT_THAT(IsTrue(RunTable.IsValid(), TEXT("the run string table is not registered")));
				RunTable->SetSourceString(TEXT("Multi"), TEXT("First line\tcolumn\r\nSecond line"), FString());
				ASSERT_THAT(IsTrue(OpenAndRead(DocumentPath), ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				ASSERT_THAT(IsTrue(View.GetProbe().bHasDocument, ArcRmlUITestHelpers::DescribeDiagnostics(View)));
				View.ClickElement(TEXT("#lc-area-edit"));
				ASSERT_THAT(IsTrue(View.Flush()));
				View.TypeText(EditedText);
				ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.ParseLayout(Elements)));

				ASSERT_THAT(AreEqual(FString(TEXT("lc-area-edit")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("the click did not focus the edited text area")));
				ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-section"))));
				ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TitleText), View.GetProbe().Title));
				ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-plain")).Contains(ArcRmlLocalizationTestsPrivate::TableText), TEXT("the plain text area does not show its table text")));
				ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-edit")).Contains(EditedText), TEXT("the typed text is not in the edited text area")));
				const FString MultiBefore = FText::FromStringTable(FName(*RuntimeTablePath), TEXT("Multi")).ToString();
				ASSERT_THAT(AreEqual(ArcRmlLocalizationTestsPrivate::Squash(MultiBefore), ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-crlf")).Replace(TEXT("\n"), TEXT("")).Replace(TEXT(" "), TEXT(""))));
				KeyLabelBefore = ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-key"));
				ASSERT_THAT(IsFalse(KeyLabelBefore.IsEmpty()));
				ASSERT_THAT(AreEqual(FString(ArcRmlLocalizationTestsPrivate::TableText), ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-bound"))));

				const FArcRmlTestLayoutElement* Section = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-section"));
				const FArcRmlTestLayoutElement* Bound = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-bound"));
				const FArcRmlTestLayoutElement* PlainArea = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-area-plain"));
				ASSERT_THAT(IsNotNull(Section));
				ASSERT_THAT(IsNotNull(Bound));
				ASSERT_THAT(IsNotNull(PlainArea));
				SectionPathBefore = Section->Path;
				BoundPathBefore = Bound->Path;
				PlainAreaPathBefore = PlainArea->Path;
			})
			.Then(TEXT("change the culture to LEET"), [this]()
			{
				PreviousCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
				bCultureChanged = true;
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(TEXT("LEET")), TEXT("the LEET culture could not be set")));
			})
			.Until(TEXT("the open view shows the LEET section"), [this]() -> bool
			{
				return View.Flush() && View.ParseLayout(Elements)
					&& ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-section")) != ArcRmlLocalizationTestsPrivate::TableText;
			}, FTimespan::FromSeconds(30))
			.Then(TEXT("every text switched on the same elements and focus stayed"), [this, EditedText]()
			{
				ASSERT_THAT(IsTrue(View.Flush() && View.Flush() && View.Flush() && View.ParseLayout(Elements)));
				const FString LeetSection = FText::FromStringTable(FName(*RuntimeTablePath), TEXT("Section")).ToString();
				const FString LeetTitle = FText::FromStringTable(FName(*RuntimeTablePath), TEXT("Title")).ToString();
				ASSERT_THAT(IsTrue(LeetSection.StartsWith(TEXT("\u2021")) && LeetSection.EndsWith(TEXT("\u2021")) && LeetSection != ArcRmlLocalizationTestsPrivate::TableText, FString::Printf(TEXT("the LEET culture shows '%s'"), *LeetSection)));

				ASSERT_THAT(AreEqual(LeetSection, ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-section"))));
				ASSERT_THAT(AreEqual(LeetTitle, View.GetProbe().Title));
				ASSERT_THAT(AreEqual(LeetSection, ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-bound"))));
				ASSERT_THAT(AreEqual(ArcRmlKeyLabels::GetKeyLabel(EKeys::Enter), ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-key"))));
				ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-key")) != KeyLabelBefore, TEXT("ArcKeys label did not change")));
				ASSERT_THAT(IsTrue(ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-plain")).Contains(LeetSection), TEXT("unedited text area")));

				const FString LeetMulti = FText::FromStringTable(FName(*RuntimeTablePath), TEXT("Multi")).ToString();
				ASSERT_THAT(IsTrue(LeetMulti.StartsWith(TEXT("\u2021")), FString::Printf(TEXT("the LEET culture shows '%s' for the tab and CRLF text"), *LeetMulti)));
				ASSERT_THAT(AreEqual(ArcRmlLocalizationTestsPrivate::Squash(LeetMulti), ArcRmlLocalizationTestsPrivate::Squash(ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-crlf")))));

				const FString EditedArea = ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-area-edit"));
				ASSERT_THAT(IsTrue(EditedArea.Contains(EditedText), TEXT("the edited text area lost its typed value")));
				ASSERT_THAT(IsFalse(EditedArea.Contains(TEXT("\u2021")), TEXT("the edited text area was translated over its value")));

				ASSERT_THAT(AreEqual(FString(TEXT("lc-area-edit")), ArcRmlUITestHelpers::GetFocusDomId(View), TEXT("focus moved")));

				const FArcRmlTestLayoutElement* Section = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-section"));
				const FArcRmlTestLayoutElement* Bound = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-bound"));
				const FArcRmlTestLayoutElement* PlainArea = ArcRmlUITestHelpers::FindByDomId(Elements, TEXT("lc-area-plain"));
				ASSERT_THAT(IsNotNull(Section));
				ASSERT_THAT(IsNotNull(Bound));
				ASSERT_THAT(IsNotNull(PlainArea));
				ASSERT_THAT(AreEqual(SectionPathBefore, Section->Path));
				ASSERT_THAT(AreEqual(BoundPathBefore, Bound->Path));
				ASSERT_THAT(AreEqual(PlainAreaPathBefore, PlainArea->Path));
			})
			.Then(TEXT("restore the previous culture"), [this]()
			{
				bCultureChanged = false;
				ASSERT_THAT(IsTrue(FInternationalization::Get().SetCurrentCulture(PreviousCulture), TEXT("the previous culture could not be restored")));
			})
			.Until(TEXT("the open view shows the restored section"), [this]() -> bool
			{
				return View.Flush() && View.ParseLayout(Elements)
					&& ArcRmlUITestHelpers::GetText(Elements, TEXT("lc-section")) == ArcRmlLocalizationTestsPrivate::TableText;
			}, FTimespan::FromSeconds(30));
	}
};
