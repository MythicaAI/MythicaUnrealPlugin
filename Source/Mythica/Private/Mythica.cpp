// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythica.h"
#include "MythicaParametersDetails.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FMythicaModule"

#define PACKAGE_MANAGER_WIDGET_ASSET TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager")

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
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FMythicaModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("FMythicaParameters");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FMythicaModule::AddMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString("Mythica Package Manager"),
		FText::FromString("Opens the Mythica Package Manager"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FMythicaModule::OnMenuItemClick))
	);
}

void FMythicaModule::OnMenuItemClick()
{
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, PACKAGE_MANAGER_WIDGET_ASSET, NULL, LOAD_None, NULL);
	EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMythicaModule, Mythica)