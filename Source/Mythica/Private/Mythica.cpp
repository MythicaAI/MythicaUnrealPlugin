// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythica.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"
#include "MythicaParametersDetails.h"
#include "MythicaJobDefinitionIdDetails.h"

#define LOCTEXT_NAMESPACE "FMythicaModule"

#define PACKAGE_MANAGER_WIDGET_ASSET TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager")
#define ASSET_GENERATOR_WIDGET_ASSET TEXT("/Mythica/UI/WBP_AssetGenerator.WBP_AssetGenerator")

void FMythicaModule::StartupModule()
{
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuExtension(
		"GetContent",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FMythicaModule::AddMenu));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"MythicaParameters",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMythicaParametersDetails::MakeInstance)
	);
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"MythicaJobDefinitionId",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMythicaJobDefinitionIdDetails::MakeInstance)
	);
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FMythicaModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("MythicaParameters");
		PropertyModule.UnregisterCustomPropertyTypeLayout("MythicaJobDefinitionId");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FMythicaModule::AddMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString("Mythica Package Manager"),
		FText::FromString("Opens the Mythica Package Manager"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FMythicaModule::OpenPackageManager))
	);
	MenuBuilder.AddMenuEntry(
		FText::FromString("Mythica Asset Generator"),
		FText::FromString("Opens the Mythica Asset Generator Tool"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FMythicaModule::OpenAssetGenerator))
	);
}

void FMythicaModule::OpenPackageManager()
{
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, PACKAGE_MANAGER_WIDGET_ASSET, NULL, LOAD_None, NULL);
	EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

void FMythicaModule::OpenAssetGenerator()
{
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, ASSET_GENERATOR_WIDGET_ASSET, NULL, LOAD_None, NULL);
	EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMythicaModule, Mythica)