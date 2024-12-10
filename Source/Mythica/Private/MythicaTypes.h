#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"
#include "MythicaUSDUtil.h"

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

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Input")
    FString Label;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
    EMythicaInputType Type = EMythicaInputType::Mesh;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input", meta = (EditCondition = "Type == EMythicaInputType::World || Type == EMythicaInputType::Spline", EditConditionHides))
    EMythicaExportTransformType TransformType = EMythicaExportTransformType::Relative;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input", meta = (EditCondition = "Type == EMythicaInputType::Mesh", EditConditionHides))
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input", meta = (EditCondition = "Type == EMythicaInputType::World", EditConditionHides))
    TArray<AActor*> Actors;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input", meta = (EditCondition = "Type == EMythicaInputType::Spline", EditConditionHides))
    AActor* SplineActor = nullptr;
};

USTRUCT(BlueprintType)
struct FMythicaInputs
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Input")
    TArray<FMythicaInput> Inputs;
};

UENUM()
enum class EMythicaParameterType : uint8
{
    Int,
    Float,
    Bool,
    String,
    Enum
};

USTRUCT()
struct FMythicaParameterInt
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    TArray<int> Values;

    UPROPERTY(VisibleAnywhere)
    TArray<int> DefaultValues;

    UPROPERTY(VisibleAnywhere)
    TOptional<int> MinValue;

    UPROPERTY(VisibleAnywhere)
    TOptional<int> MaxValue;
};

USTRUCT()
struct FMythicaParameterFloat
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    TArray<float> Values;

    UPROPERTY(VisibleAnywhere)
    TArray<float> DefaultValues;

    UPROPERTY(VisibleAnywhere)
    TOptional<float> MinValue;

    UPROPERTY(VisibleAnywhere)
    TOptional<float> MaxValue;
};

USTRUCT()
struct FMythicaParameterBool
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    bool Value = false;

    UPROPERTY(VisibleAnywhere)
    bool DefaultValue = false;
};

USTRUCT()
struct FMythicaParameterString
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    FString Value;

    UPROPERTY(VisibleAnywhere)
    FString DefaultValue;
};

USTRUCT()
struct FMythicaParameterEnumValue
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    FString Name;

    UPROPERTY(VisibleAnywhere)
    FString Label;
};

USTRUCT()
struct FMythicaParameterEnum
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    FString Value;

    UPROPERTY(VisibleAnywhere)
    FString DefaultValue;

    UPROPERTY(VisibleAnywhere)
    TArray<FMythicaParameterEnumValue> Values;
};

USTRUCT(BlueprintType)
struct FMythicaParameter
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere)
    FString Name;

    UPROPERTY(VisibleAnywhere)
    FString Label;

    UPROPERTY(VisibleAnywhere)
    EMythicaParameterType Type = EMythicaParameterType::Int;

    UPROPERTY(VisibleAnywhere)
    FMythicaParameterInt ValueInt;

    UPROPERTY(VisibleAnywhere)
    FMythicaParameterFloat ValueFloat;

    UPROPERTY(VisibleAnywhere)
    FMythicaParameterBool ValueBool;

    UPROPERTY(VisibleAnywhere)
    FMythicaParameterString ValueString;

    UPROPERTY(VisibleAnywhere)
    FMythicaParameterEnum ValueEnum;
};

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parameter")
    TArray<FMythicaParameter> Parameters;
};

USTRUCT(BlueprintType)
struct FMythicaMaterialParameters
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
    UMaterialInterface* Material;
};

namespace Mythica
{
    bool IsSystemParameter(const FString& Name);

    void ReadParameters(const TSharedPtr<FJsonObject>& ParamsSchema, FMythicaInputs& OutInputs, FMythicaParameters& OutParameters);
    void WriteParameters(const FMythicaInputs& Inputs, const TArray<FString>& InputFileIds, const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet);
}
