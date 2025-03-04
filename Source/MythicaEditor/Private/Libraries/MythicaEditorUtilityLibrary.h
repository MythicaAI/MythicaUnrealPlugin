// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MythicaEditorUtilityLibrary.generated.h"

#define PACKAGE_MANAGER_WIDGET_ASSET TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager")

/**
 * Mythica Editor Utility Library
 *
 * This library is used for any common editor utility function wrappers needed by Mythica.
 */
UCLASS()
class UMythicaEditorUtilityLibrary : public UBlueprintFunctionLibrary
{

    GENERATED_BODY()

public:

    static void OpenEditorUtilityWidgetAsTab(UObject* Outer, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr, const FLinkerInstancingContext* InstancingContext = nullptr);

    static void OpenEditorUtilityWidgetAsTab(FSoftObjectPath WidgetToLoad, FUObjectSerializeContext* Context = nullptr);

    static void OpenPackageManager();

    static void OpenJobDirector();

public:

    static FSoftObjectPath s_PackageManagerWidgetPath;

    static FSoftObjectPath s_JobDirectorWidgetPath;

};
