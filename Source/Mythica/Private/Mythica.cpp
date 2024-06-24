// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythica.h"

#include "MythicaPackageManagerWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FMythicaModule"

void FMythicaModule::StartupModule()
{
	// Add menu option
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	MenuExtender->AddMenuExtension(
		"GetContent",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FMythicaModule::AddMenu));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	// Add panel type
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("MythicaTab", FOnSpawnTab::CreateRaw(this, &FMythicaModule::SpawnTab))
		.SetDisplayName(FText::FromString("Mythica Packages"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMythicaModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("MythicaTab");
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
	FGlobalTabmanager::Get()->TryInvokeTab(FName("MythicaTab"));
}

TSharedRef<SDockTab> FMythicaModule::SpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::MajorTab)
		[
			SNew(SMythicaPackageManagerWidget)
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMythicaModule, Mythica)