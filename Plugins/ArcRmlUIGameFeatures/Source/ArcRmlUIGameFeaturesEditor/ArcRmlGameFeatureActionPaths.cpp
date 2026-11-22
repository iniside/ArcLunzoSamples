// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlGameFeatureActionPaths.h"

#include "ArcRmlGameFeatureAction_PackageDocuments.h"
#include "ArcRmlGameFeatureAction_PreloadDocuments.h"
#include "ArcRmlUILog.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Cook/ArcRmlCookPathProviders.h"
#include "GameFeatureAction.h"
#include "GameFeatureData.h"
#include "Interfaces/ArcRmlPaths.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/ITargetPlatform.h"
#include "UObject/ICookInfo.h"

void ArcRmlGameFeatureActionPaths::Read(UE::Cook::ICookInfo& CookInfo, TArray<FArcRmlCookPathEntry>& OutEntries)
{
	TSet<IPlugin*> SessionPlugins;
	for (const ITargetPlatform* Platform : CookInfo.GetSessionPlatforms())
	{
		if (const TSet<IPlugin*>* Enabled = CookInfo.GetEnabledPlugins(Platform))
		{
			SessionPlugins.Append(*Enabled);
		}
	}

	TArray<FAssetData> Assets;
	IAssetRegistry::GetChecked().GetAssetsByClass(UGameFeatureData::StaticClass()->GetClassPathName(), Assets, true);
	Assets.Sort([](const FAssetData& A, const FAssetData& B) -> bool
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});
	for (const FAssetData& Asset : Assets)
	{
		FString MountName;
		FString RelativePath;
		if (!ArcRmlPaths::SplitPackagePath(Asset.PackageName.ToString(), MountName, RelativePath))
		{
			continue;
		}
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(MountName);
		if (!Plugin.IsValid() || !SessionPlugins.Contains(Plugin.Get()))
		{
			continue;
		}
		const UGameFeatureData* Data = Cast<UGameFeatureData>(Asset.GetAsset());
		if (!Data)
		{
			UE_LOGF(LogArcRmlUI, Error, "ArcRmlUI cook: cannot load the game feature data '%ls' to read its RML packaging actions", *Asset.GetObjectPathString());
			continue;
		}
		for (const UGameFeatureAction* Action : Data->GetActions())
		{
			if (const UArcRmlGameFeatureAction_PackageDocuments* PackageAction = Cast<UArcRmlGameFeatureAction_PackageDocuments>(Action))
			{
				FArcRmlCookPathEntry& Entry = OutEntries.AddDefaulted_GetRef();
				Entry.Label = FString::Printf(TEXT("game feature action of plugin '%s' (%s)"), *Plugin->GetName(), *Asset.GetObjectPathString());
				Entry.Plugin = Plugin.Get();
				Entry.PathsToPackage = PackageAction->GetPathsToPackage();
			}
			else if (const UArcRmlGameFeatureAction_PreloadDocuments* PreloadAction = Cast<UArcRmlGameFeatureAction_PreloadDocuments>(Action))
			{
				TArray<FString> Documents;
				for (const FArcRmlDocumentRef& Document : PreloadAction->GetDocuments())
				{
					if (!Document.Path.TrimStartAndEnd().IsEmpty())
					{
						Documents.Add(Document.Path);
					}
				}
				if (!Documents.IsEmpty())
				{
					FArcRmlCookPathEntry& Entry = OutEntries.AddDefaulted_GetRef();
					Entry.Label = FString::Printf(TEXT("game feature preload action of plugin '%s' (%s)"), *Plugin->GetName(), *Asset.GetObjectPathString());
					Entry.Plugin = Plugin.Get();
					Entry.Documents = MoveTemp(Documents);
				}
			}
		}
	}
}
