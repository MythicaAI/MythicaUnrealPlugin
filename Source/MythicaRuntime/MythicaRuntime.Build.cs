// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MythicaRuntime : ModuleRules
{
    public MythicaRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                //"MythicaRuntime"
            }
        );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
        );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UMG"
                // ... add other public dependencies that you statically link with here ...
            }
        );
            
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AssetRegistry",
                "CoreUObject",
                "DeveloperSettings",
                "Projects",
                "Slate",
                "SlateCore"
                // ... add private dependencies that you statically link with here ...    
            }
        );
        
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );
    }
}
