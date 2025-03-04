// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

class ISlateStyle;

/**
 * Mythica Editor Style
 * 
 * Slate style used by the Mythica Plugin.
 */
class FMythicaEditorStyle
{
public:

    static void Initialize();

    static void Shutdown();

    /* Reloads textures used by slate renderer */
    static void ReloadTextures();

    static const ISlateStyle& Get();

    static FName GetStyleSetName();

private:

    static TSharedRef<class FSlateStyleSet> Create();

private:

    static TSharedPtr<class FSlateStyleSet> StyleInstance;

};
