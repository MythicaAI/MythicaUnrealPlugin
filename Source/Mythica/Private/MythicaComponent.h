#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MythicaEditorSubsystem.h"
#include "MythicaTypes.h"
#include "MythicaComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UMythicaComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UMythicaComponent();

    virtual void PostLoad() override;
    void OnComponentDestroyed(bool bDestroyingHierarchy);

    bool CanRegenerateMesh() const;
    void RegenerateMesh();
    EMythicaJobState GetJobState() const { return State; }

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    void OnJobDefIdChanged();

    void BindWorldInputListeners();
    void OnWorldInputTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);
    void OnTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);

    UFUNCTION()
    void OnJobStateChanged(int InRequestId, EMythicaJobState InState);

    void UpdateMesh();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaJobDefinitionId JobDefId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnParameterChange = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnInputChange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnTransformChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaParameters Parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaInputs Inputs;

private:
    int RequestId = -1;
    EMythicaJobState State = EMythicaJobState::Invalid;
    bool QueueRegenerate = false;
    FTimerHandle DelayRegenerateHandle;

    UPROPERTY(Transient)
    TSet<USceneComponent*> WorldInputComponents;

    UPROPERTY()
    TArray<FName> MeshComponentNames;
};
