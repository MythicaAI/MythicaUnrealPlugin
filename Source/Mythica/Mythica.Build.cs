// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Mythica : ModuleRules
{
    public Mythica(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
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
                "AssetTools",
                "Blutility",
                "Boost",
                "CoreUObject",
                "DeveloperSettings",
                "EditorSubsystem",
                "Engine",
                "FileUtilities",
                "HTTP",
                "ImageCore",
                "InputCore",
                "Json",
                "Landscape",
                "MaterialBaking",
                "Projects",
                "PythonScriptPlugin",
                "Slate",
                "SlateCore",
                "ToolWidgets",
                "UMGEditor",
                "UnrealEd",
                "UnrealUSDWrapper",
                "USDClasses",
                "USDExporter",
                "USDStageImporter",
                "USDUtilities"
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
