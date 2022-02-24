﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

using UnrealBuildTool;

public class LGUIEditor : ModuleRules
{
	public LGUIEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string EnginSourceFolder = EngineDirectory + "/Source/";
        PrivateIncludePaths.AddRange(
                new string[] {
                    EnginSourceFolder + "/Editor/DetailCustomizations/Private",
                });

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Slate",
                "SlateCore",
                "Engine",
                "UnrealEd",
                "PropertyEditor",
                "RenderCore",
                "RHI",
                "LGUI",
                "LevelEditor",
                "Projects",
                "EditorWidgets",
                "DesktopPlatform",//file system
                "ImageWrapper",//texture load
                "InputCore",//STableRow
                "Json",//json
                "JsonUtilities",//json
                "AssetTools",//Asset editor
                "KismetWidgets",
                "ContentBrowser",//LGUI editor
                "SceneOutliner",//LGUIPrefab editor, extend SceneOutliner
                "ApplicationCore",//ClipboardCopy
                "BlueprintGraph",//K2Node
                "KismetCompiler",
                "AppFramework",
                //"AssetRegistry",
                //"InputCore",
				// ... add other public dependencies that you statically link with here ...
                
                "Kismet",
                "ToolMenus",//PrefabEditor
                "Sequencer",
				"MovieScene",
				"MovieSceneTracks",
				"ActorPickerMode",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "EditorStyle",
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
