// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlDocumentTemplates.h"
#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "Host/ArcRmlInputBindings.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

TEST_CLASS(ArcRmlPreviewCreationTests, "ArcRmlUI.PreviewCreation")
{
	TEST_METHOD(DefaultContexts_MapTheCommonUIBackAction)
	{
		const FArcRmlInputBindings Bindings = ArcRmlInputBindings::GatherFromContexts(FArcRmlAppliedContext(), ArcRmlInputBindings::LoadDefaultContexts());
		ASSERT_THAT(IsFalse(Bindings.Back.Keys.IsEmpty(), FString::Printf(TEXT("no default mapping context maps the CommonUI back action '%s', so the preview reads no back key"),
			*Bindings.Back.ActionName)));
	}

	TEST_METHOD(DocumentNames_AreIdentifiers)
	{
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::IsValidDocumentName(TEXT("MyDoc"))));
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::IsValidDocumentName(TEXT("_Panel2"))));
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::IsValidDocumentName(TEXT("My-Doc"))));
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::IsValidDocumentName(TEXT("My Doc"))));
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::IsValidDocumentName(TEXT("1Doc"))));
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::IsValidDocumentName(FString())));
	}

	TEST_METHOD(Rename_RefusesBadAndTakenNames)
	{
		const FString Folder = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Focus"));
		const FString Filename = FPaths::Combine(Folder, TEXT("NavClipTest.rml"));
		const FName InternalPath(TEXT("/ArcRmlUITest/UI/Rml/Test/Focus/NavClipTest.rml"));
		FText Error;
		const FString BadName = TEXT("Nav Clip");
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &BadName, &Error)));
		const FString TakenName = TEXT("AreaBackTest");
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &TakenName, &Error), TEXT("a name of another document in the folder was accepted")));
		const FString FreeName = TEXT("NavClipRenamed");
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &FreeName, &Error), Error.ToString()));
		const FString NoneName = TEXT("None");
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &NoneName, &Error), TEXT("'None' was accepted")));
		const FString DeviceName = TEXT("CON");
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &DeviceName, &Error), TEXT("the reserved device name 'CON' was accepted")));
	}

	TEST_METHOD(Creation_RefusesANameWhosePreviewFileExists)
	{
		const FString Folder = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("CreationScratch"));
		const FString PreviewFile = FPaths::Combine(Folder, TEXT("Preview/JsonOnly.json"));
		ON_SCOPE_EXIT
		{
			IFileManager::Get().DeleteDirectory(*Folder, false, true);
			ArcRmlDocumentTemplates::BeginCreation(EArcRmlDocumentKind::Screen);
		};
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{}"), *PreviewFile)));
		ArcRmlDocumentTemplates::BeginCreation(EArcRmlDocumentKind::Screen);
		const FString Filename = FPaths::Combine(Folder, TEXT("NewDocument.rml"));
		const FName InternalPath(TEXT("/ArcRmlUITest/UI/Rml/Test/CreationScratch/NewDocument.rml"));
		FText Error;
		const FString TakenName = TEXT("JsonOnly");
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &TakenName, &Error),
			TEXT("a new document was named after an existing preview file, so Create fails after the name is accepted")));
		ASSERT_THAT(IsTrue(Error.ToString().Contains(TEXT("JsonOnly.json")), Error.ToString()));
		const FString FreeName = TEXT("JsonFree");
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &FreeName, &Error), Error.ToString()));

		ArcRmlDocumentTemplates::BeginCreation(EArcRmlDocumentKind::Component);
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::CanRename(InternalPath, Filename, &TakenName, &Error),
			TEXT("a component, which writes no preview file, was refused for an existing preview file")));
	}

	TEST_METHOD(Creation_IsOfferedOnlyInDocumentFolders)
	{
		const TSharedPtr<IPlugin> TestPlugin = IPluginManager::Get().FindPlugin(TEXT("ArcRmlUITest"));
		ASSERT_THAT(IsTrue(TestPlugin.IsValid(), TEXT("the ArcRmlUITest plugin is not loaded")));
		const FString UIFolder = FPaths::ConvertRelativePathToFull(TestPlugin->GetContentDir() / TEXT("UI/Rml"));
		FText Error;
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::CanCreate(FName(TEXT("/ArcRmlUITest/UI/Rml")), UIFolder, &Error), Error.ToString()));
		ASSERT_THAT(IsTrue(ArcRmlDocumentTemplates::CanCreate(FName(TEXT("/ArcRmlUITest/UI/Rml/Components")), UIFolder / TEXT("Components"), &Error), Error.ToString()));
		ASSERT_THAT(IsFalse(ArcRmlDocumentTemplates::CanCreate(FName(TEXT("/ArcRmlUITest/UI/Rml/Fonts")), UIFolder / TEXT("Fonts"), &Error), TEXT("the font folder offers document creation")));
	}
};
