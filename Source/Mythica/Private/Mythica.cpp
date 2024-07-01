// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mythica.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "MythicaPackageManagerWidget.h"

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
	LevelEditorModule.OnMapChanged().AddRaw(this, &FMythicaModule::OnMapChanged);
}

void FMythicaModule::ShutdownModule()
{
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
	if (Window.IsValid())
	{
		Window->BringToFront();
	}
	else
	{
		Window = SNew(SWindow)
			.Title(FText::FromString("Mythica Package Manager"))
			.ClientSize(FVector2D(860, 600))
			.MinWidth(860)
			.MinHeight(600)
			.SupportsMinimize(false)
			.SupportsMaximize(false)
			[
				SNew(SMythicaPackageManagerWidget)
			];

		Window->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FMythicaModule::OnWindowClosed));

		FSlateApplication::Get().AddWindow(Window.ToSharedRef());
	}
}

void FMythicaModule::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	Window.Reset();
}

void FMythicaModule::OnMapChanged(UWorld* InWorld, EMapChangeType InMapChangeType)
{
	if (Window.IsValid() && InMapChangeType == EMapChangeType::TearDownWorld)
	{
		FSlateApplication::Get().RequestDestroyWindow(Window.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMythicaModule, Mythica)