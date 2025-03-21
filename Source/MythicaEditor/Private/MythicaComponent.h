#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MythicaEditorSubsystem.h"
#include "MythicaTypes.h"
#include "MythicaComponent.generated.h"

#define MYTHICA_COMPONENT_TAG TEXT("MythicaEditorComp")

/**
 * FMythicaComponentSettings
 *
 * Used to store important settings about how the component functions.
 */
USTRUCT(BlueprintType)
struct FMythicaComponentSettings
{

    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool RegenerateOnParameterChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool RegenerateOnInputChange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool RegenerateOnTransformChange = false;
};

/**
 * UMythicaComponent
 *
 * A component that interfaces with Mythica's API to generate mesh assets and add them to an actor.
 * This is an Editor-Only component which will get stripped in production builds so do not rely on it for
 * runtime applications.
 */
UCLASS( ClassGroup=(Mythica), meta=(BlueprintSpawnableComponent), hidecategories=(Activation, Cooking, AssetUserData, Navigation))
class UMythicaComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UMythicaComponent(const FObjectInitializer& ObjectInitializer);

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
    TArray<UActorComponent*> GetGeneratedMeshComponents() const;

    UFUNCTION(BlueprintPure, Category = "Mythica|Component")
    TArray<AActor*> GetWorldInputActors() const;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (EditCondition = "false", EditConditionHides))
    FMythicaJobDefinitionId JobDefId = FMythicaJobDefinitionId();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (EditCondition = "false", EditConditionHides))
    FMythicaAssetVersionEntryPointReference Source = FMythicaAssetVersionEntryPointReference();

    UPROPERTY(BlueprintReadOnly)
    FString ToolName = FString();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaComponentSettings Settings = FMythicaComponentSettings();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica|Parameters")
    FMythicaParameters Parameters = FMythicaParameters();

private:

    UPROPERTY(VisibleAnywhere, DuplicateTransient, meta = (EditCondition = "false", EditConditionHides))
    int RequestId = -1;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, meta = (EditCondition = "false", EditConditionHides))
    EMythicaJobState State = EMythicaJobState::Invalid;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, meta = (EditCondition = "false", EditConditionHides))
    FText Message = FText();

    UPROPERTY(VisibleAnywhere, DuplicateTransient, meta = (EditCondition = "false", EditConditionHides))
    double StateBeginTime = 0.0f;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, meta = (EditCondition = "false", EditConditionHides))
    bool QueueRegenerate = false;

    UPROPERTY(Transient, DuplicateTransient)
    FTimerHandle DelayRegenerateHandle;

    UPROPERTY(VisibleAnywhere, meta = (EditCondition = "false", EditConditionHides))
    TMap<EMythicaJobState, double> StateDurations = TMap<EMythicaJobState, double>();

    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<UStaticMeshComponent> PlaceholderMeshComponent = nullptr;

    UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "Mythica", meta = (EditCondition = "false", EditConditionHides))
    FGuid ComponentGuid = FGuid();

    UPROPERTY(Transient, DuplicateTransient)
    TSet<TObjectPtr<USceneComponent>> WorldInputComponents = TSet<TObjectPtr<USceneComponent>>();
};
