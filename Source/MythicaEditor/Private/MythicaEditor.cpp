// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicaEditor.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "LevelEditor.h"
#include "MythicaComponentDetails.h"
#include "MythicaParametersDetails.h"

#define LOCTEXT_NAMESPACE "FMythicaEditorModule"

#define PACKAGE_MANAGER_WIDGET_ASSET TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager")

void FMythicaEditorModule::StartupModule()
{
    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
    MenuExtender->AddMenuExtension(
        "GetContent",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateRaw(this, &FMythicaEditorModule::AddMenu));

    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(
        "MythicaComponent", 
        FOnGetDetailCustomizationInstance::CreateStatic(&FMythicaComponentDetails::MakeInstance)
    );
    PropertyModule.RegisterCustomPropertyTypeLayout(
        "MythicaParameters",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMythicaParametersDetails::MakeInstance)
    );
    PropertyModule.NotifyCustomizationModuleChanged();
}

void FMythicaEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout("MythicaComponent");
        PropertyModule.UnregisterCustomPropertyTypeLayout("MythicaParameters");
        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

void FMythicaEditorModule::AddMenu(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(
        FText::FromString("Mythica Package Manager"),
        FText::FromString("Opens the Mythica Package Manager"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FMythicaEditorModule::OpenPackageManager))
    );
}

void FMythicaEditorModule::OpenPackageManager()
{
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, PACKAGE_MANAGER_WIDGET_ASSET, NULL, LOAD_None, NULL);
    EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

IMPLEMENT_MODULE(FMythicaEditorModule, MythicaEditor)

#undef LOCTEXT_NAMESPACE
