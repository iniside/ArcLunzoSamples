// Copyright 2026 Lukasz Baran. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArcRmlUISamples : ModuleRules
{
	public ArcRmlUISamples(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.GetFullPath(ModuleDirectory));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ArcRmlUIData",
				"ModelViewViewModel",
				"FieldNotification"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ArcRmlUI",
				"InputCore",
				"Slate",
				"SlateCore",
				"UMG"
			}
		);
	}
}
