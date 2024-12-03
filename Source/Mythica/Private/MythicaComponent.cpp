#include "MythicaComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"

#define IMPORT_NAME_LENGTH 10

UMythicaComponent::UMythicaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    EstimateStateTimes[(uint8)EMythicaJobState::Requesting] = 0.1f;
    EstimateStateTimes[(uint8)EMythicaJobState::Queued] = 0.1f;
    EstimateStateTimes[(uint8)EMythicaJobState::Processing] = 1.0f;
    EstimateStateTimes[(uint8)EMythicaJobState::Importing] = 0.25f;
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
    // Prepare estimated durations for each job state
    const TArray<TPair<EMythicaJobState, double>> StateEstimatedDurations = {
        { EMythicaJobState::Requesting, EstimateStateTimes[(uint8)EMythicaJobState::Requesting] },
        { EMythicaJobState::Queued,     EstimateStateTimes[(uint8)EMythicaJobState::Queued] },
        { EMythicaJobState::Processing, EstimateStateTimes[(uint8)EMythicaJobState::Processing] },
        { EMythicaJobState::Importing,  EstimateStateTimes[(uint8)EMythicaJobState::Importing] },
    };
    static_assert((uint8)EMythicaJobState::Completed - (uint8)EMythicaJobState::Invalid == 5);

    double TotalEstimatedTime = 0.0f;
    for (const auto& StateDuration : StateEstimatedDurations)
    {
        TotalEstimatedTime += StateDuration.Value;
    }

    // Calculate current progress precentage
    double ElapsedTime = 0.0f;
    for (const auto& StateDuration : StateEstimatedDurations)
    {
        if (StateDuration.Key == State)
        {
            double CurrentTime = FPlatformTime::Seconds();
            double TimeInCurrentState = CurrentTime - StateBeginTime;
            ElapsedTime += FMath::Clamp(TimeInCurrentState, 0.0f, StateDuration.Value);
            break;
        }
        else
        {
            ElapsedTime += StateDuration.Value;
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
        if (RegenerateOnParameterChange)
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
        if (RegenerateOnInputChange)
        {
            BindWorldInputListeners();
            RegenerateMesh();
        }
    }
    else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UMythicaComponent, RegenerateOnInputChange))
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

    if (!RegenerateOnInputChange)
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
    if (!GIsEditorLoadingPackage && RegenerateOnTransformChange)
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

    if (State >= EMythicaJobState::Requesting && State <= EMythicaJobState::Importing)
    {
        EstimateStateTimes[(int8)State] = FPlatformTime::Seconds() - StateBeginTime;
    }

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
}
