// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicaEditorStyle.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FMythicaEditorStyle::StyleInstance = nullptr;

void FMythicaEditorStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FMythicaEditorStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

void FMythicaEditorStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

const ISlateStyle& FMythicaEditorStyle::Get()
{
    return *StyleInstance;
}

FName FMythicaEditorStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("MythicaEditorStyle"));
    return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);
const FVector2D Icon64x64(64.0f, 64.0f);
const FVector2D Icon128x128(128.0f, 128.0f);

TSharedRef<class FSlateStyleSet> FMythicaEditorStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("Mythica")->GetBaseDir() / TEXT("Resources"));
    Style->Set("MythicaEditor.MythicaLogo", new IMAGE_BRUSH(TEXT("Icon128"), Icon128x128));

    return Style;
}
