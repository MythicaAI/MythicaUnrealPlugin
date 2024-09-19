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
};
