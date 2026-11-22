// Copyright 2026 Lukasz Baran. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArcRmlUIGameFeaturesEditor : ModuleRules
{
	public ArcRmlUIGameFeaturesEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.GetFullPath(ModuleDirectory));

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ArcRmlUI",
				"ArcRmlUIEditor",
				"ArcRmlUIGameFeatures",
				"GameFeatures",
				"AssetRegistry",
				"Projects"
			}
		);
	}
}
