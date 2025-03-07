#include "MythicaComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
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

UMythicaComponent::UMythicaComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;

    // This way the engine will stop people from trying to reference this comp at runtime.
    bIsEditorOnly = true;

    // This is so that is easy to get a reference to all MythicaComponnents
    ComponentTags.Emplace(MYTHICA_COMPONENT_TAG);
}

void UMythicaComponent::OnRegister()
{
    Super::OnRegister();

    UpdatePlaceholderMesh();

    if (CanRegenerateMesh())
    {
        BindWorldInputListeners();
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

    if (!ComponentGuid.IsValid())
    {
        ComponentGuid = FGuid::NewGuid();
    }

    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    RequestId = MythicaEditorSubsystem->ExecuteJob(JobDefId.JobDefId, Parameters, GetImportPath(), GetOwner()->GetActorLocation(), this);

    if (RequestId > 0 && IsRegistered())
    {
        MythicaEditorSubsystem->OnJobStateChange.AddDynamic(this, &UMythicaComponent::OnJobStateChanged);
    }
}

FString UMythicaComponent::GetImportPath()
{
    FString ImportFolderClean = ObjectTools::SanitizeObjectName(ToolName);
    FString ImportNameClean = ObjectTools::SanitizeObjectName(ComponentGuid.ToString().Left(IMPORT_NAME_LENGTH));

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

TArray<UActorComponent*> UMythicaComponent::GetGeneratedMeshComponents() const
{
    AActor* Owner = GetOwner();
    ensure(Owner);

    TArray<UActorComponent*> MeshComponents = Owner->GetComponentsByTag(UStaticMeshComponent::StaticClass(), *ComponentGuid.ToString());

    return MoveTemp(MeshComponents);
}

TArray<AActor*> UMythicaComponent::GetWorldInputActors() const
{
    TArray<AActor*> InputActors = TArray<AActor*>();

    for (const FMythicaParameter Param : Parameters.Parameters)
    {
        if (Param.Type == EMythicaParameterType::File)
        {
            const FMythicaParameterFile& File = Param.ValueFile;

            for (AActor* Actor : File.Actors)
            {
                InputActors.Emplace(Actor);
            }
        }
    }

    return InputActors;
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
    ensure(OwnerActor);

    TArray<UActorComponent*> ExistingMeshCache = OwnerActor->GetComponentsByTag(UStaticMeshComponent::StaticClass(), *ComponentGuid.ToString());

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
            UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(ExistingMeshCache[i]);
            if (!IsValid(MeshComp))
            {
                continue;
            }

            UStaticMesh* Mesh = MeshComp->GetStaticMesh();
            if (Mesh && Mesh->GetPathName() == ObjectPath)
            {
                MeshComponent = MeshComp;
                ExistingMeshCache.RemoveAt(i);
                break;
            }
        }
        
        // Otherwise spawn a new one
        if (!MeshComponent)
        {
            FString ComponentName = FString::Printf(TEXT("GEN_%s"), *GetName());
            MeshComponent = NewObject<UStaticMeshComponent>(OwnerActor, *ComponentName, RF_Transactional);

            MeshComponent->SetStaticMesh(Cast<UStaticMesh>(Asset.GetAsset()));
            MeshComponent->SetupAttachment(OwnerActor->GetRootComponent());

            OwnerActor->AddInstanceComponent(MeshComponent);
            MeshComponent->RegisterComponent();

            MeshComponent->ComponentTags.Emplace(*ComponentGuid.ToString());
        }

        MythicaEditorSubsystem->SetJobsCachedAssetData(RequestId, Asset);

        // Used to trigger a save object in the level instance so that we can save the new instance components.
        OwnerActor->Modify();
    }

    // Destroy any meshes that were not re-used
    for (UActorComponent* GenComponent : ExistingMeshCache)
    {
        GenComponent->DestroyComponent();
    }

    UpdatePlaceholderMesh();
}

void UMythicaComponent::UpdatePlaceholderMesh()
{
    AActor* Owner = GetOwner();
    ensure(Owner);

    TArray<UActorComponent*> GenComps = GetGeneratedMeshComponents();
    if (GenComps.IsEmpty() && !IsValid(PlaceholderMeshComponent))
    {
        Owner->Modify();

        UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, PLACEHOLDER_MESH_ASSET));

        PlaceholderMeshComponent = NewObject<UStaticMeshComponent>(
            this, UStaticMeshComponent::StaticClass(), NAME_None, RF_Transactional);

        PlaceholderMeshComponent->SetStaticMesh(Mesh);
        PlaceholderMeshComponent->SetHiddenInGame(true);
        PlaceholderMeshComponent->SetupAttachment(GetOwner()->GetRootComponent());
        Owner->AddOwnedComponent(PlaceholderMeshComponent);
        PlaceholderMeshComponent->RegisterComponent();

        Owner->Modify();
    }
    else if (!GenComps.IsEmpty() && IsValid(PlaceholderMeshComponent))
    {
        Owner->RemoveOwnedComponent(PlaceholderMeshComponent);
        PlaceholderMeshComponent->DestroyComponent();
        PlaceholderMeshComponent = nullptr;
    }
}

void UMythicaComponent::DestroyPlaceholderMesh()
{
    if (IsValid(PlaceholderMeshComponent))
    {
        PlaceholderMeshComponent->DestroyComponent();
        PlaceholderMeshComponent = nullptr;
    }
}
