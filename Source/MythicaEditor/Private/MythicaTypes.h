#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveLinearColor.h"
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

UENUM(BlueprintType)
enum class EMythicaParameterType : uint8
{
    Int,
    Float,
    Bool,
    String,
    Enum,
    File,
    Curve
};

USTRUCT(BlueprintType)
struct FMythicaParameterInt
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    TArray<int> Values;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TArray<int> DefaultValues;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TOptional<int> MinValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TOptional<int> MaxValue;

    bool IsDefault() const;
    bool Validate(const TArray<int>& Value) const;
    void Copy(const FMythicaParameterInt& Source);
};

USTRUCT(BlueprintType)
struct FMythicaParameterFloat
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    TArray<float> Values;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TArray<float> DefaultValues;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TOptional<float> MinValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TOptional<float> MaxValue;

    bool IsDefault() const;
    bool Validate(const TArray<float>& Value) const;
    void Copy(const FMythicaParameterFloat& Source);
};

USTRUCT(BlueprintType)
struct FMythicaParameterBool
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    bool Value = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    bool DefaultValue = false;

    bool IsDefault() const;
    void Copy(const FMythicaParameterBool& Source);
};

USTRUCT(BlueprintType)
struct FMythicaParameterString
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString Value;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    FString DefaultValue;

    bool IsDefault() const;
    void Copy(const FMythicaParameterString& Source);
};

USTRUCT(BlueprintType)
struct FMythicaParameterEnumValue
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    FString Name;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    FString Label;
};

USTRUCT(BlueprintType)
struct FMythicaParameterEnum
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString Value;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    FString DefaultValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    TArray<FMythicaParameterEnumValue> Values;

    bool IsDefault() const;
    bool Validate(const FString& Value) const;
    void Copy(const FMythicaParameterEnum& Source);
};

USTRUCT(BlueprintType)
struct FMythicaParameterFileSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMythicaExportTransformType TransformType = EMythicaExportTransformType::Relative;
};

USTRUCT(BlueprintType)
struct FMythicaParameterFile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMythicaInputType Type = EMythicaInputType::World;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::Mesh", EditConditionHides))
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::World", EditConditionHides))
    TArray<AActor*> Actors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::Spline", EditConditionHides))
    AActor* SplineActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::Volume", EditConditionHides))
    AMythicaInputSelectionVolume* VolumeActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type != EMythicaInputType::Mesh", EditConditionHides))
    FMythicaParameterFileSettings Settings;

    void Copy(const FMythicaParameterFile& Source);
};

UENUM(BlueprintType)
enum class EMythicaCurveType : uint8
{
    MCT_Invalid = 0     UMETA(Hidden),
    MCT_Float = 1       UMETA(DisplayName = "Float"),
    MCT_Vector = 2      UMETA(DisplayName = "Vector"),
    MCT_Color = 3       UMETA(DisplayName = "Color")
};

UENUM(BlueprintType)
enum class EMythicaCurveInterpolationType : uint8
{
    MCIT_Invalid = 0        UMETA(Hidden),
    MCIT_Constant = 1       UMETA(DisplayName = "Constant"),
    MCIT_Linear = 2         UMETA(DisplayName = "Linear"),
    MCIT_Catmull_Rom = 3    UMETA(DisplayName = "Catmull_Rom"),
    MCIT_Monotone_Cubic = 4 UMETA(DisplayName = "Monotone_Cubic"),
    MCIT_Bezier = 5         UMETA(DisplayName = "Bezier"),
    MCIT_BSpline = 6        UMETA(DisplayName = "BSpline"),
    MCIT_Hermite = 7        UMETA(DisplayName = "Hermite")
};

USTRUCT(BlueprintType)
struct FMythicaParameterCurve
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMythicaCurveType Type = EMythicaCurveType::MCT_Invalid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaCurveType::MCT_Float", EditConditionHides))
    TObjectPtr<UCurveFloat> FloatCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::MCT_Vector", EditConditionHides))
    TObjectPtr<UCurveVector> VectorCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "Type == EMythicaInputType::MCT_Color", EditConditionHides))
    TObjectPtr<UCurveLinearColor> ColorCurve = nullptr;

public:

    FMythicaParameterCurve() = default;
    FMythicaParameterCurve(EMythicaCurveType InType);
    ~FMythicaParameterCurve();

    void Copy(const FMythicaParameterCurve& Source);
    bool IsDataValid();

};

USTRUCT(BlueprintType)
struct FMythicaParameter
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    FString Label;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Config")
    EMythicaParameterType Type = EMythicaParameterType::Int;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FMythicaParameterInt ValueInt;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FMythicaParameterFloat ValueFloat;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FMythicaParameterBool ValueBool;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FMythicaParameterString ValueString;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FMythicaParameterEnum ValueEnum;
 
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMythicaParameterFile ValueFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMythicaParameterCurve ValueCurve;

};

USTRUCT(BlueprintType)
struct FMythicaParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMythicaParameter> Parameters;
};

USTRUCT(BlueprintType)
struct FMythicaMaterialParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInterface* Material;
};

namespace Mythica
{
    bool IsSystemParameter(const FString& Name);

    void ReadParameters(const TSharedPtr<FJsonObject>& ParamsSchema, FMythicaParameters& OutParameters);
    void WriteParameters(const TArray<FString>& InputFileIds, const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet);
    void CopyParameterValues(const FMythicaParameters& Source, FMythicaParameters& Target);
}
