#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"

#include "MythicaTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicaInputType : uint8
{
    Mesh,
    World
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
    AActor* Actor = nullptr;
};

USTRUCT(BlueprintType)
struct FMythicaInputs
{
    GENERATED_BODY()

    UPROPERTY()
    int InputCount = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "InputCount > 0", EditConditionHides))
    FMythicaInput Input0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "InputCount > 1", EditConditionHides))
    FMythicaInput Input1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "InputCount > 2", EditConditionHides))
    FMythicaInput Input2;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "InputCount > 3", EditConditionHides))
    FMythicaInput Input3;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "InputCount > 4", EditConditionHides))
    FMythicaInput Input4;
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

namespace Mythica
{
    void ReadParameters(const TSharedPtr<FJsonObject>& ParameterDef, FMythicaParameters& OutParameters);
    void WriteParameters(const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet);
}
