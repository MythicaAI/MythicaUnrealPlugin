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
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void OnRegister() override;

    bool CanRegenerateMesh() const;
    void RegenerateMesh();
    FString GetImportName();

    EMythicaJobState GetJobState() const { return State; }
    FText GetJobMessage() const { return Message; }
    bool IsJobProcessing() const;
    float JobProgressPercent() const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    void OnJobDefIdChanged();

    void BindWorldInputListeners();
    void OnWorldInputTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);
    void OnTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);

    UFUNCTION()
    void OnJobStateChanged(int InRequestId, EMythicaJobState InState, FText InMessage);

    void UpdateMesh();
    void UpdatePlaceholderMesh();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FMythicaJobDefinitionId JobDefId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaComponentSettings Settings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaParameters Parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (EditCondition = "DisplayInputs", EditConditionHides, HideEditConditionToggle))
    FMythicaInputs Inputs;

private:
    int RequestId = -1;
    EMythicaJobState State = EMythicaJobState::Invalid;
    FText Message;
    double StateBeginTime = 0.0f;

    bool QueueRegenerate = false;
    FTimerHandle DelayRegenerateHandle;

    UPROPERTY(Transient)
    bool DisplayInputs = false;

    UPROPERTY(VisibleAnywhere, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    TMap<EMythicaJobState, double> StateDurations;

    UPROPERTY(Transient)
    TSet<USceneComponent*> WorldInputComponents;

    UPROPERTY(Transient)
    UStaticMeshComponent* PlaceholderMeshComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    TArray<FName> MeshComponentNames;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FGuid ComponentGUID;
};
