#include "MythicaComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"

#define IMPORT_NAME_LENGTH 10

#define PLACEHOLDER_MESH_ASSET TEXT("/Mythica/Placeholder/SM_Placeholder.SM_Placeholder")

struct FMythicaProcessingStep
{
    EMythicaJobState State;
    double EstimatedDuration;
};

const FMythicaProcessingStep ProcessingSteps[] = {
    { EMythicaJobState::Requesting, 0.1f },
    { EMythicaJobState::Queued, 0.1f },
    { EMythicaJobState::Processing, 5.0f },
    { EMythicaJobState::Importing, 0.25f }
};
static_assert((uint8)EMythicaJobState::Completed - (uint8)EMythicaJobState::Invalid == 5);

UMythicaComponent::UMythicaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicaComponent::OnRegister()
{
    Super::OnRegister();

    UpdatePlaceholderMesh();

    if (CanRegenerateMesh())
    {
        BindWorldInputListeners();
        TransformUpdated.AddUObject(this, &UMythicaComponent::OnTransformUpdated);
    }

    if (RequestId > 0)
    {
        UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();

        State = MythicaEditorSubsystem->GetRequestState(RequestId);
        if (IsJobProcessing())
        {
            MythicaEditorSubsystem->OnJobStateChange.AddDynamic(this, &UMythicaComponent::OnJobStateChanged);
        }
        else
        {
            RequestId = -1;
        }
    }
}

void UMythicaComponent::OnUnregister()
{
    Super::OnUnregister();

    DestroyPlaceholderMesh();

    UnbindWorldInputListeners();
    TransformUpdated.RemoveAll(this);

    if (RequestId > 0)
    {
        UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
        if (MythicaEditorSubsystem)
        {
            MythicaEditorSubsystem->OnJobStateChange.RemoveDynamic(this, &UMythicaComponent::OnJobStateChanged);
        }
    }
}

void UMythicaComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    GEditor->GetTimerManager()->ClearTimer(DelayRegenerateHandle);
}

bool UMythicaComponent::CanRegenerateMesh() const
{
    UWorld* World = GetWorld();
    return World && World->WorldType == EWorldType::Editor;
}

void UMythicaComponent::RegenerateMesh()
{
    if (RequestId > 0)
    {
        QueueRegenerate = true;
        return;
    }

    if (!ComponentGUID.IsValid())
    {
        ComponentGUID = FGuid::NewGuid();
    }

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    RequestId = MythicaEditorSubsystem->ExecuteJob(JobDefId.JobDefId, Parameters, GetImportPath(), K2_GetComponentLocation());

    if (RequestId > 0 && IsRegistered())
    {
        MythicaEditorSubsystem->OnJobStateChange.AddDynamic(this, &UMythicaComponent::OnJobStateChanged);
    }
}

FString UMythicaComponent::GetImportPath()
{
    FString ImportFolderClean = ObjectTools::SanitizeObjectName(ToolName);
    FString ImportNameClean = ObjectTools::SanitizeObjectName(ComponentGUID.ToString().Left(IMPORT_NAME_LENGTH));

    return FPaths::Combine(ImportFolderClean, ImportNameClean);
}

bool UMythicaComponent::IsJobProcessing() const
{
    return State >= EMythicaJobState::Requesting && State <= EMythicaJobState::Importing;
}

float UMythicaComponent::JobProgressPercent() const
{
    double TotalEstimatedTime = 0.0f;
    double ElapsedTime = 0.0f;
    for (const FMythicaProcessingStep& Step : ProcessingSteps)
    {
        double EstimatedTime = StateDurations.Contains(Step.State) ? StateDurations[Step.State] : Step.EstimatedDuration;
        TotalEstimatedTime += EstimatedTime;

        if (Step.State < State)
        {
            ElapsedTime += EstimatedTime;
        }
        else if (Step.State == State)
        {
            double TimeInCurrentState = FPlatformTime::Seconds() - StateBeginTime;
            ElapsedTime += FMath::Clamp(TimeInCurrentState, 0.0f, EstimatedTime);
        }
    }

    double Progress = TotalEstimatedTime > 0.0f ? ElapsedTime / TotalEstimatedTime : 0.0f;
    return FMath::Clamp(Progress, 0.0f, 1.0f);
}

void UMythicaComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, JobDefId))
    {
        OnJobDefIdChanged();
    }

    if (CanRegenerateMesh())
    {
        if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, Parameters))
        {
            if (Settings.RegenerateOnParameterChange)
            {
                if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
                {
                    GEditor->GetTimerManager()->SetTimer(DelayRegenerateHandle, [this]() { RegenerateMesh(); }, 0.05f, false);
                }
                else
                {
                    RegenerateMesh();
                }
            }

            BindWorldInputListeners();
        }
        else if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FMythicaComponentSettings, RegenerateOnInputChange))
        {
            BindWorldInputListeners();
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

static void ForceRefreshDetailsViewPanel()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    static const FName DetailsTabIdentifiers[] = {
        "LevelEditorSelectionDetails",
        "LevelEditorSelectionDetails2",
        "LevelEditorSelectionDetails3",
        "LevelEditorSelectionDetails4",
        "BlueprintDefaults",
        "BlueprintInspector"
    };

    for (const FName DetailsPanelName : DetailsTabIdentifiers)
    {
        TSharedPtr<IDetailsView> DetailsView = PropertyModule.FindDetailView(DetailsPanelName);
        if (DetailsView.IsValid())
        {
            DetailsView->ForceRefresh();
        }
    }
}

void UMythicaComponent::OnJobDefIdChanged()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();

    FMythicaParameters OldParameters = Parameters;
    FMythicaAssetVersionEntryPointReference OldSource = Source;

    FMythicaJobDefinition Definition = MythicaEditorSubsystem->GetJobDefinitionById(JobDefId.JobDefId);
    ToolName = Definition.Name;
    Parameters = Definition.Parameters;
    Source = Definition.Source;

    // Keep existing paramater values when updating to new version of same tool
    if (Source.IsValid() && OldSource.IsValid() && Source.Compare(OldSource))
    {
        Mythica::CopyParameterValues(OldParameters, Parameters);
    }

    State = EMythicaJobState::Invalid;
    StateDurations.Reset();

    ForceRefreshDetailsViewPanel();
}

void UMythicaComponent::BindWorldInputListeners()
{
    UnbindWorldInputListeners();

    if (!Settings.RegenerateOnInputChange)
    {
        return;
    }

    for (const FMythicaParameter& Parameter : Parameters.Parameters)
    {
        if (Parameter.Type != EMythicaParameterType::File)
        {
            continue;
        }

        const FMythicaParameterFile& Input = Parameter.ValueFile;
        if (Input.Type != EMythicaInputType::World)
        {
            continue;
        }

        for (AActor* Actor : Input.Actors)
        {
            if (!Actor)
            {
                continue;
            }

            USceneComponent* RootComponent = Actor->GetRootComponent();
            if (!WorldInputComponents.Contains(RootComponent))
            {
                RootComponent->TransformUpdated.AddUObject(this, &UMythicaComponent::OnWorldInputTransformUpdated);
                WorldInputComponents.Add(RootComponent);
            }
        }
    }
}

void UMythicaComponent::UnbindWorldInputListeners()
{
    for (USceneComponent* Component : WorldInputComponents)
    {
        if (Component)
        {
            Component->TransformUpdated.RemoveAll(this);
        }
    }
    WorldInputComponents.Reset();
}

void UMythicaComponent::OnWorldInputTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType)
{
    // Ignore events that fire during level load actor initialization
    if (!GIsEditorLoadingPackage)
    {
        GEditor->GetTimerManager()->SetTimer(DelayRegenerateHandle, [this]() { RegenerateMesh(); }, 0.05f, false);
    }
}

void UMythicaComponent::OnTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType)
{
    // Ignore events that fire during level load actor initialization
    if (!GIsEditorLoadingPackage && Settings.RegenerateOnTransformChange)
    {
        GEditor->GetTimerManager()->SetTimer(DelayRegenerateHandle, [this]() { RegenerateMesh(); }, 0.05f, false);
    }
}

void UMythicaComponent::OnJobStateChanged(int InRequestId, EMythicaJobState InState, FText InMessage)
{
    if (InRequestId != RequestId)
    {
        return;
    }

    StateDurations.Add(State, FPlatformTime::Seconds() - StateBeginTime);

    State = InState;
    Message = InMessage;
    StateBeginTime = FPlatformTime::Seconds();
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

    if (QueueRegenerate)
    {
        QueueRegenerate = false;
        RegenerateMesh();
    }
}

void UMythicaComponent::UpdateMesh()
{
    AActor* OwnerActor = GetOwner();

    // Clear existing meshes but save cache for re-use
    TArray<UStaticMeshComponent*> ExistingMeshCache;

    for (UActorComponent* Component : OwnerActor->GetComponents())
    {
        UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
        if (!MeshComponent)
        {
            continue;
        }

        for (FName Name : MeshComponentNames)
        {
            if (Component->GetFName() == Name)
            {
                ExistingMeshCache.Add(MeshComponent);
                break;
            }
        }
    }
    MeshComponentNames.Reset();

    // Scan the directory for desired meshes
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    FString ImportDirectory = MythicaEditorSubsystem->GetImportDirectory(RequestId);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(*ImportDirectory, Assets, true, false);

    for (FAssetData Asset : Assets)
    {
        if (!Asset.IsInstanceOf(UStaticMesh::StaticClass()))
        {
            continue;
        }

        FString ObjectPath = Asset.GetObjectPathString();
        UStaticMeshComponent* MeshComponent = nullptr;

        // Try to re-use an existing mesh component
        for (int32 i = 0; i < ExistingMeshCache.Num(); i++)
        {
            UStaticMesh* Mesh = ExistingMeshCache[i]->GetStaticMesh();
            if (Mesh && Mesh->GetPathName() == ObjectPath)
            {
                MeshComponent = ExistingMeshCache[i];
                ExistingMeshCache.RemoveAt(i);
                break;
            }
        }
        
        // Otherwise spawn a new one
        if (!MeshComponent)
        {
            MeshComponent = NewObject<UStaticMeshComponent>(OwnerActor);
            MeshComponent->SetStaticMesh(Cast<UStaticMesh>(Asset.GetAsset()));
            MeshComponent->SetWorldLocation(K2_GetComponentLocation());
            MeshComponent->AttachToComponent(GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);

            OwnerActor->AddInstanceComponent(MeshComponent);
            MeshComponent->RegisterComponent();

        }

        MeshComponentNames.Add(MeshComponent->GetFName());
    }
    
    // Destroy any meshes that were not re-used
    for (UStaticMeshComponent* MeshComponent : ExistingMeshCache)
    {
        MeshComponent->DestroyComponent();
    }

    UpdatePlaceholderMesh();
}

void UMythicaComponent::UpdatePlaceholderMesh()
{
    if (MeshComponentNames.IsEmpty() && !PlaceholderMeshComponent)
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, PLACEHOLDER_MESH_ASSET));

        PlaceholderMeshComponent = NewObject<UStaticMeshComponent>(
            this, UStaticMeshComponent::StaticClass(), NAME_None, RF_Transactional);

        PlaceholderMeshComponent->SetStaticMesh(Mesh);
        PlaceholderMeshComponent->SetHiddenInGame(true);
        PlaceholderMeshComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        PlaceholderMeshComponent->RegisterComponent();
    }
    else if (!MeshComponentNames.IsEmpty() && PlaceholderMeshComponent)
    {
        PlaceholderMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
        PlaceholderMeshComponent->UnregisterComponent();
        PlaceholderMeshComponent->DestroyComponent();
        PlaceholderMeshComponent = nullptr;
    }
}

void UMythicaComponent::DestroyPlaceholderMesh()
{
    if (PlaceholderMeshComponent)
    {
        PlaceholderMeshComponent->DestroyComponent();
        PlaceholderMeshComponent = nullptr;
    }
}