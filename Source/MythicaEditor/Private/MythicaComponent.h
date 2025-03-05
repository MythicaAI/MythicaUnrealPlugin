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
    bool RegenerateOnParameterChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnInputChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnTransformChange = false;
};

UCLASS( ClassGroup=(Mythica), meta=(BlueprintSpawnableComponent), hidecategories=(Activation, Cooking, AssetUserData))
class UMythicaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMythicaComponent();

    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    UFUNCTION(BlueprintPure, Category="Mythica|Component")
    bool CanRegenerateMesh() const;
    UFUNCTION(BlueprintCallable, Category = "Mythica|Component")
    void RegenerateMesh();
    FString GetImportPath();

    EMythicaJobState GetJobState() const { return State; }
    FText GetJobMessage() const { return Message; }
    bool IsJobProcessing() const;

    UFUNCTION(BlueprintPure, Category="Mythica|Component")
    float JobProgressPercent() const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    UFUNCTION(BlueprintPure, Category = "Mythica|Component")
    TArray<UStaticMeshComponent*> GetGeneratedMeshComponents() const;

    UFUNCTION(BlueprintPure, Category = "Mythica|Component")
    TArray<AActor*> GetWorldInputActors() const;

    UFUNCTION(BlueprintPure, Category="Mythica|Component")
    TArray<FName> GetMeshComponentNames() const { return MeshComponentNames; }

    UFUNCTION(BlueprintPure, Category="Mythica|Component")
    const FGuid GetGuid() const { return ComponentGuid; }

private:
    void OnJobDefIdChanged();

    void BindWorldInputListeners();
    void UnbindWorldInputListeners();
    void OnWorldInputTransformUpdated(USceneComponent* InComponent, EUpdateTransformFlags InFlags, ETeleportType InType);

    UFUNCTION()
    void OnJobStateChanged(int InRequestId, EMythicaJobState InState, FText InMessage);

    void UpdateMesh();
    void UpdatePlaceholderMesh();
    void DestroyPlaceholderMesh();

public:

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FMythicaJobDefinitionId JobDefId = FMythicaJobDefinitionId();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FMythicaAssetVersionEntryPointReference Source = FMythicaAssetVersionEntryPointReference();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FString ToolName = FString();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaComponentSettings Settings = FMythicaComponentSettings();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaParameters Parameters = FMythicaParameters();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TSet<TObjectPtr<USceneComponent>> WorldInputComponents = TSet<TObjectPtr<USceneComponent>>();

private:
    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    int RequestId = -1;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    EMythicaJobState State = EMythicaJobState::Invalid;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FText Message = FText();

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    double StateBeginTime = 0.0f;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    bool QueueRegenerate = false;

    UPROPERTY(Transient)
    FTimerHandle DelayRegenerateHandle;

    UPROPERTY(VisibleAnywhere, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    TMap<EMythicaJobState, double> StateDurations = TMap<EMythicaJobState, double>();

    //UPROPERTY(Transient)
    //TObjectPtr<UStaticMeshComponent> PlaceholderMeshComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    TArray<FName> MeshComponentNames = TArray<FName>();

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FGuid ComponentGuid = FGuid();
};
