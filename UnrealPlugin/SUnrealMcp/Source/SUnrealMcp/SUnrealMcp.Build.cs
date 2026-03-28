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
                "DeveloperSettings",
                "Json",
                "JsonUtilities",
                "Networking",
                "Sockets",
                "UnrealEd"
            });
    }
}
