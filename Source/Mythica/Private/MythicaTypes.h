#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"

#include "MythicaTypes.generated.h"

struct FMythicaParameterBool
{
    bool Value = false;
    bool DefaultValue = false;
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

using FMythicaParameterValue = TVariant<FMythicaParameterBool, FMythicaParameterInt, FMythicaParameterFloat>;

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
