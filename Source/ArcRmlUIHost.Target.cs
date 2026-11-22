// Copyright 2026 Lukasz Baran. All Rights Reserved.

using UnrealBuildTool;

public class ArcRmlUIHostTarget : TargetRules
{
	public ArcRmlUIHostTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("ArcRmlUIHost");
	}
}
