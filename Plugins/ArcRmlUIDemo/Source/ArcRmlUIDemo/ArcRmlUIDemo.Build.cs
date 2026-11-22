// Copyright 2026 Lukasz Baran. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArcRmlUIDemo : ModuleRules
{
	public ArcRmlUIDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(PluginDirectory, "Source/ArcRmlUIDemo")));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ModelViewViewModel",
				"FieldNotification",
				"GameplayTags",
				"ArcRmlUIData"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CommonInput",
				"DeveloperSettings",
				"EnhancedInput",
				"InputCore",
				"UMG",
				"ArcRmlUI"
			}
		);
	}
}
