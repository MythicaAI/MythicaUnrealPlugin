// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorUtilityWidget.h"
#include "SceneHelperEditorWidget.generated.h"

/**
 * Scene Helper Editor Widget
 *
 * 
 */
UCLASS()
class USceneHelperEditorWidget : public UEditorUtilityWidget
{

    GENERATED_BODY()

public:

    /** Working in Native calls in generally more useful than the C++ standard constructors */
    //virtual void NativeOnInitialized();
    //virtual void NativePreConstruct();
    virtual void NativeConstruct();
    virtual void NativeDestruct();
    //virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);


};
