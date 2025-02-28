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

    USceneHelperEntryEditorWidget(const FObjectInitializer& Initializer);

    /** Working in "Native" calls in generally more useful than the C++ standards for UserWidgets */
    virtual void NativeConstruct();
    virtual void NativeDestruct();
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    bool IsOwningComponentValid() const;
    void SetOwningComponent(UMythicaComponent* ComponentRef);

    UFUNCTION(BlueprintPure, BlueprintCosmetic, Category="Mythica|Jobs")
    bool HasJobHistory() const { return CurrentRequestId != -1; }

    FString GetComponentId() const;

    virtual void NativePostCreate();
    virtual void NativeJobCreated(int RequestId);
    virtual void NativeJobStateUpdated(int RequestId, EMythicaJobState State, FText Message);
    virtual void NativeGenAssetCreated(int RequestId);

protected:

    /**
     * Called after the widget is constructed and created. Used to trigger logic after variables have been set in C++.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Init")
    void OnPostCreate();

    /**
     * Forwards the job created call up to the widget if any additional logic needs to be done.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Jobs")
    void OnJobCreated();

    /**
     * Forwards the job state updated call up to the widget if any additional logic needs to be done.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Jobs")
    void OnJobStateUpdated(EMythicaJobState State, const FText& Message);

    /**
     * Forwards the gen asset call up to the widget if any additional logic needs to be done.
     */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Assets")
    void OnGenAssetCreated();

    void CacheLatestJobRequest();
    void CacheCurrentJobData();
    void CacheGenMeshAssetData();

protected:

    UE_DEPRECATED(5.2, "Direct access to bHasJobHistory is deprecated. Please use HasJobHistory() function call instead.")
    /** Used to change UI based on if the stored component has any job history yet. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Init")
    uint8 bHasJobHistory:1;

    /** This lets the SceneHelper widget know if this widget should be reconstructed or not. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Init")
    uint8 bThumbnailRequiresReset:1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    int32 CurrentRequestId = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    FMythicaJob CurrentJobData = FMythicaJob();

    /** A reference to the generated mesh associated to the Mythica Component. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    FAssetData GenMeshAssetDataRef = FAssetData();

    /** A weak object ptr that will get reset by the SceneHelper when it becomes invalid. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
    TWeakObjectPtr<UMythicaComponent> OwningComponentRef = TWeakObjectPtr<UMythicaComponent>();

};
