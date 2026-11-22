// Copyright 2026 Lukasz Baran. All Rights Reserved.

using UnrealBuildTool;

public class ArcRmlUITest : ModuleRules
{
	public ArcRmlUITest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
			"InputCore",
			"ApplicationCore",
			"SlateCore",
			"Slate",
			"CommonInput",
			"CommonUI",
			"UMG",
			"DeveloperSettings",
			"EnhancedInput",
			"GameplayTags",
			"AssetRegistry",
			"Projects",
			"Json",
			"CQTest",
			"ArcRmlUI",
			"ArcRmlUIShaders",
			"ArcRmlUIData",
			"ArcRmlUiCore",
			"TraceLog",
			"ModelViewViewModel",
			"FieldNotification"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("ArcRmlUIEditor");
			PrivateDependencyModuleNames.Add("PropertyEditor");
			PrivateDependencyModuleNames.Add("BlueprintGraph");
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
