// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

<<<<<<<< HEAD:Source/MythicaRuntime/Public/MythicaRuntime.h
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMythicaRuntimeModule : public IModuleInterface
========
#include "Modules/ModuleManager.h"

class FMythicaEditorModule : public IModuleInterface
>>>>>>>> b177ecb56632918db3d991d0e76477fd695ee592:Source/MythicaEditor/Public/MythicaEditor.h
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

<<<<<<<< HEAD:Source/MythicaRuntime/Public/MythicaRuntime.h
========
    void OpenPackageManager();

private:
    void AddMenu(FMenuBuilder& MenuBuilder);
>>>>>>>> b177ecb56632918db3d991d0e76477fd695ee592:Source/MythicaEditor/Public/MythicaEditor.h
};
