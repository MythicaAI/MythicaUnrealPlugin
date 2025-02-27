// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorUtilityWidget.h"

#include "UObject/WeakObjectPtrTemplates.h"
#include "MythicaEditorSubsystem.h"

#include "SceneHelperEntryEditorWidget.generated.h"

class UMythicaComponent;

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

    /** Working in "Native" calls in generally more useful than the C++ standards for UserWidgets */
    virtual void NativeConstruct();
    virtual void NativeDestruct();

    bool IsOwningComponentValid() const;
    void SetOwningComponent(UMythicaComponent* ComponentRef);

    bool DoesThumbnailRequireRestart() const { return bThumbnailRequiresReset; }

    virtual void NativePostCreate();
    virtual void NativeJobCreated(int RequestId);
    virtual void NativeJobStateUpdated(int RequestId, EMythicaJobState State, FText Message);

    /**
     * Called after the widget is constructed and created. Used to trigger logic after variables have been set in C++.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Init")
    void OnPostCreate();

    /**
     * Forwards the job created call up to the widget if any additional logic needs to be done.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Jobs")
    void OnJobCreated(int RequestId);

    /**
     * Forwards the job state updated call up to the widget if any additional logic needs to be done.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Jobs")
    void OnJobStateUpdated(int RequestId, EMythicaJobState State, const FText& Message);

protected:

    /** Used to change UI based on if the stored component has any job history yet. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Init")
    uint8 bHasJobHistory:1;

    /** This lets the SceneHelper widget know if this widget should be reconstructed or not. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Init")
    uint8 bThumbnailRequiresReset:1;

    /** A reference to the generated mesh associated to the Mythica Component. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    FAssetData CachedGenMeshAssetData = FAssetData();

    /** A weak object ptr that will get reset by the SceneHelper when it becomes invalid. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    TWeakObjectPtr<UMythicaComponent> OwningComponentRef = TWeakObjectPtr<UMythicaComponent>();

};
