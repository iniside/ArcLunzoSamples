// Copyright 2026 Lukasz Baran. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ArcRmlUIGameFeatures : ModuleRules
{
	public ArcRmlUIGameFeatures(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(Path.GetFullPath(ModuleDirectory));

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameFeatures",
				"ArcRmlUIData"
			}
		);

		if (Target.Type != TargetType.Server)
		{
			PrivateDependencyModuleNames.Add("ArcRmlUI");
			PrivateDefinitions.Add("ARC_WITH_RMLUI=1");
		}
		else
		{
			PrivateDefinitions.Add("ARC_WITH_RMLUI=0");
		}
	}
}
