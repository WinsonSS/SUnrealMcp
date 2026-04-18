using UnrealBuildTool;

public class SUnrealMcp : ModuleRules
{
    public SUnrealMcp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "AssetRegistry",
                "BlueprintGraph",
                "DeveloperSettings",
                "EnhancedInput",
                "InputBlueprintNodes",
                "InputCore",
                "Json",
                "JsonUtilities",
                "Networking",
                "Sockets",
                "Slate",
                "SlateCore",
                "UMG",
                "UMGEditor",
                "UnrealEd"
            });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("LiveCoding");
        }
    }
}
