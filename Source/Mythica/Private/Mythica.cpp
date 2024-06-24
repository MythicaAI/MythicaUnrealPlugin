// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythica.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FMythicaModule"

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
}

void FMythicaModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FMythicaModule::AddMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString("Mythica Package Manager"),
		FText::FromString("Opens the Mythica Package Manager"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FMythicaModule::OnMenuClick))
	);
}

void FMythicaModule::OnMenuClick()
{
	UE_LOG(LogTemp, Warning, TEXT("Mythica menu pressed"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMythicaModule, Mythica)