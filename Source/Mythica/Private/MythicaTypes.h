#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"

#include "MythicaTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicaInputType : uint8
{
    Mesh,
    World,
    Spline
};

USTRUCT(BlueprintType)
struct FMythicaInput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EMythicaInputType Type = EMythicaInputType::Mesh;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "Type == EMythicaInputType::Mesh", EditConditionHides))
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "Type == EMythicaInputType::World", EditConditionHides))
    TArray<AActor*> Actors;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "Type == EMythicaInputType::Spline", EditConditionHides))
    AActor* SplineActor;
};

USTRUCT(BlueprintType)
struct FMythicaInputs
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FMythicaInput> Inputs;
};

struct FMythicaParameterInt
{
    TArray<int> Values;
    TArray<int> DefaultValues;
    TOptional<int> MinValue;
    TOptional<int> MaxValue;
};

struct FMythicaParameterFloat
{
    TArray<float> Values;
    TArray<float> DefaultValues;
    TOptional<float> MinValue;
    TOptional<float> MaxValue;
};

struct FMythicaParameterBool
{
    bool Value = false;
    bool DefaultValue = false;
};

struct FMythicaParameterString
{
    FString Value;
    FString DefaultValue;
};

using FMythicaParameterValue = TVariant<FMythicaParameterInt, FMythicaParameterFloat, FMythicaParameterBool, FMythicaParameterString>;

USTRUCT(BlueprintType)
struct FMythicaParameter
{
    GENERATED_BODY()

    FString Name;
    FString Label;
    FMythicaParameterValue Value;
};

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FMythicaParameter> Parameters;
};

USTRUCT(BlueprintType)
struct FMythicaMaterialParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UMaterialInterface* Material;
};

namespace Mythica
{
    void ReadParameters(const TSharedPtr<FJsonObject>& ParameterDef, FMythicaParameters& OutParameters);
    void WriteParameters(const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet);
}
