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

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Mythica")
	void RegenerateMesh();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    void OnJobDefIdChanged();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
	FString JobDefId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
	FMythicaInputs Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
	FMythicaParameters Parameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythica")
	FMythicaMaterialParameters MaterialParameters;

private:
	UPROPERTY()
	int JobId = -1;
};
