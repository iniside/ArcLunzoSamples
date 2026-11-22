// Copyright 2026 Lukasz Baran. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArcRmlUIGameFeatureTest : ModuleRules
{
	public ArcRmlUIGameFeatureTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.GetFullPath(ModuleDirectory));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ModelViewViewModel",
				"FieldNotification"
			}
		);
	}
}
