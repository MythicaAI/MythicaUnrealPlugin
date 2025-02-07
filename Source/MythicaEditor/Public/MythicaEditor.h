// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythicaEditor, Log, All);

class FMythicaEditorModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** This is stored on the module so that it can be accessed by other UI */
    void OpenPackageManager();

private:

    /** This is called on the module so that we register the editor windows with it */
    void RegisterEditorMenus();

};
