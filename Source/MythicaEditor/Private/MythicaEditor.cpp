// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicaEditor.h"

#include "Editor/UnrealEdEngine.h"
#include "LevelEditor.h"
#include "Libraries/MythicaEditorUtilityLibrary.h"
#include "MythicaEditorStyle.h"
#include "MythicaComponentDetails.h"
#include "MythicaParametersDetails.h"
#include "PropertyEditorModule.h"
#include "UnrealEdGlobals.h"
#include "ToolMenu.h"
#include "ToolMenus.h"

#include "MythicaEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE MYTHICA_LOCTEXT_NAMESPACE

DEFINE_LOG_CATEGORY(LogMythicaEditor)

static bool HasPlayWorld()
{
    return GEditor->PlayWorld != nullptr;
}

static bool HasNoPlayWorld()
{
    return !HasPlayWorld();
}

static bool HasPlayWorldAndRunning()
{
    return HasPlayWorld() && !GUnrealEd->PlayWorld->bDebugPauseExecution;
}

static void OpenMythicaPackageManager_Clicked()
{
    UMythicaEditorUtilityLibrary::OpenPackageManager();
}

static void OpenMythicaSceneHelper_Clicked()
{
    UMythicaEditorUtilityLibrary::OpenSceneHelper();
}

static TSharedRef<SWidget> GetMythicaHubDropDown()
{
    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection("Windows", LOCTEXT("HubWindowSectionText", "Windows"));

    MenuBuilder.AddMenuEntry(
        LOCTEXT("OpenMythicaPackageManagerButtonShort", "Package Manager"),
        LOCTEXT("OpenMythicaPackageManagerDescription", "Opens the Mythica Package Manager."),
        FSlateIcon(FMythicaEditorStyle::GetStyleSetName(), "MythicaEditor.MythicaLogo"),
        FUIAction(
            FExecuteAction::CreateStatic(&OpenMythicaPackageManager_Clicked),
            FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
            FIsActionChecked(),
            FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
        )
    );

    MenuBuilder.AddMenuEntry(
        LOCTEXT("OpenMythicaSceneHelperButtonShort", "Scene Helper"),
        LOCTEXT("OpenMythicaSceneHelperDescription", "An editor tab that displays a list of all MythicaComponents in the current level with their latest jobs status and some quick actions."),
        FSlateIcon(FMythicaEditorStyle::GetStyleSetName(), "MythicaEditor.MythicaLogo"),
        FUIAction(
            FExecuteAction::CreateStatic(&OpenMythicaSceneHelper_Clicked),
            FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
            FIsActionChecked(),
            FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
        )
    );

    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}

void FMythicaEditorModule::StartupModule()
{
    FMythicaEditorStyle::Initialize();
    FMythicaEditorStyle::ReloadTextures();

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMythicaEditorModule::RegisterEditorMenus));

    // Setting up custom property editors
    {
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
}

void FMythicaEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);

    UToolMenus::UnregisterOwner(this);

    FMythicaEditorStyle::Shutdown();

    // Removing custom property editors
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout("MythicaComponent");
        PropertyModule.UnregisterCustomPropertyTypeLayout("MythicaParameters");
        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

void FMythicaEditorModule::RegisterEditorMenus()
{
    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);

    // Note: Liam - If you need to view what you want to extend use `ToolMenus.Edit 1`
    
    // Extending the Asset Tool Bar to add a quick menu and hub option
    {
        UToolMenu* AssetToolBarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AssetsToolBar");
        FToolMenuSection& ContentSection = AssetToolBarMenu->FindOrAddSection("Content");

        FToolMenuEntry OpenMythicaHubEntry = FToolMenuEntry::InitComboButton(
            "OpenMythicaHUB",
            FUIAction(
                FExecuteAction::CreateStatic(&UMythicaEditorUtilityLibrary::OpenPackageManager),
                FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
                FIsActionChecked(),
                FIsActionButtonVisible()
            ),
            FOnGetContent::CreateStatic(&GetMythicaHubDropDown),
            LOCTEXT("OpenMythicaHubButton", "Mythica HUB"),
            LOCTEXT("OpenMythicaHubDescription", "Opens the Mythica HUB. Where you can find all plugin information, documentation, and tweak settings in one window."),
            FSlateIcon(FMythicaEditorStyle::GetStyleSetName(), "MythicaEditor.MythicaLogo")
        );
        ContentSection.AddEntry(OpenMythicaHubEntry);
    }

    // Add an extension to the Main Menu > Windows drop down
    {
        UToolMenu* MainMenuWindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        FToolMenuSection& GetContentSection = MainMenuWindowMenu->FindOrAddSection("GetContent");

        FToolMenuEntry OpenPackageManagerEntry = FToolMenuEntry::InitMenuEntry(
            "OpenPackageManager",
            LOCTEXT("OpenMythicaPackageManagerButton", "Mythica Package Manager"),
            LOCTEXT("OpenMythicaPackageManagerDescription", "Opens the Mythica Package Manager."),
            FSlateIcon(FMythicaEditorStyle::GetStyleSetName(), "MythicaEditor.MythicaLogo"),
            FUIAction(
                FExecuteAction::CreateStatic(&OpenMythicaPackageManager_Clicked),
                FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
                FIsActionChecked(),
                FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
            )
        );
        GetContentSection.AddEntry(OpenPackageManagerEntry);

        FToolMenuEntry OpenSceneHelperEntry = FToolMenuEntry::InitMenuEntry(
            "OpenSceneHelper",
            LOCTEXT("OpenMythicaSceneHelperButton", "Mythica Scene Helper"),
            LOCTEXT("OpenMythicaSceneHelperDescription", "An editor tab that displays a list of all MythicaComponents in the current level with their latest jobs status and some quick actions."),
            FSlateIcon(FMythicaEditorStyle::GetStyleSetName(), "MythicaEditor.MythicaLogo"),
            FUIAction(
                FExecuteAction::CreateStatic(&OpenMythicaSceneHelper_Clicked),
                FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
                FIsActionChecked(),
                FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
            )
        );
        GetContentSection.AddEntry(OpenSceneHelperEntry);
    }
}

IMPLEMENT_MODULE(FMythicaEditorModule, MythicaEditor)

#undef LOCTEXT_NAMESPACE
