// Copyright 2026 Lukasz Baran. All Rights Reserved.

using UnrealBuildTool;

public class ArcRmlUIHostEditorTarget : TargetRules
{
	public ArcRmlUIHostEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("ArcRmlUIHost");
	}
}
