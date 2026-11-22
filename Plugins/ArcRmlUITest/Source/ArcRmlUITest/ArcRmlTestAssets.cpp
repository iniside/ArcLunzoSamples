// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlTestAssets.h"

#include "ArcRmlUIModule.h"
#include "ArcRmlUITestHelpers.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Localization/ArcRmlStringSync.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace ArcRmlTestAssetsPrivate
{
	struct FLocEntry
	{
		const TCHAR* Key;
		const TCHAR* Text;
	};

	const FLocEntry LocTestEntries[] = {
		{TEXT("Title"), TEXT("Loc Test title [ST]")},
		{TEXT("Heading"), TEXT("Heading from table")},
		{TEXT("Unequip"), TEXT("Unequip [ST]")},
		{TEXT("SelfClosing"), TEXT("Self-closing from table")},
		{TEXT("InsideIf"), TEXT("Inside data-if from table")},
		{TEXT("IfText"), TEXT("data-if text from table")},
		{TEXT("Section"), TEXT("Section from table")},
		{TEXT("ButtonLabel"), TEXT("Button from table")},
		{TEXT("AreaText"), TEXT("Text area from table")},
	};

	const TCHAR* const LocTestDocuments[] = {
		TEXT("LocLoaded/LocTest.rml"),
		TEXT("LocMissing/LocTest.rml"),
		TEXT("LocTest.rml"),
	};

	bool IsLocTestKey(const FString& Key)
	{
		for (const FLocEntry& Entry : LocTestEntries)
		{
			if (Key == Entry.Key)
			{
				return true;
			}
		}
		return false;
	}

	bool SaveTable(UStringTable& Table)
	{
		UPackage* Package = Table.GetPackage();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, &Table, *Filename, Args);
	}

	FString EnsureLocTestTable(const FString& DocumentPath, int32& InOutSavedTables)
	{
		TArray<FString> Paths;
		Paths.Add(DocumentPath);
		FArcRmlStringSyncReport Report = ArcRmlStringSync::Collect(FArcRmlUIModule::Get().GetContentRoots(), Paths);
		if (Report.Files.Num() != 1 || Report.Files[0].TableObjectPath.IsEmpty())
		{
			return FString::Printf(TEXT("%s names no string table"), *DocumentPath);
		}
		const FString TableObjectPath = Report.Files[0].TableObjectPath;
		bool bChanged = false;
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(TableObjectPath)))
		{
			ArcRmlStringSync::Write(Report);
			if (!Report.Files[0].bTableCreated)
			{
				return FString::Printf(TEXT("the string sync of %s did not create %s"), *DocumentPath, *TableObjectPath);
			}
			bChanged = true;
		}
		UStringTable* Table = LoadObject<UStringTable>(nullptr, *TableObjectPath);
		if (!Table)
		{
			return FString::Printf(TEXT("%s could not be loaded"), *TableObjectPath);
		}

		const FStringTableRef Mutable = Table->GetMutableStringTable();
		TArray<FString> Unexpected;
		Mutable->EnumerateSourceStrings([&Unexpected](const FString& Key, const FString& SourceString) -> bool
		{
			if (!IsLocTestKey(Key))
			{
				Unexpected.Add(Key);
			}
			return true;
		});
		for (const FString& Key : Unexpected)
		{
			Mutable->RemoveSourceString(FTextKey(Key));
			bChanged = true;
		}
		for (const FLocEntry& Entry : LocTestEntries)
		{
			FString Existing;
			if (!Mutable->GetSourceString(FTextKey(Entry.Key), Existing) || Existing != Entry.Text)
			{
				Mutable->SetSourceString(FTextKey(Entry.Key), Entry.Text, FString());
				bChanged = true;
			}
		}
		if (!bChanged)
		{
			return FString();
		}
		if (!SaveTable(*Table))
		{
			return FString::Printf(TEXT("%s could not be saved"), *TableObjectPath);
		}
		++InOutSavedTables;
		return FString();
	}
}

FString UArcRmlTestAssetLibrary::CreateLocTestTables(int32& OutSavedTables)
{
	OutSavedTables = 0;
	for (const TCHAR* const Document : ArcRmlTestAssetsPrivate::LocTestDocuments)
	{
		const FString Error = ArcRmlTestAssetsPrivate::EnsureLocTestTable(ArcRmlUITestHelpers::MakePath(Document), OutSavedTables);
		if (!Error.IsEmpty())
		{
			return Error;
		}
	}
	return FString();
}
