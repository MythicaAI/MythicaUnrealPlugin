#include "MythicaComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"

UMythicaComponent::UMythicaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMythicaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (RequestId > 0)
    {
        UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
        MythicaEditorSubsystem->OnJobStateChange.RemoveDynamic(this, &UMythicaComponent::OnJobStateChanged);
    }
}

void UMythicaComponent::RegenerateMesh()
{
    if (RequestId > 0)
    {
        return;
    }

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    RequestId = MythicaEditorSubsystem->ExecuteJob(JobDefId, Inputs, Parameters, MaterialParameters, "GeneratedMesh");

    if (RequestId > 0)
    {
        MythicaEditorSubsystem->OnJobStateChange.AddDynamic(this, &UMythicaComponent::OnJobStateChanged);
    }
}

void UMythicaComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, JobDefId))
    {
        OnJobDefIdChanged();
    }
}

void UMythicaComponent::OnJobDefIdChanged()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();

    FMythicaJobDefinition Definition = MythicaEditorSubsystem->GetJobDefinitionById(JobDefId);
    Inputs = Definition.Inputs;
    Parameters = Definition.Parameters;
    MaterialParameters = FMythicaMaterialParameters();
}

void UMythicaComponent::OnJobStateChanged(int InRequestId, EMythicaJobState State)
{
    if (InRequestId != RequestId)
    {
        return;
    }

    if (State != EMythicaJobState::Completed && State != EMythicaJobState::Failed)
    {
        return;
    }

    if (State == EMythicaJobState::Completed)
    {
        UpdateMesh();
    }

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    MythicaEditorSubsystem->OnJobStateChange.RemoveDynamic(this, &UMythicaComponent::OnJobStateChanged);
    RequestId = -1;
}

void UMythicaComponent::UpdateMesh()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    FString ImportDirectory = MythicaEditorSubsystem->GetImportDirectory(RequestId);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(*ImportDirectory, Assets, true, false);

    AActor* OwnerActor = GetOwner();
    USceneComponent* ParentComponent = GetAttachParent();

    for (FAssetData Asset : Assets)
    {
        if (Asset.IsInstanceOf(UStaticMesh::StaticClass()))
        {
            UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(OwnerActor);
            StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(Asset.GetAsset()));
            StaticMeshComponent->AttachToComponent(ParentComponent, FAttachmentTransformRules::KeepRelativeTransform);

            OwnerActor->AddInstanceComponent(StaticMeshComponent);
            StaticMeshComponent->RegisterComponent();
        }
    }
}
