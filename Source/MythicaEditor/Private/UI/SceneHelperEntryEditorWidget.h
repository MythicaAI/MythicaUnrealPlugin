// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "SceneHelperEntryEditorWidget.generated.h"

/**
 * Scene Helper Entry Editor Widget
 *
 * 
 */
UCLASS()
class USceneHelperEntryEditorWidget : public UEditorUtilityWidget
{

    GENERATED_BODY()

public:

    /** Working in Native calls in generally more useful than the C++ standard constructors */
    virtual void NativeConstruct();
    virtual void NativeDestruct();
};
