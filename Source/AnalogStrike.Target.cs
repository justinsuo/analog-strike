using UnrealBuildTool;

public class AnalogStrikeTarget : TargetRules
{
    public AnalogStrikeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("AnalogStrike");
    }
}
