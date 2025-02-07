// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/MythicaEditorUtilityLibrary.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"

void UMythicaEditorUtilityLibrary::OpenEditorUtilityWidgetAsTab(UObject* Outer, const TCHAR* Name, const TCHAR* Filename, uint32 LoadFlags, UPackageMap* Sandbox, const FLinkerInstancingContext* InstancingContext)
{
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    ensure(EditorUtilitySubsystem);

    UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(Outer, Name, Filename, LoadFlags, Sandbox, InstancingContext);
    ensure(UtilityWidgetBlueprint);

    EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

void UMythicaEditorUtilityLibrary::OpenPackageManager()
{
    OpenEditorUtilityWidgetAsTab(NULL, PACKAGE_MANAGER_WIDGET_ASSET);
}
