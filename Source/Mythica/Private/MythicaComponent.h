#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MythicaEditorSubsystem.h"
#include "MythicaTypes.h"
#include "MythicaComponent.generated.h"

USTRUCT(BlueprintType)
struct FMythicaComponentSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnParameterChange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnInputChange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnTransformChange = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UMythicaComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UMythicaComponent();

    virtual void PostLoad() override;
    virtual void OnComponentCreated() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    bool CanRegenerateMesh() const;
    void RegenerateMesh();
    FString GetImportName();

    EMythicaJobState GetJobState() const { return State; }
    bool IsJobProcessing() const;
    float JobProgressPercent() const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    void OnJobDefIdChanged();

    void BindWorldInputListeners();
    void OnWorldInputTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);
    void OnTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);

    UFUNCTION()
    void OnJobStateChanged(int InRequestId, EMythicaJobState InState);

    void UpdateMesh();
    void UpdatePlaceholderMesh();

public:
    UPROPERTY()
    FMythicaJobDefinitionId JobDefId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaComponentSettings Settings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaParameters Parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaInputs Inputs;

private:
    int RequestId = -1;
    EMythicaJobState State = EMythicaJobState::Invalid;
    double StateBeginTime = 0.0f;
    bool QueueRegenerate = false;
    FTimerHandle DelayRegenerateHandle;

    UPROPERTY()
    TMap<EMythicaJobState, double> StateDurations;

    UPROPERTY(Transient)
    TSet<USceneComponent*> WorldInputComponents;

    UPROPERTY(Transient)
    UStaticMeshComponent* PlaceholderMeshComponent = nullptr;

    UPROPERTY()
    TArray<FName> MeshComponentNames;

    UPROPERTY(DuplicateTransient)
    FGuid ComponentGUID;
};
