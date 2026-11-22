// Copyright 2026 Lukasz Baran. All Rights Reserved.

using UnrealBuildTool;

public class ArcRmlUIDemoEditor : ModuleRules
{
	public ArcRmlUIDemoEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetRegistry",
			"BlueprintGraph",
			"Json",
			"JsonUtilities"
		});
	}
}
