#pragma once

#include "CoreMinimal.h"
#include "Misc/TVariant.h"
#include "MythicaUSDUtil.h"

#include "MythicaTypes.generated.h"

class AMythicaInputSelectionVolume;

UENUM(BlueprintType)
enum class EMythicaInputType : uint8
{
    Mesh    UMETA(DisplayName = "Static Mesh"),
    World   UMETA(DisplayName = "World Actors"),
    Spline  UMETA(DisplayName = "World Spline"),
    Volume  UMETA(DisplayName = "Volume")
};

UENUM()
enum class EMythicaParameterType : uint8
{
    Int,
    Float,
    Bool,
    String,
    Enum,
    File
};

USTRUCT()
struct FMythicaParameterInt
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TArray<int> Values;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TArray<int> DefaultValues;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TOptional<int> MinValue;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TOptional<int> MaxValue;
};

USTRUCT()
struct FMythicaParameterFloat
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TArray<float> Values;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TArray<float> DefaultValues;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TOptional<float> MinValue;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TOptional<float> MaxValue;
};

USTRUCT()
struct FMythicaParameterBool
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    bool Value = false;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    bool DefaultValue = false;
};

USTRUCT()
struct FMythicaParameterString
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Value;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString DefaultValue;
};

USTRUCT()
struct FMythicaParameterEnumValue
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Name;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Label;
};

USTRUCT()
struct FMythicaParameterEnum
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Value;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString DefaultValue;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    TArray<FMythicaParameterEnumValue> Values;
};

USTRUCT()
struct FMythicaParameterFile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Parameter")
    EMythicaInputType Type = EMythicaInputType::World;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Type != EMythicaInputType::Mesh", EditConditionHides))
    EMythicaExportTransformType TransformType = EMythicaExportTransformType::Relative;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Type == EMythicaInputType::Mesh", EditConditionHides))
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Type == EMythicaInputType::World", EditConditionHides))
    TArray<AActor*> Actors;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Type == EMythicaInputType::Spline", EditConditionHides))
    AActor* SplineActor = nullptr;

    UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "Type == EMythicaInputType::Volume", EditConditionHides))
    AMythicaInputSelectionVolume* VolumeActor = nullptr;
};

USTRUCT(BlueprintType)
struct FMythicaParameter
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Name;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FString Label;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    EMythicaParameterType Type = EMythicaParameterType::Int;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FMythicaParameterInt ValueInt;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FMythicaParameterFloat ValueFloat;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FMythicaParameterBool ValueBool;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FMythicaParameterString ValueString;

    UPROPERTY(VisibleAnywhere, Category = "Parameter")
    FMythicaParameterEnum ValueEnum;
 
    UPROPERTY(EditAnywhere, Category = "Parameter")
    FMythicaParameterFile ValueFile;
};

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameter")
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

    void ReadParameters(const TSharedPtr<FJsonObject>& ParamsSchema, FMythicaParameters& OutParameters);
    void WriteParameters(const TArray<FString>& InputFileIds, const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet);
}
