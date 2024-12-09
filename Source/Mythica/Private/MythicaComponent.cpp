#include "MythicaComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"

#define IMPORT_NAME_LENGTH 10

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

void UMythicaComponent::OnComponentCreated()
{
    Super::OnComponentCreated();

    ComponentGUID = FGuid::NewGuid();
}

void UMythicaComponent::PostLoad()
{
    Super::PostLoad();

    if (!CanRegenerateMesh())
    {
        return;
    }

    BindWorldInputListeners();

    TransformUpdated.AddUObject(this, &UMythicaComponent::OnTransformUpdated);
}

void UMythicaComponent::OnRegister()
{
    Super::OnRegister();

    UpdatePlaceholderMesh();
}

void UMythicaComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    GEditor->GetTimerManager()->ClearTimer(DelayRegenerateHandle);

    if (RequestId > 0)
    {
        UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
        if (MythicaEditorSubsystem)
        {
            MythicaEditorSubsystem->OnJobStateChange.RemoveDynamic(this, &UMythicaComponent::OnJobStateChanged);
        }
    }

    if (PlaceholderMeshComponent)
    {
        PlaceholderMeshComponent->DestroyComponent();
        PlaceholderMeshComponent = nullptr;
    }
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

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    RequestId = MythicaEditorSubsystem->ExecuteJob(JobDefId.JobDefId, Inputs, Parameters, GetImportName(), K2_GetComponentLocation());

    if (RequestId > 0)
    {
        MythicaEditorSubsystem->OnJobStateChange.AddDynamic(this, &UMythicaComponent::OnJobStateChanged);
    }
}

FString UMythicaComponent::GetImportName()
{
    return ComponentGUID.ToString().Left(IMPORT_NAME_LENGTH);
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
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (!CanRegenerateMesh())
    {
        return;
    }

    if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, JobDefId))
    {
        OnJobDefIdChanged();
    }
    else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, Parameters))
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
    }
    else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, Inputs))
    {
        if (Settings.RegenerateOnInputChange)
        {
            BindWorldInputListeners();
            RegenerateMesh();
        }
    }
    else if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FMythicaComponentSettings, RegenerateOnInputChange))
    {
        BindWorldInputListeners();
    }
}

static void ForceRefreshDetailsViewPanel()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    static const FName DetailsTabIdentifiers[] = {
        "LevelEditorSelectionDetails",
        "LevelEditorSelectionDetails2",
        "LevelEditorSelectionDetails3",
        "LevelEditorSelectionDetails4" 
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

    FMythicaJobDefinition Definition = MythicaEditorSubsystem->GetJobDefinitionById(JobDefId.JobDefId);
    Parameters = Definition.Parameters;
    Inputs = Definition.Inputs;

    State = EMythicaJobState::Invalid;
    StateDurations.Reset();

    ForceRefreshDetailsViewPanel();
}

void UMythicaComponent::BindWorldInputListeners()
{
    for (USceneComponent* Component : WorldInputComponents)
    {
        if (Component)
        {
            Component->TransformUpdated.RemoveAll(this);
        }
    }
    WorldInputComponents.Reset();

    if (!Settings.RegenerateOnInputChange)
    {
        return;
    }

    for (FMythicaInput& Input : Inputs.Inputs)
    {
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

void UMythicaComponent::OnJobStateChanged(int InRequestId, EMythicaJobState InState)
{
    if (InRequestId != RequestId)
    {
        return;
    }

    StateDurations.Add(State, FPlatformTime::Seconds() - StateBeginTime);

    State = InState;
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

    // Remove existing meshes
    for (FName Name : MeshComponentNames)
    {
        for (UActorComponent* Component : OwnerActor->GetComponents())
        {
            if (Component->GetFName() == Name)
            {
                Component->DestroyComponent();
                break;
            }
        }
    }
    MeshComponentNames.Reset();

    // Spawn new meshes
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    FString ImportDirectory = MythicaEditorSubsystem->GetImportDirectory(RequestId);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(*ImportDirectory, Assets, true, false);

    for (FAssetData Asset : Assets)
    {
        if (Asset.IsInstanceOf(UStaticMesh::StaticClass()))
        {
            UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(OwnerActor);
            StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(Asset.GetAsset()));
            StaticMeshComponent->SetWorldLocation(K2_GetComponentLocation());
            StaticMeshComponent->AttachToComponent(GetAttachParent(), FAttachmentTransformRules::KeepWorldTransform);

            OwnerActor->AddInstanceComponent(StaticMeshComponent);
            StaticMeshComponent->RegisterComponent();

            MeshComponentNames.Add(StaticMeshComponent->GetFName());
        }
    }

    UpdatePlaceholderMesh();
}

void UMythicaComponent::UpdatePlaceholderMesh()
{
    if (MeshComponentNames.IsEmpty() && !PlaceholderMeshComponent)
    {
        const FString CubePath = TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'");
        UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, *CubePath));

        PlaceholderMeshComponent = NewObject<UStaticMeshComponent>(
            this, UStaticMeshComponent::StaticClass(), NAME_None, RF_Transactional);

        PlaceholderMeshComponent->SetStaticMesh(Mesh);
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
