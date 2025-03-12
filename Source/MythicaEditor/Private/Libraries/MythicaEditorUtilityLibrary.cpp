// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/MythicaEditorUtilityLibrary.h"

#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"

#include "MythicaEditorPrivatePCH.h"

FSoftObjectPath UMythicaEditorUtilityLibrary::PackageManagerWidgetPath = FString(TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager"));
FSoftObjectPath UMythicaEditorUtilityLibrary::SceneHelperWidgetPath = FString(TEXT("/Mythica/UI/Scene/EUW_SceneHelper.EUW_SceneHelper"));

void UMythicaEditorUtilityLibrary::OpenEditorUtilityWidgetAsTab(UObject* Outer, const TCHAR* Name, const TCHAR* Filename, uint32 LoadFlags, UPackageMap* Sandbox, const FLinkerInstancingContext* InstancingContext)
{
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    ensure(EditorUtilitySubsystem);

    UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(Outer, Name, Filename, LoadFlags, Sandbox, InstancingContext);
    ensure(UtilityWidgetBlueprint);

    EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

void UMythicaEditorUtilityLibrary::OpenEditorUtilityWidgetAsTab(FSoftObjectPath WidgetToLoad, FUObjectSerializeContext* Context)
{
    UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
    ensure(EditorUtilitySubsystem);

    UEditorUtilityWidgetBlueprint* UtilityWidgetBlueprint = CastChecked<UEditorUtilityWidgetBlueprint>(WidgetToLoad.TryLoad(Context));

    EditorUtilitySubsystem->SpawnAndRegisterTab(UtilityWidgetBlueprint);
}

void UMythicaEditorUtilityLibrary::OpenPackageManager()
{
    OpenEditorUtilityWidgetAsTab(PackageManagerWidgetPath);
}

void UMythicaEditorUtilityLibrary::OpenSceneHelper()
{
    OpenEditorUtilityWidgetAsTab(SceneHelperWidgetPath);
}
