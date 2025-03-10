// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneHelperEntryEditorWidget.h"

#include "Kismet\KismetSystemLibrary.h"
#include "MythicaComponent.h"
#include "MythicaEditorSubsystem.h"

#include "MythicaEditorPrivatePCH.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SceneHelperEntryEditorWidget)

USceneHelperEntryEditorWidget::USceneHelperEntryEditorWidget(const FObjectInitializer& Initializer)
    : Super(Initializer)
{
    bThumbnailRequiresReset = false;
}

void USceneHelperEntryEditorWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void USceneHelperEntryEditorWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void USceneHelperEntryEditorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    if (HasJobHistory() && CurrentJobData.State > EMythicaJobState::Invalid && CurrentJobData.State < EMythicaJobState::Completed)
    {
        CacheCurrentJobData();
    }

    Super::NativeTick(MyGeometry, InDeltaTime);
}

bool USceneHelperEntryEditorWidget::IsOwningComponentValid() const
{
    return OwningComponentRef.IsValid();
}

void USceneHelperEntryEditorWidget::SetOwningComponent(UMythicaComponent* ComponentRef)
{
    OwningComponentRef = ComponentRef;
}

FString USceneHelperEntryEditorWidget::GetComponentId() const
{
    ensure(OwningComponentRef.IsValid());
    return UKismetSystemLibrary::GetDisplayName(OwningComponentRef.Get());
}

void USceneHelperEntryEditorWidget::NativePostCreate()
{
    CacheLatestJobRequest();

    OnPostCreate();
}

void USceneHelperEntryEditorWidget::NativeJobCreated(int RequestId)
{
    CurrentRequestId = RequestId;

    CacheCurrentJobData();

    OnJobCreated();
}

void USceneHelperEntryEditorWidget::NativeJobStateUpdated(int RequestId, EMythicaJobState State, FText Message)
{
    if (State == EMythicaJobState::Completed || State == EMythicaJobState::Failed)
    {
        CacheCurrentJobData();
    }

    OnJobStateUpdated(State, Message);
}

void USceneHelperEntryEditorWidget::NativeGenAssetCreated(int RequestId)
{
    if (CurrentRequestId != RequestId)
    {
        return;
    }

    CacheCurrentJobData();
    CacheGenMeshAssetData();

    OnGenAssetCreated();
}

void USceneHelperEntryEditorWidget::CacheLatestJobRequest()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    TMap<FString, FMythicaRequestIdList> JobsToCompsMap = MythicaEditorSubsystem->GetJobsToComponentList();
    FMythicaRequestIdList* RequestList = JobsToCompsMap.Find(GetComponentId());

    if (RequestList == nullptr || RequestList->RequestIds.IsEmpty())
    {
        // There is no job history so we can early out
        CurrentRequestId = -1;
        return;
    }

    CurrentRequestId = RequestList->RequestIds[0];

    CacheCurrentJobData();
    CacheGenMeshAssetData();
}

void USceneHelperEntryEditorWidget::CacheCurrentJobData()
{
    if (!HasJobHistory())
    {
        // This shouldnt be getting called when we dont have a job history
        return;
    }

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    TMap<int, FMythicaJob> Jobs = MythicaEditorSubsystem->GetActiveJobsList();
    CurrentJobData = Jobs.FindChecked(CurrentRequestId);
}

void USceneHelperEntryEditorWidget::CacheGenMeshAssetData()
{
    if (!HasJobHistory())
    {
        return;
    }

    if (!CurrentJobData.CreatedMeshData.IsValid())
    {
        // Maybe the mesh data has not been created yet, so we wait
        return;
    }

    if (GenMeshAssetDataRef == CurrentJobData.CreatedMeshData)
    {
        // They are the same no need to update
        return;
    }

    GenMeshAssetDataRef = CurrentJobData.CreatedMeshData;

    bThumbnailRequiresReset = true;
}
