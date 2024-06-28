// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMythicaModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void AddMenu(FMenuBuilder& MenuBuilder);
	void OnMenuItemClick();
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);
	void OnMapChanged(UWorld* InWorld, EMapChangeType InMapChangeType);

	TSharedPtr<SWindow> Window;
};
