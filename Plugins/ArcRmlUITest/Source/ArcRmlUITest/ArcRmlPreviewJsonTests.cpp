// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "CQTest.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "MVVM/ArcRmlModelBinding.h"
#include "MVVM/ArcRmlPreviewData.h"
#include "MVVM/ArcRmlViewModelDeclarations.h"
#include "Preview/ArcRmlPreviewJson.h"
#include "Preview/ArcRmlPreviewSelfWrites.h"
#include "Preview/SArcRmlPreviewJsonPanel.h"

namespace ArcRmlPreviewJsonTestsPrivate
{
	bool MakeFile(const FString& Text, FArcRmlPreviewJsonFile& OutFile, FString& OutError)
	{
		OutFile = FArcRmlPreviewJsonFile();
		OutFile.PreviewPath = TEXT("/ArcRmlUITest/UI/Rml/Test/Preview/Inline.json");
		OutFile.Text = Text;
		FArcRmlPreviewJsonError Error;
		const bool bParsed = ArcRmlPreviewJson::Parse(OutFile, Error);
		OutError = Error.Message;
		return bParsed;
	}

	TArray<FArcRmlPathEntry> MakePath(const FString& Key)
	{
		TArray<FArcRmlPathEntry> Path;
		Path.Add(FArcRmlPathEntry{Key, INDEX_NONE});
		return Path;
	}

	FArcRmlValue MakeFloat(double Value)
	{
		FArcRmlValue Result;
		Result.Kind = EArcRmlValueKind::Float;
		Result.Float = Value;
		return Result;
	}

	FArcRmlValue MakeStruct(const TArray<FString>& Names, const TArray<FArcRmlValue>& Children)
	{
		FArcRmlValue Result;
		Result.Kind = EArcRmlValueKind::Struct;
		Result.MemberNames = Names;
		Result.Children = Children;
		return Result;
	}
}

TEST_CLASS(ArcRmlPreviewJsonTests, "ArcRmlUI.PreviewJson")
{
	TEST_METHOD(OverrideCopy_KeepsTheBaseLayout)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\n\t\"items\": [\n\t\t{ \"id\": 1, \"tags\": [\"a\", \"b\"] },\n\t\t{ \"id\": 2, \"tags\": [] }\n\t],\n\t\"$variants\": {\n\t\t\"alt\": {}\n\t}\n}\n");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddOverride(File, TEXT("alt"), TEXT("items"), Error), Error));

		const FString Expected = TEXT("{\n\t\"items\": [\n\t\t{ \"id\": 1, \"tags\": [\"a\", \"b\"] },\n\t\t{ \"id\": 2, \"tags\": [] }\n\t],\n\t\"$variants\": {\n\t\t\"alt\": {\n")
			TEXT("\t\t\t\"items\": [\n\t\t\t\t{ \"id\": 1, \"tags\": [\"a\", \"b\"] },\n\t\t\t\t{ \"id\": 2, \"tags\": [] }\n\t\t\t]\n\t\t}\n\t}\n}\n");
		ASSERT_THAT(AreEqual(Expected, File.Text));
	}

	TEST_METHOD(Override_GoesInBaseKeyOrder)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\n\t\"a\": 1,\n\t\"b\": 2,\n\t\"c\": 3,\n\t\"$variants\": {\n\t\t\"alt\": {\n\t\t\t\"c\": 30\n\t\t}\n\t}\n}");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddOverride(File, TEXT("alt"), TEXT("a"), Error), Error));

		const FString Expected = TEXT("{\n\t\"a\": 1,\n\t\"b\": 2,\n\t\"c\": 3,\n\t\"$variants\": {\n\t\t\"alt\": {\n\t\t\t\"a\": 1,\n\t\t\t\"c\": 30\n\t\t}\n\t}\n}");
		ASSERT_THAT(AreEqual(Expected, File.Text));
	}

	TEST_METHOD(ScalarIntoEmptyArray_TakesTheParentLineForm)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{\n\t\"list\": []\n}"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::InsertElement(File, FString(), MakePath(TEXT("list")), 0, ArcRmlUITestHelpers::MakeString(TEXT("x")), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"list\": [\n\t\t\"x\"\n\t]\n}")), File.Text));
	}

	TEST_METHOD(InsertedElement_TakesTheSiblingForm)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\n\t\"rows\": [\n\t\t{\n\t\t\t\"id\": 1, \"name\": \"one\"\n\t\t}\n\t]\n}");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));

		TArray<FString> Names;
		Names.Add(TEXT("id"));
		Names.Add(TEXT("name"));
		TArray<FArcRmlValue> Children;
		Children.Add(ArcRmlUITestHelpers::MakeInt(0));
		Children.Add(ArcRmlUITestHelpers::MakeString(FString()));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::InsertElement(File, FString(), MakePath(TEXT("rows")), 1, MakeStruct(Names, Children), Error), Error));

		const FString Expected = TEXT("{\n\t\"rows\": [\n\t\t{\n\t\t\t\"id\": 1, \"name\": \"one\"\n\t\t},\n\t\t{\n\t\t\t\"id\": 0, \"name\": \"\"\n\t\t}\n\t]\n}");
		ASSERT_THAT(AreEqual(Expected, File.Text));
	}

	TEST_METHOD(FloatWidenedFromSingle_PrintsShortestDigits)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{ \"speed\": 0.5 }"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::SetScalar(File, FString(), MakePath(TEXT("speed")), MakeFloat(static_cast<double>(0.1f)), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"speed\": 0.1 }")), File.Text));
	}

	TEST_METHOD(Float_KeepsTheExponentNotation)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{ \"range\": 1.5E3 }"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::SetScalar(File, FString(), MakePath(TEXT("range")), MakeFloat(2500.0), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"range\": 2.5E3 }")), File.Text));
	}

	TEST_METHOD(NewLine_FollowsTheEditedLine)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\r\n\t\"a\": 1,\r\n\t\"b\": [\n\t\t1\n\t]\n}");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::InsertElement(File, FString(), MakePath(TEXT("b")), 1, ArcRmlUITestHelpers::MakeInt(2), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\r\n\t\"a\": 1,\r\n\t\"b\": [\n\t\t1,\n\t\t2\n\t]\n}")), File.Text));
	}

	TEST_METHOD(IndentUnit_IsTheSmallestIndent)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\n\"a\": {\n      \"q\": 1\n  },\n  \"e\": []\n}");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::InsertElement(File, FString(), MakePath(TEXT("e")), 0, ArcRmlUITestHelpers::MakeInt(7), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\"a\": {\n      \"q\": 1\n  },\n  \"e\": [\n    7\n  ]\n}")), File.Text));
	}

	TEST_METHOD(Utf16Files_LoadAndWriteInTheirEncoding)
	{
		TArray<FString> Names;
		Names.Add(TEXT("Preview/Utf16LE.json"));
		Names.Add(TEXT("Preview/Utf16BE.json"));
		for (const FString& Name : Names)
		{
			FArcRmlPreviewJsonFile File;
			FArcRmlPreviewJsonError Error;
			ASSERT_THAT(IsTrue(ArcRmlPreviewJson::Load(ArcRmlUITestHelpers::MakePath(Name), File, Error), Error.Message));
			const FArcRmlPreviewJsonNode* Title = ArcRmlPreviewJson::FindNode(File, FString(), ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("title")));
			ASSERT_THAT(IsNotNull(Title));
			ASSERT_THAT(AreEqual(FString(TEXT("Zażółć")), Title->String));

			FString EditError;
			ASSERT_THAT(IsTrue(ArcRmlPreviewJson::SetScalar(File, FString(), ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("count")), ArcRmlUITestHelpers::MakeInt(4), EditError),
				EditError));
			TArray<uint8> Bytes;
			ArcRmlPreviewJson::Encode(File, Bytes);
			FArcRmlPreviewJsonFile Decoded;
			Decoded.PreviewPath = File.PreviewPath;
			ASSERT_THAT(IsTrue(ArcRmlPreviewJson::Decode(Bytes, Decoded, Error), Error.Message));
			ASSERT_THAT(AreEqual(static_cast<int32>(File.Encoding), static_cast<int32>(Decoded.Encoding)));
			ASSERT_THAT(AreEqual(File.Text, Decoded.Text));
			ASSERT_THAT(IsTrue(Decoded.Text.Contains(TEXT("\"count\": 4"))));
		}
	}

	TEST_METHOD(InvalidUtf8_NamesTheByteOffset)
	{
		FArcRmlPreviewJsonFile File;
		FArcRmlPreviewJsonError Error;
		ASSERT_THAT(IsFalse(ArcRmlPreviewJson::Load(ArcRmlUITestHelpers::MakePath(TEXT("Preview/InvalidUtf8.json")), File, Error)));
		ASSERT_THAT(IsTrue(Error.Message.Contains(TEXT("byte 14 (0xff)")), Error.Message));
		ASSERT_THAT(AreEqual(2, Error.Line));
	}

	TEST_METHOD(VariantNames_CompareWithCase)
	{
		FArcRmlPreviewJsonNode Root;
		FString Error;
		ASSERT_THAT(IsTrue(ArcRmlPreviewData::Parse(TEXT("{ \"title\": \"Base\", \"Title\": \"Other\", \"$variants\": { \"Alt\": { \"title\": \"Alt\" } } }"), Root, Error), Error));
		ASSERT_THAT(IsNull(ArcRmlPreviewData::FindVariant(Root, TEXT("alt"))));
		ASSERT_THAT(IsNotNull(ArcRmlPreviewData::FindVariant(Root, TEXT("Alt"))));

		FArcRmlValue Snapshot;
		ArcRmlPreviewData::MakeSnapshot(Root, TEXT("Alt"), Snapshot);
		ASSERT_THAT(AreEqual(2, Snapshot.MemberNames.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("Alt")), Snapshot.Children[0].String));
		ASSERT_THAT(AreEqual(FString(TEXT("Other")), Snapshot.Children[1].String));

		FArcRmlPreviewJsonFile File;
		ASSERT_THAT(IsTrue(ArcRmlPreviewJsonTestsPrivate::MakeFile(TEXT("{ \"title\": \"Base\", \"$variants\": { \"Alt\": {} } }"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddVariant(File, TEXT("alt"), Error), Error));
		ASSERT_THAT(IsNotNull(ArcRmlPreviewData::FindVariant(File.Root, TEXT("alt"))));
		ASSERT_THAT(IsNotNull(ArcRmlPreviewData::FindVariant(File.Root, TEXT("Alt"))));
	}

	TEST_METHOD(ObjectElement_IntoEmptyArrayTakesMembers)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{\n\t\"rows\": []\n}"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::InsertElement(File, FString(), MakePath(TEXT("rows")), 0, MakeStruct(TArray<FString>(), TArray<FArcRmlValue>()), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"rows\": [\n\t\t{}\n\t]\n}")), File.Text));

		TArray<FArcRmlPathEntry> RowPath = MakePath(TEXT("rows"));
		RowPath.Add(FArcRmlPathEntry{FString(), 0});
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddMember(File, FString(), RowPath, TEXT("id"), ArcRmlUITestHelpers::MakeInt(1), Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddMember(File, FString(), RowPath, TEXT("name"), ArcRmlUITestHelpers::MakeString(TEXT("one")), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"rows\": [\n\t\t{\n\t\t\t\"id\": 1,\n\t\t\t\"name\": \"one\"\n\t\t}\n\t]\n}")), File.Text));
		ASSERT_THAT(IsFalse(ArcRmlPreviewJson::AddMember(File, FString(), RowPath, TEXT("id"), ArcRmlUITestHelpers::MakeInt(2), Error), TEXT("a second 'id' was added")));
	}

	TEST_METHOD(BaseKeys_RenameAndRemoveTheirOverrides)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		const FString Text = TEXT("{\n\t\"a\": 1,\n\t\"b\": 2,\n\t\"$variants\": {\n\t\t\"alt\": {\n\t\t\t\"a\": 10\n\t\t}\n\t}\n}");
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(Text, File, Error), Error));

		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::RenameMember(File, FString(), TArray<FArcRmlPathEntry>(), TEXT("a"), TEXT("z"), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"z\": 1,\n\t\"b\": 2,\n\t\"$variants\": {\n\t\t\"alt\": {\n\t\t\t\"z\": 10\n\t\t}\n\t}\n}")), File.Text));

		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::AddMember(File, FString(), TArray<FArcRmlPathEntry>(), TEXT("c"), ArcRmlUITestHelpers::MakeBool(true), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"z\": 1,\n\t\"b\": 2,\n\t\"c\": true,\n\t\"$variants\": {\n\t\t\"alt\": {\n\t\t\t\"z\": 10\n\t\t}\n\t}\n}")), File.Text));

		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::RemoveMember(File, FString(), TArray<FArcRmlPathEntry>(), TEXT("z"), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{\n\t\"b\": 2,\n\t\"c\": true,\n\t\"$variants\": {\n\t\t\"alt\": {}\n\t}\n}")), File.Text));
		ASSERT_THAT(IsFalse(ArcRmlPreviewJson::AddMember(File, FString(), TArray<FArcRmlPathEntry>(), TEXT("$variants"), ArcRmlUITestHelpers::MakeInt(1), Error)));
	}

	TEST_METHOD(ValueKind_Changes)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{ \"a\": 1, \"b\": { \"c\": 2 } }"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::ReplaceValue(File, FString(), MakePath(TEXT("a")), ArcRmlUITestHelpers::MakeString(FString()), Error), Error));

		TArray<FArcRmlPathEntry> InnerPath = MakePath(TEXT("b"));
		InnerPath.Add(FArcRmlPathEntry{TEXT("c"), INDEX_NONE});
		FArcRmlValue Array;
		Array.Kind = EArcRmlValueKind::Array;
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::ReplaceValue(File, FString(), InnerPath, Array, Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": \"\", \"b\": { \"c\": [] } }")), File.Text));
	}

	TEST_METHOD(Commit_KeepsAFileChangedOnDisk)
	{
		const FString PreviewPath = ArcRmlUITestHelpers::MakePath(TEXT("Preview/CommitScratch.json"));
		const FString Filename = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Preview/CommitScratch.json"));
		ON_SCOPE_EXIT
		{
			IFileManager::Get().Delete(*Filename, false, true, true);
		};
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 1 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));

		FArcRmlPreviewJsonFile File;
		FArcRmlPreviewJsonError LoadError;
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::Load(PreviewPath, File, LoadError), LoadError.Message));
		const TSharedRef<FArcRmlPreviewJsonSession> Session = MakeShared<FArcRmlPreviewJsonSession>(MoveTemp(File), TWeakPtr<FArcRmlModelBinding>(), FString());
		Session->SetScalar(ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("a")), ArcRmlUITestHelpers::MakeInt(2));
		ASSERT_THAT(IsTrue(Session->Commit(), Session->GetError().ToString()));
		FString OnDisk;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(OnDisk, *Filename)));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 2 }")), OnDisk));

		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 7 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));
		Session->SetScalar(ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("a")), ArcRmlUITestHelpers::MakeInt(3));
		TestRunner->AddExpectedErrorPlain(TEXT("changed on disk after the preview read it"), EAutomationExpectedErrorFlags::Contains, 1);
		ASSERT_THAT(IsFalse(Session->Commit(), TEXT("a pending edit overwrote a file another program changed")));
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(OnDisk, *Filename)));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 7 }")), OnDisk));
		ASSERT_THAT(IsTrue(Session->HasConflict()));
		ASSERT_THAT(IsTrue(Session->GetFile().Text.Contains(TEXT("\"a\": 3")), TEXT("the refused edit was dropped")));

		Session->ResolveConflict(true);
		ASSERT_THAT(IsFalse(Session->HasConflict()));
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(OnDisk, *Filename)));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 3 }")), OnDisk));
	}

	TEST_METHOD(Close_AsksBeforeDroppingAConflictingEdit)
	{
		const FString PreviewPath = ArcRmlUITestHelpers::MakePath(TEXT("Preview/CloseScratch.json"));
		const FString Filename = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Preview/CloseScratch.json"));
		ON_SCOPE_EXIT
		{
			IFileManager::Get().Delete(*Filename, false, true, true);
		};
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 1 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));

		FArcRmlPreviewJsonFile File;
		FArcRmlPreviewJsonError LoadError;
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::Load(PreviewPath, File, LoadError), LoadError.Message));
		const TSharedRef<FArcRmlPreviewJsonSession> Session = MakeShared<FArcRmlPreviewJsonSession>(MoveTemp(File), TWeakPtr<FArcRmlModelBinding>(), FString());
		const TArray<TSharedPtr<FArcRmlPreviewJsonSession>> Sessions = { Session };
		int32 Asked = 0;
		ASSERT_THAT(IsTrue(FArcRmlPreviewJsonSession::ResolveBeforeClose(Sessions, [&Asked](const FText&) -> EAppReturnType::Type
		{
			++Asked;
			return EAppReturnType::Cancel;
		})));
		ASSERT_THAT(AreEqual(0, Asked, TEXT("a session without a conflict asked before closing")));

		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 7 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));
		Session->SetScalar(ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("a")), ArcRmlUITestHelpers::MakeInt(3));
		TestRunner->AddExpectedErrorPlain(TEXT("changed on disk after the preview read it"), EAutomationExpectedErrorFlags::Contains, 1);
		ASSERT_THAT(IsFalse(Session->Commit()));

		FText Message;
		ASSERT_THAT(IsFalse(FArcRmlPreviewJsonSession::ResolveBeforeClose(Sessions, [&Message](const FText& InMessage) -> EAppReturnType::Type
		{
			Message = InMessage;
			return EAppReturnType::Cancel;
		}), TEXT("Cancel closed the tab")));
		ASSERT_THAT(IsTrue(Message.ToString().Contains(PreviewPath), Message.ToString()));
		ASSERT_THAT(IsTrue(Session->HasConflict(), TEXT("Cancel dropped the edit")));

		ASSERT_THAT(IsTrue(FArcRmlPreviewJsonSession::ResolveBeforeClose(Sessions, [](const FText&) -> EAppReturnType::Type
		{
			return EAppReturnType::Yes;
		})));
		ASSERT_THAT(IsFalse(Session->HasConflict()));
		FString OnDisk;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(OnDisk, *Filename)));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 3 }")), OnDisk, TEXT("Yes did not write the edit")));
	}

	TEST_METHOD(Close_UnattendedDropsTheEditAndKeepsTheFile)
	{
		ASSERT_THAT(IsTrue(FApp::IsUnattended(), TEXT("this test needs an unattended run")));
		const FString PreviewPath = ArcRmlUITestHelpers::MakePath(TEXT("Preview/UnattendedScratch.json"));
		const FString Filename = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Preview/UnattendedScratch.json"));
		ON_SCOPE_EXIT
		{
			IFileManager::Get().Delete(*Filename, false, true, true);
		};
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 1 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));

		FArcRmlPreviewJsonFile File;
		FArcRmlPreviewJsonError LoadError;
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::Load(PreviewPath, File, LoadError), LoadError.Message));
		const TSharedRef<FArcRmlPreviewJsonSession> Session = MakeShared<FArcRmlPreviewJsonSession>(MoveTemp(File), TWeakPtr<FArcRmlModelBinding>(), FString());
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(TEXT("{ \"a\": 7 }"), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)));
		Session->SetScalar(ArcRmlPreviewJsonTestsPrivate::MakePath(TEXT("a")), ArcRmlUITestHelpers::MakeInt(3));
		TestRunner->AddExpectedErrorPlain(TEXT("changed on disk after the preview read it"), EAutomationExpectedErrorFlags::Contains, 1);
		ASSERT_THAT(IsFalse(Session->Commit()));

		TestRunner->AddExpectedErrorPlain(TEXT("unattended run"), EAutomationExpectedErrorFlags::Contains, 1);
		const TArray<TSharedPtr<FArcRmlPreviewJsonSession>> Sessions = { Session };
		ASSERT_THAT(IsTrue(FArcRmlPreviewJsonSession::ResolveBeforeClose(Sessions, &FArcRmlPreviewJsonSession::AskBeforeClose),
			TEXT("an unattended run cannot close a tab with a conflicting edit")));
		ASSERT_THAT(IsFalse(Session->HasConflict()));
		FString OnDisk;
		ASSERT_THAT(IsTrue(FFileHelper::LoadFileToString(OnDisk, *Filename)));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 7 }")), OnDisk, TEXT("the unattended close wrote the edit over the file")));
	}

	TEST_METHOD(Float_KeepsTheMantissaAndExponentForm)
	{
		using namespace ArcRmlPreviewJsonTestsPrivate;
		FArcRmlPreviewJsonFile File;
		FString Error;
		ASSERT_THAT(IsTrue(MakeFile(TEXT("{ \"a\": 1e5, \"b\": 1.5e+05 }"), File, Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::SetScalar(File, FString(), MakePath(TEXT("a")), MakeFloat(200000.0), Error), Error));
		ASSERT_THAT(IsTrue(ArcRmlPreviewJson::SetScalar(File, FString(), MakePath(TEXT("b")), MakeFloat(250000.0), Error), Error));
		ASSERT_THAT(AreEqual(FString(TEXT("{ \"a\": 2e5, \"b\": 2.5e+05 }")), File.Text));
	}

	TEST_METHOD(SelfWrite_IsConsumedByItsFirstChangeEvent)
	{
		const FString Filename = FPaths::Combine(ArcRmlUITestHelpers::GetTestContentDir(), TEXT("Preview/SelfWrite.json"));
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>("{ \"a\": 1 }"), 10);

		FArcRmlPreviewJsonFile WriterFile;
		WriterFile.PreviewPath = TEXT("/ArcRmlUITest/UI/Rml/Test/Preview/SelfWrite.json");
		const TSharedRef<FArcRmlPreviewJsonSession> Writer = MakeShared<FArcRmlPreviewJsonSession>(MoveTemp(WriterFile), TWeakPtr<FArcRmlModelBinding>(), FString());

		int32 Reads = 0;
		TArray<uint8> OnDisk = Bytes;
		const TFunction<bool(TArray<uint8>&)> ReadFile = [&Reads, &OnDisk](TArray<uint8>& OutBytes) -> bool
		{
			++Reads;
			OutBytes = OnDisk;
			return true;
		};

		FArcRmlPreviewSelfWrites SelfWrites;
		TWeakPtr<FArcRmlPreviewJsonSession> Consumed;
		ASSERT_THAT(IsFalse(SelfWrites.Consume(Filename, ReadFile, Consumed)));
		ASSERT_THAT(AreEqual(0, Reads, TEXT("a change without a record read the file")));
		SelfWrites.EndWindow();

		SelfWrites.Record(Filename, Bytes, Writer);
		ASSERT_THAT(IsTrue(SelfWrites.Consume(Filename, ReadFile, Consumed)));
		ASSERT_THAT(IsTrue(Consumed.Pin() == Writer));
		ASSERT_THAT(IsTrue(SelfWrites.Consume(Filename, ReadFile, Consumed), TEXT("a second notification of the same save in the window counted as external")));
		ASSERT_THAT(AreEqual(1, Reads));
		SelfWrites.EndWindow();
		ASSERT_THAT(IsFalse(SelfWrites.Consume(Filename, ReadFile, Consumed), TEXT("a byte-identical later write still counts as the self-write")));
		SelfWrites.EndWindow();

		SelfWrites.Record(Filename, Bytes, Writer);
		OnDisk[7] = '2';
		ASSERT_THAT(IsFalse(SelfWrites.Consume(Filename, ReadFile, Consumed)));
		SelfWrites.EndWindow();
		OnDisk = Bytes;
		ASSERT_THAT(IsFalse(SelfWrites.Consume(Filename, ReadFile, Consumed), TEXT("a record survived a change with other content")));
	}

	TEST_METHOD(PreviewSet_WithoutPreviewModelsFails)
	{
		const TSharedPtr<FArcRmlUIThread> UIThread = FArcRmlUIModule::Get().GetOrStartUIThread();
		ASSERT_THAT(IsTrue(UIThread.IsValid()));

		const TSharedRef<FArcRmlModelBinding> Binding = MakeShared<FArcRmlModelBinding>(UIThread.ToSharedRef(), 0, nullptr, nullptr, TArray<FArcRmlViewModelDeclaration>(),
			FString(), ArcRmlUITestHelpers::MakePath(TEXT("Preview/NoModels.rml")));
		Binding->Start();
		ASSERT_THAT(IsFalse(Binding->DeclaresPreviewModels()));
		ASSERT_THAT(IsFalse(Binding->SetPreviewVariant(FString()), TEXT("the base variant was set on a document with no preview models")));
		Binding->Shutdown();
	}

	TEST_METHOD(Binding_ReadsOnlyItsPreviewFiles)
	{
		const TSharedPtr<FArcRmlUIThread> UIThread = FArcRmlUIModule::Get().GetOrStartUIThread();
		ASSERT_THAT(IsTrue(UIThread.IsValid()));

		TArray<FArcRmlViewModelDeclaration> Declarations;
		FArcRmlViewModelDeclaration& Declaration = Declarations.AddDefaulted_GetRef();
		Declaration.Name = TEXT("model");
		Declaration.PreviewPath = ArcRmlUITestHelpers::MakePath(TEXT("Preview/Read.json"));
		const TSharedRef<FArcRmlModelBinding> Binding = MakeShared<FArcRmlModelBinding>(UIThread.ToSharedRef(), 0, nullptr, nullptr, MoveTemp(Declarations), FString(),
			ArcRmlUITestHelpers::MakePath(TEXT("Preview/Read.rml")));
		ASSERT_THAT(IsTrue(Binding->DeclaresPreviewModels()));
		ASSERT_THAT(IsTrue(Binding->ReadsPreviewFile(ArcRmlUITestHelpers::MakePath(TEXT("Preview/Read.json")))));
		ASSERT_THAT(IsFalse(Binding->ReadsPreviewFile(ArcRmlUITestHelpers::MakePath(TEXT("Preview/Other.json")))));
	}
};
