// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "MythicaRuntime"

class FMythicaRuntimeModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override
    {

    }

    virtual void ShutdownModule() override
    {

    }

};


IMPLEMENT_MODULE(FMythicaRuntimeModule, MythicaRuntime)

#undef LOCTEXT_NAMESPACE
