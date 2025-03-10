// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneHelperEditorWidget.h"

#include "EditorUtilityWidgetComponents.h"
#include "Kismet\KismetSystemLibrary.h"
#include "MythicaComponent.h"
#include "MythicaEditor.h"
#include "MythicaEditorSubsystem.h"
#include "SceneHelperEntryEditorWidget.h"
#include "Subsystems/EditorActorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SceneHelperEditorWidget)

void USceneHelperEditorWidget::NativeConstruct()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    MythicaEditorSubsystem->OnJobCreated.AddUniqueDynamic(this, &ThisClass::OnJobCreated);
    MythicaEditorSubsystem->OnJobStateChange.AddUniqueDynamic(this, &ThisClass::OnJobStateChanged);
    MythicaEditorSubsystem->OnGenAssetCreated.AddUniqueDynamic(this, &ThisClass::OnGenAssetCreated);

    // Store initial stats then let even driven take over.
    TArray<FMythicaJob> Jobs;
    MythicaEditorSubsystem->GetActiveJobsList().GenerateValueArray(Jobs);
    for (const FMythicaJob& Job : Jobs)
    {
        if (Job.State >= EMythicaJobState::Completed)
        {
            Stats.FinishedJobsCount++;
        }
        else if (Job.State >= EMythicaJobState::Requesting)
        {
            Stats.ActiveJobsCount++;
        }
    }

    // Run the first poll to construct all widgets
    NativePoll();

    Super::NativeConstruct();
}

void USceneHelperEditorWidget::NativeDestruct()
{
    Super::NativeDestruct();

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    MythicaEditorSubsystem->OnJobCreated.RemoveAll(this);
    MythicaEditorSubsystem->OnJobStateChange.RemoveAll(this);
    MythicaEditorSubsystem->OnGenAssetCreated.RemoveAll(this);

    TrackedComponentWidgets.Empty();
}

void USceneHelperEditorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    ElapsedSinceLastPoll += InDeltaTime;

    if (ElapsedSinceLastPoll >= PollFrequency)
    {
        NativePoll();

        ElapsedSinceLastPoll = 0.0f;
    }

    // I want to update all my data before we fire any blueprint or other tick operations
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void USceneHelperEditorWidget::OnJobCreated(int RequestId, const FString& ComponentId)
{
    TObjectPtr<USceneHelperEntryEditorWidget>* WidgetRefCheck = TrackedComponentWidgets.Find(ComponentId);

    if (WidgetRefCheck == nullptr || !IsValid(*WidgetRefCheck))
    {
        UE_LOG(LogMythicaEditor, Warning, TEXT("There was no Widgets found with the Id of [%s]."), *ComponentId);
        return;
    }

    USceneHelperEntryEditorWidget* WidgetRef = *WidgetRefCheck;
    WidgetRef->NativeJobCreated(RequestId);

    Stats.ActiveJobsCount++;
}

void USceneHelperEditorWidget::OnJobStateChanged(int RequestId, EMythicaJobState State, FText Message)
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    for (const TPair<FString, FMythicaRequestIdList>& RequestCompLink : MythicaEditorSubsystem->GetJobsToComponentList())
    {
        const FMythicaRequestIdList ReqList = RequestCompLink.Value;

        if (!ReqList.RequestIds.Contains(RequestId))
        {
            continue;
        }

        FString CompId = RequestCompLink.Key;

        TObjectPtr<USceneHelperEntryEditorWidget>* WidgetRefCheck = TrackedComponentWidgets.Find(CompId);

        if (WidgetRefCheck == nullptr || !IsValid(*WidgetRefCheck))
        {
            UE_LOG(LogMythicaEditor, Warning, TEXT("There was no Widgets found with the Id of [%s]."), *CompId);
            break;
        }

        USceneHelperEntryEditorWidget* WidgetRef = *WidgetRefCheck;

        WidgetRef->NativeJobStateUpdated(RequestId, State, Message);

        break;
    }

    if (State >= EMythicaJobState::Completed)
    {
        Stats.ActiveJobsCount = FMath::Max(Stats.ActiveJobsCount-1, 0);
        Stats.FinishedJobsCount++;
    }
}

void USceneHelperEditorWidget::OnGenAssetCreated(int RequestId)
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    ensure(MythicaEditorSubsystem);

    for (const TPair<FString, FMythicaRequestIdList>& RequestCompLink : MythicaEditorSubsystem->GetJobsToComponentList())
    {
        const FMythicaRequestIdList ReqList = RequestCompLink.Value;

        if (!ReqList.RequestIds.Contains(RequestId))
        {
            continue;
        }

        FString CompId = RequestCompLink.Key;

        TObjectPtr<USceneHelperEntryEditorWidget>* WidgetRefCheck = TrackedComponentWidgets.Find(CompId);

        if (WidgetRefCheck == nullptr || !IsValid(*WidgetRefCheck))
        {
            UE_LOG(LogMythicaEditor, Warning, TEXT("There was no Widgets found with the Id of [%s]."), *CompId);
            return;
        }

        USceneHelperEntryEditorWidget* WidgetRef = *WidgetRefCheck;

        WidgetRef->NativeGenAssetCreated(RequestId);
    }
}

void USceneHelperEditorWidget::NativePoll()
{
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    ensure(EditorActorSubsystem);

    TSet<FString> TrackedCompIds = TSet<FString>();
    TSet<AActor*> MythicaActors = TSet<AActor*>();
    TArray<UActorComponent*> LevelComponents = EditorActorSubsystem->GetAllLevelActorsComponents();
    for (UActorComponent* Component : LevelComponents)
    {
        UMythicaComponent* MythComp = Cast<UMythicaComponent>(Component);
        if (!IsValid(MythComp))
        {
            continue;
        }

        // Storing the unique parent actors for stats
        MythicaActors.Emplace(MythComp->GetOwner());

        FString DisplayName = UKismetSystemLibrary::GetDisplayName(MythComp);
        TrackedCompIds.Emplace(DisplayName);

        TObjectPtr<USceneHelperEntryEditorWidget>* WidgetTmp = TrackedComponentWidgets.Find(DisplayName);
        if (WidgetTmp != nullptr && IsValid(*WidgetTmp))
        {
            USceneHelperEntryEditorWidget* Widget = *WidgetTmp;

            if (!Widget->IsOwningComponentValid())
            {
                Widget->SetOwningComponent(MythComp);
            }
        }
        else
        {
            CreateSceneEntry(MythComp);
        }
    }

    TSet<FString> Keys;
    TrackedComponentWidgets.GetKeys(Keys);
    TSet<FString> RemovedKeys = Keys.Difference(TrackedCompIds);
    for (FString Key : RemovedKeys)
    {
        TObjectPtr<USceneHelperEntryEditorWidget> Widget;
        TrackedComponentWidgets.RemoveAndCopyValue(Key, Widget);
        MythicaSceneObjectsList->RemoveChild(Widget);
    }

    // Updating our stats
    Stats.ActorCount = MythicaActors.Num();
    Stats.ComponentCount = TrackedComponentWidgets.Num();
}

USceneHelperEntryEditorWidget* USceneHelperEditorWidget::CreateSceneEntry(UMythicaComponent* OwningComponent)
{
    ensure(OwningComponent);

    USceneHelperEntryEditorWidget* NewWidget = CreateWidget<USceneHelperEntryEditorWidget>(this, EntryClassToSpawn);
    ensure(NewWidget);

    // Initialize any pre visibility variables here. Since we cannot inject params pre construction time in c++
    NewWidget->SetOwningComponent(OwningComponent);
    NewWidget->NativePostCreate();

    UPanelSlot* NewSlot = MythicaSceneObjectsList->AddChild(NewWidget);
    ensure(NewSlot);

    // @TODO - Liam: Look into replacing this with Component->UniqueId()
    FString DisplayName = UKismetSystemLibrary::GetDisplayName(OwningComponent);
    TrackedComponentWidgets.Emplace(DisplayName, NewWidget);

    return NewWidget;
}
