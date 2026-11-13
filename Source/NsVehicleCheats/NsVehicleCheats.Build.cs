// Copyright (C) 2025 nulled.softworks. All rights reserved.

using System.IO;
using System.Linq;
using EpicGames.Core;
using UnrealBuildBase;
using UnrealBuildTool;

public class NsVehicleCheats : ModuleRules
{
    public NsVehicleCheats(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NsVehicle" });
        PrivateDependencyModuleNames.AddRange(new[] { "AssetRegistry" });

        // NsCheats is optional. Link it only when the plugin is present, so NsVehicles compiles alone
        // The search of the engine and the project does not see the additional plugin folders of the project, so a studio plugin next to NsVehicles counts too
        DirectoryReference ProjectDirectory = Target.ProjectFile != null ? Target.ProjectFile.Directory : null;
        bool bWithNsCheats = Plugins.ReadAvailablePlugins(Unreal.EngineDirectory, ProjectDirectory, null).Any(Plugin => Plugin.Name == "NsCheats")
            || File.Exists(Path.Combine(PluginDirectory, "..", "NsCheats", "NsCheats.uplugin"));
        if (bWithNsCheats)
        {
            PrivateDependencyModuleNames.Add("NsCheatsRuntime");
        }
        PublicDefinitions.Add("WITH_NSCHEATS=" + (bWithNsCheats ? "1" : "0"));
    }
}
