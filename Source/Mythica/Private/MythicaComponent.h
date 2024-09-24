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

    void EndPlay(const EEndPlayReason::Type EndPlayReason);

    void RegenerateMesh();
    EMythicaJobState GetJobState() const { return State; }

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    void OnJobDefIdChanged();

    UFUNCTION()
    void OnJobStateChanged(int InRequestId, EMythicaJobState InState);

    void UpdateMesh();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    FMythicaJobDefinitionId JobDefId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
    bool RegenerateOnParameterChange = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaParameters Parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaMaterialParameters MaterialParameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
    FMythicaInputs Inputs;

private:
    int RequestId = -1;
    EMythicaJobState State = EMythicaJobState::Invalid;
    bool QueueRegenerate = false;

    UPROPERTY()
    TArray<FName> MeshComponentNames;
};
