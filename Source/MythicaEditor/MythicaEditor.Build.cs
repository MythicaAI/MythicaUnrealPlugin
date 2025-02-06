// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MythicaEditor : ModuleRules
{
    public MythicaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                "MythicaEditor"
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
                "CoreUObject",
                "Engine",
                "EditorFramework",
                "UnrealEd",
                "UMG",
                "MythicaRuntime"
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
                "DeveloperSettings",
                "EditorSubsystem",
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
