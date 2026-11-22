// Copyright 2026 Lukasz Baran. All Rights Reserved.

using UnrealBuildTool;

public class ArcRmlUITestRuntime : ModuleRules
{
	public ArcRmlUITestRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ModelViewViewModel",
			"FieldNotification",
			"ArcRmlUIData",
			"ArcRmlUI"
		});
	}
}
