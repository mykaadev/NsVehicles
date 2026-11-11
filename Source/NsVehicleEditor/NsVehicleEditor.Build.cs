// Copyright (C) 2025 nulled.softworks. All rights reserved.

using UnrealBuildTool;

public class NsVehicleEditor : ModuleRules
{
    public NsVehicleEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "DeveloperSettings",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
            }
        );
    }
}
