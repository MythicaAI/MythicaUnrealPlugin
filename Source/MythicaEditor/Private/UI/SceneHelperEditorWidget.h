// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorUtilityWidget.h"

#include "SceneHelperEditorWidget.generated.h"

USTRUCT(BlueprintType)
struct FSceneHelperStats
{

    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
    int32 ComponentCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
    int32 ActorCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jobs")
    int32 ActiveJobsCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jobs")
    int32 FinishedJobsCount = 0;

};

class UEditorUtilityScrollBox;
class USceneHelperEntryEditorWidget;

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

    // Start - Native Overrides
    /** Working in "Native" calls in generally more useful than the C++ standards for UserWidgets */

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // End - Native Overrides

    UFUNCTION()
    void OnJobCreated(int RequestId, const FString& ComponentId);

    UFUNCTION()
    void OnJobStateChanged(int RequestId, EMythicaJobState State, FText Message);

    UFUNCTION()
    void OnGenAssetCreated(int RequestId);

protected:

    virtual void NativePoll();

    UFUNCTION(BlueprintCallable)
    virtual USceneHelperEntryEditorWidget* CreateSceneEntry(class UMythicaComponent* OwningComponent);

protected:

    // Start - Widget Bindings

    /** A widget that houses all of the SceneHelperEntryEditorWidgets displayed in the scroll box. It should display all mythica objects in the open scene that are integrated with the job system. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets", meta = (BindWidget))
    TObjectPtr<UEditorUtilityScrollBox> MythicaSceneObjectsList = nullptr;

    // End - Widget Bindings

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<USceneHelperEntryEditorWidget> EntryClassToSpawn = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    FSceneHelperStats Stats = FSceneHelperStats();

    /** How often(sec) the UI will poll for updated scene information. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polling")
    float PollFrequency = 1.0f;

private:

    /** The time(sec) since we last polled scene information. */
    UPROPERTY(Transient)
    float ElapsedSinceLastPoll = 0.0f;

    /** A map of all Component GUIDs to their component counterparts */
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<USceneHelperEntryEditorWidget>> TrackedComponentWidgets = TMap<FString, TObjectPtr<USceneHelperEntryEditorWidget>>();

};
