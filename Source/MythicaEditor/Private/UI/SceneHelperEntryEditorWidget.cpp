// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SceneHelperEntryEditorWidget.h"

#include "MythicaComponent.h"
//#include "MythicaEditorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SceneHelperEntryEditorWidget)

void USceneHelperEntryEditorWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void USceneHelperEntryEditorWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

bool USceneHelperEntryEditorWidget::IsOwningComponentValid() const
{
    return OwningComponentRef.IsValid();
}

void USceneHelperEntryEditorWidget::SetOwningComponent(UMythicaComponent* ComponentRef)
{
    OwningComponentRef = ComponentRef;
}

void USceneHelperEntryEditorWidget::NativePostCreate()
{
    OnPostCreate();
}

void USceneHelperEntryEditorWidget::NativeJobCreated(int RequestId)
{
    OnJobCreated(RequestId);
}

void USceneHelperEntryEditorWidget::NativeJobStateUpdated(int RequestId, EMythicaJobState State, FText Message)
{
    OnJobStateUpdated(RequestId, State, Message);
}
