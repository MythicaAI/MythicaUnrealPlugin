#pragma once

#include "MythicaTypes.h"

#include "Dom/JsonObject.h"

#include "MythicaEditorPrivatePCH.h"

bool FMythicaParameterInt::IsDefault() const
{
    return Values == DefaultValues;
}

bool FMythicaParameterInt::Validate(const TArray<int>& InValue) const
{
    if (InValue.Num() != DefaultValues.Num())
    {
        return false;
    }

    for (int i = 0; i < InValue.Num(); ++i)
    {
        if (MinValue.IsSet() && InValue[i] < MinValue.GetValue())
        {
            return false;
        }
        if (MaxValue.IsSet() && InValue[i] > MaxValue.GetValue())
        {
            return false;
        }
    }
    
    return true;
}

void FMythicaParameterInt::Copy(const FMythicaParameterInt& Source)
{
    if (!Source.IsDefault() && Validate(Source.Values))
    {
        Values = Source.Values;
    }
}

bool FMythicaParameterFloat::IsDefault() const
{
    return Values == DefaultValues;
}

bool FMythicaParameterFloat::Validate(const TArray<float>& InValue) const
{
    if (InValue.Num() != DefaultValues.Num())
    {
        return false;
    }

    for (int i = 0; i < InValue.Num(); ++i)
    {
        if (MinValue.IsSet() && InValue[i] < MinValue.GetValue())
        {
            return false;
        }
        if (MaxValue.IsSet() && InValue[i] > MaxValue.GetValue())
        {
            return false;
        }
    }

    return true;
}

void FMythicaParameterFloat::Copy(const FMythicaParameterFloat& Source)
{
    if (!Source.IsDefault() && Validate(Source.Values))
    {
        Values = Source.Values;
    }
}

bool FMythicaParameterBool::IsDefault() const
{
    return Value == DefaultValue;
}

void FMythicaParameterBool::Copy(const FMythicaParameterBool& Source)
{
    if (!Source.IsDefault())
    {
        Value = Source.Value;
    }
}

bool FMythicaParameterString::IsDefault() const
{
    return Value == DefaultValue;
}

void FMythicaParameterString::Copy(const FMythicaParameterString& Source)
{
    if (!Source.IsDefault())
    {
        Value = Source.Value;
    }
}

bool FMythicaParameterEnum::IsDefault() const
{
    return Value == DefaultValue;
}

bool FMythicaParameterEnum::Validate(const FString& InValue) const
{
    for (const FMythicaParameterEnumValue& EnumValue : Values)
    {
        if (EnumValue.Name == InValue)
        {
            return true;
        }
    }

    return false;
}   

void FMythicaParameterEnum::Copy(const FMythicaParameterEnum& Source)
{
    if (!Source.IsDefault() && Validate(Source.Value))
    {
        Value = Source.Value;
    }
}

void FMythicaParameterFile::Copy(const FMythicaParameterFile& Source)
{
    *this = Source;
}

//FMythicaParameterCurve::FMythicaParameterCurve(EMythicaCurveType InType)
//{
//    Type = InType;
//
//    switch (Type)
//    {
//    case EMythicaCurveType::MCT_Float:
//    {
//        if (!IsValid(FloatCurve))
//        {
//            FloatCurve = NewObject<UCurveFloat>(
//                GetTransientPackage(),
//                UCurveFloat::StaticClass(),
//                NAME_None,
//                RF_Transactional | RF_Public);
//            ensure(FloatCurve);
//
//            FloatCurve->AddToRoot();
//        }
//        break;
//    }
//    case EMythicaCurveType::MCT_Vector:
//    {
//        if (!IsValid(VectorCurve))
//        {
//            VectorCurve = NewObject<UCurveVector>(
//                GetTransientPackage(),
//                UCurveVector::StaticClass(),
//                NAME_None,
//                RF_Transactional | RF_Public);
//            ensure(VectorCurve);
//
//            VectorCurve->AddToRoot();
//        }
//        break;
//    }
//    case EMythicaCurveType::MCT_Color:
//    {
//        if (!IsValid(ColorCurve))
//        {
//            ColorCurve = NewObject<UCurveLinearColor>(
//                GetTransientPackage(),
//                UCurveLinearColor::StaticClass(),
//                NAME_None,
//                RF_Transactional | RF_Public);
//            ensure(ColorCurve);
//
//            ColorCurve->AddToRoot();
//        }
//        break;
//    }
//    case EMythicaCurveType::MCT_Invalid:
//    default:
//        UE_LOG(LogTemp, Warning, TEXT("An invalid MythicaParameterCurve has been instaniated."));
//        break;
//    }
//}

//FMythicaParameterCurve::~FMythicaParameterCurve()
//{
//    if (IsValid(FloatCurve))
//    {
//        FloatCurve->RemoveFromRoot();
//        FloatCurve = nullptr;
//    }
//    
//    if (IsValid(VectorCurve))
//    {
//        VectorCurve->RemoveFromRoot();
//        VectorCurve = nullptr;
//    }
//    
//    if (IsValid(ColorCurve))
//    {
//        ColorCurve->RemoveFromRoot();
//        ColorCurve = nullptr;
//    }
//}

void FMythicaParameterCurve::Copy(const FMythicaParameterCurve& Source)
{
    *this = Source;
}

bool FMythicaParameterCurve::IsDataValid()
{
    return Type != EMythicaCurveType::MCT_Invalid; /*&&
        ((Type == EMythicaCurveType::MCT_Float && IsValid(FloatCurve)) ||
        (Type == EMythicaCurveType::MCT_Color && IsValid(ColorCurve)) ||
        (Type == EMythicaCurveType::MCT_Vector && IsValid(VectorCurve)));*/
}

const TCHAR* SystemParameters[] =
{
    TEXT("format"),
    TEXT("record_profile")
};

bool Mythica::IsSystemParameter(const FString& Name)
{
    for (const TCHAR* Parameter : SystemParameters)
    {
        if (Name == Parameter)
        {
            return true;
        }
    }
    return false;
}

void Mythica::ReadParameters(const TSharedPtr<FJsonObject>& ParamsSchema, FMythicaParameters& OutParameters)
{
    for (auto It = ParamsSchema->Values.CreateConstIterator(); It; ++It)
    {
        const FString& Name = It.Key();
        TSharedPtr<FJsonObject> ParameterObject = It.Value()->AsObject();

        bool Constant = false;
        ParameterObject->TryGetBoolField(TEXT("constant"), Constant);
        if (Constant)
        {
            continue; 
        }

        FString Label = ParameterObject->GetStringField(TEXT("label"));

        // Handle Parameters
        FMythicaParameter Parameter;
        Parameter.Name = Name;
        Parameter.Label = Label;

        FString Type = ParameterObject->GetStringField(TEXT("param_type"));
        bool IsArray = ParameterObject->HasTypedField<EJson::Array>(TEXT("default"));

        if (Type == "int")
        {
            TArray<int> DefaultValues;
            if (IsArray)
            {
                TArray<TSharedPtr<FJsonValue>> DefaultArray = ParameterObject->GetArrayField(TEXT("default"));
                for (TSharedPtr<FJsonValue> DefaultValue : DefaultArray)
                {
                    DefaultValues.Add(DefaultValue->AsNumber());
                }
            }
            else
            {
                DefaultValues.Add(ParameterObject->GetNumberField(TEXT("default")));
            }

            TOptional<int> MinValue;
            TOptional<int> MaxValue;
            if (ParameterObject->HasField(TEXT("min")))
            {
                MinValue = ParameterObject->GetNumberField(TEXT("min"));
            }
            if (ParameterObject->HasField(TEXT("max")))
            {
                MaxValue = ParameterObject->GetNumberField(TEXT("max"));
            }

            Parameter.Type = EMythicaParameterType::Int;
            Parameter.ValueInt = FMythicaParameterInt{ DefaultValues, DefaultValues, MinValue, MaxValue };
        }
        else if (Type == "float")
        {
            TArray<float> DefaultValues;
            if (IsArray)
            {
                TArray<TSharedPtr<FJsonValue>> DefaultArray = ParameterObject->GetArrayField(TEXT("default"));
                for (TSharedPtr<FJsonValue> DefaultValue : DefaultArray)
                {
                    DefaultValues.Add(DefaultValue->AsNumber());
                }
            }
            else
            {
                DefaultValues.Add(ParameterObject->GetNumberField(TEXT("default")));
            }

            TOptional<float> MinValue;
            TOptional<float> MaxValue;
            if (ParameterObject->HasField(TEXT("min")))
            {
                MinValue = ParameterObject->GetNumberField(TEXT("min"));
            }
            if (ParameterObject->HasField(TEXT("max")))
            {
                MaxValue = ParameterObject->GetNumberField(TEXT("max"));
            }

            Parameter.Type = EMythicaParameterType::Float;
            Parameter.ValueFloat = FMythicaParameterFloat{ DefaultValues, DefaultValues, MinValue, MaxValue };
        }
        else if (Type == "bool")
        {
            bool DefaultValue = ParameterObject->GetBoolField(TEXT("default"));
            Parameter.Type = EMythicaParameterType::Bool;
            Parameter.ValueBool = FMythicaParameterBool{ DefaultValue, DefaultValue };
        }
        else if (Type == "string")
        {
            FString DefaultValue = ParameterObject->GetStringField(TEXT("default"));
            Parameter.Type = EMythicaParameterType::String;
            Parameter.ValueString = FMythicaParameterString{ DefaultValue, DefaultValue };
        }
        else if (Type == "enum")
        {
            FString DefaultValue = ParameterObject->GetStringField(TEXT("default"));

            TArray<FMythicaParameterEnumValue> Values;

            TArray<TSharedPtr<FJsonValue>> ValuesArray = ParameterObject->GetArrayField(TEXT("values"));
            for (TSharedPtr<FJsonValue> Value : ValuesArray)
            {
                TSharedPtr<FJsonObject> ValueObject = Value->AsObject();
                FString ValueName = ValueObject->GetStringField(TEXT("name"));
                FString ValueLabel = ValueObject->GetStringField(TEXT("label"));
                Values.Add({ ValueName, ValueLabel });
            }

            Parameter.Type = EMythicaParameterType::Enum;
            Parameter.ValueEnum = FMythicaParameterEnum{ DefaultValue, DefaultValue, Values };
        }
        else if (Type == "file")
        {
            Parameter.Type = EMythicaParameterType::File;
            Parameter.ValueFile = FMythicaParameterFile{};
        }
        else if (Type == "ramp")
        {
            Parameter.Type = EMythicaParameterType::Curve;

            FString RampType = ParameterObject->GetStringField(TEXT("ramp_parm_type"));
            UE_LOG(LogTemp, Warning, TEXT("- Param: %s\n - Label: %s\n - Curve Type: %s\nDefault:"), *Parameter.Name, *Parameter.Label, *RampType);

            if (RampType.Contains(TEXT("Float")))
            {
                Parameter.ValueCurve = FMythicaParameterCurve{ EMythicaCurveType::MCT_Float };
            }
            else if (RampType.Contains(TEXT("Color")))
            {
                Parameter.ValueCurve = FMythicaParameterCurve{ EMythicaCurveType::MCT_Color };
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Param [%s] Contained an unsupported Curve Type {%s}"), *Parameter.Name, *RampType);
                check(false);
                continue;
            }

            UE_LOG(LogTemp, Warning, TEXT("The content %s"), (Parameter.ValueCurve.IsDataValid() ? TEXT("is valid") : TEXT("is NOT valid")));

            TArray<TSharedPtr<FJsonValue>> ValuesArray = ParameterObject->GetArrayField(TEXT("default"));
            for (TSharedPtr<FJsonValue> Point : ValuesArray)
            {
                TSharedPtr<FJsonObject> PointObject = Point->AsObject();

                float Pos = PointObject->GetNumberField(TEXT("pos"));
                FString InterpType = PointObject->GetStringField(TEXT("interp"));

                //ERichCurveInterpMode InterpMode;
                //if (InterpType == TEXT("Linear"))
                //{
                //    InterpMode = ERichCurveInterpMode::RCIM_Linear;
                //}
                //else if (InterpType == TEXT("Constant"))
                //{
                //    InterpMode = ERichCurveInterpMode::RCIM_Constant;
                //}
                //else if (InterpType == TEXT("Bezier") || InterpType == TEXT("BSpline")
                //    || InterpType == TEXT("CatmullRom") || InterpType == TEXT("Hermite") || InterpType == TEXT("MonotoneCubic"))
                //{
                //    InterpMode = ERichCurveInterpMode::RCIM_Cubic;
                //}

                EMythicaCurveInterpolationType InterpMode;
                if (InterpType == TEXT("Linear"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Linear;
                }
                else if (InterpType == TEXT("Constant"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Constant;
                }
                else if (InterpType == TEXT("Bezier"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Bezier;
                }
                else if (InterpType == TEXT("BSpline"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_BSpline;
                }
                else if (InterpType == TEXT("CatmullRom"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Catmull_Rom;
                }
                else if (InterpType == TEXT("Hermite"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Hermite;
                }
                else if (InterpType == TEXT("MonotoneCubic"))
                {
                    InterpMode = EMythicaCurveInterpolationType::MCIT_Monotone_Cubic;
                }


                switch (Parameter.ValueCurve.Type)
                {
                case EMythicaCurveType::MCT_Float:
                {
                    float FloatValue = PointObject->GetNumberField(TEXT("value"));
                    UE_LOG(LogTemp, Warning, TEXT("\t{%f, %f} - %s"), Pos, FloatValue, *InterpType);

                    //FKeyHandle Key = Parameter.ValueCurve.FloatCurve->FloatCurve.AddKey(Pos, FloatValue);
                    //Parameter.ValueCurve.FloatCurve->FloatCurve.SetKeyInterpMode(Key, InterpMode);
                    Parameter.ValueCurve.Points.Emplace(FMythicaCurvePoint(Pos, FloatValue, FLinearColor::Black, FVector::ZeroVector, InterpMode));
                    break;
                }
                case EMythicaCurveType::MCT_Color:
                {
                    TArray<TSharedPtr<FJsonValue>> ColorArray = PointObject->GetArrayField(TEXT("c"));
                    ensure(ColorArray.Num() == 3);

                    float R = ColorArray[0]->AsNumber();
                    float G = ColorArray[1]->AsNumber();
                    float B = ColorArray[2]->AsNumber();

                    FLinearColor Color = FLinearColor{ R, G, B };
                    UE_LOG(LogTemp, Warning, TEXT("\t{%f, %s} - %s"), Pos, *Color.ToString(), *InterpType);

                    Parameter.ValueCurve.Points.Emplace(FMythicaCurvePoint(Pos, 0.0, Color, FVector::ZeroVector, InterpMode));

                    //FKeyHandle RKey = Parameter.ValueCurve.ColorCurve->FloatCurves[0].AddKey(Pos, R);
                    //FKeyHandle GKey = Parameter.ValueCurve.ColorCurve->FloatCurves[1].AddKey(Pos, G);
                    //FKeyHandle BKey = Parameter.ValueCurve.ColorCurve->FloatCurves[2].AddKey(Pos, B);

                    //Parameter.ValueCurve.ColorCurve->FloatCurves[0].SetKeyInterpMode(RKey, InterpMode);
                    //Parameter.ValueCurve.ColorCurve->FloatCurves[1].SetKeyInterpMode(GKey, InterpMode);
                    //Parameter.ValueCurve.ColorCurve->FloatCurves[2].SetKeyInterpMode(BKey, InterpMode);

                    break;
                }
                case EMythicaCurveType::MCT_Vector:
                    break;
                }
            }

        }
        else
        {
            continue;
        }

        OutParameters.Parameters.Add(Parameter);
    }
}

void Mythica::WriteParameters(const TArray<FString>& InputFileIds, const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& OutParamsSet)
{
    for (int i = 0; i < Parameters.Parameters.Num(); ++i)
    {
        const FMythicaParameter& Param = Parameters.Parameters[i];
        switch (Param.Type)
        {
            case EMythicaParameterType::Int:
                if (Param.ValueInt.Values.Num() == 1)
                {
                    OutParamsSet->SetNumberField(Param.Name, Param.ValueInt.Values[0]);
                }
                else
                {
                    TArray<TSharedPtr<FJsonValue>> Array;
                    for (int Value : Param.ValueInt.Values)
                    {
                        Array.Add(MakeShareable(new FJsonValueNumber(Value)));
                    }
                    OutParamsSet->SetArrayField(Param.Name, Array);
                }
                break;

            case EMythicaParameterType::Float:
                if (Param.ValueFloat.Values.Num() == 1)
                {
                    OutParamsSet->SetNumberField(Param.Name, Param.ValueFloat.Values[0]);
                }
                else
                {
                    TArray<TSharedPtr<FJsonValue>> Array;
                    for (float Value : Param.ValueFloat.Values)
                    {
                        Array.Add(MakeShareable(new FJsonValueNumber(Value)));
                    }
                    OutParamsSet->SetArrayField(Param.Name, Array);
                }
                break;

            case EMythicaParameterType::Bool:
                OutParamsSet->SetBoolField(Param.Name, Param.ValueBool.Value);
                break;

            case EMythicaParameterType::String:
                OutParamsSet->SetStringField(Param.Name, Param.ValueString.Value);
                break;

            case EMythicaParameterType::Enum:
                OutParamsSet->SetStringField(Param.Name, Param.ValueEnum.Value);
                break;

            case EMythicaParameterType::File:
            {
                FString FileId = InputFileIds.IsValidIndex(i) ? InputFileIds[i] : FString();

                TSharedPtr<FJsonObject> FileObject = MakeShareable(new FJsonObject());
                FileObject->SetStringField(TEXT("file_id"), FileId);

                OutParamsSet->SetObjectField(Param.Name, FileObject);
                break;
            }
            case EMythicaParameterType::Curve:
                OutParamsSet->SetStringField(Param.Name, Param.ValueEnum.Value);
                break;
        }
    }
}

void Mythica::CopyParameterValues(const FMythicaParameters& Source, FMythicaParameters& Target)
{
    for (const FMythicaParameter& SourceParam : Source.Parameters)
    {
        if (Mythica::IsSystemParameter(SourceParam.Name))
        {
            continue;
        }

        auto MatchParameter = [&](const FMythicaParameter& P) 
        { 
            return P.Name == SourceParam.Name && P.Type == SourceParam.Type; 
        };
        FMythicaParameter* TargetParam = Target.Parameters.FindByPredicate(MatchParameter);
        if (!TargetParam)
        {
            continue;
        }

        switch (SourceParam.Type)
        {
            case EMythicaParameterType::Int:
            {
                TargetParam->ValueInt.Copy(SourceParam.ValueInt);
                break;
            }
            case EMythicaParameterType::Float:
            {
                TargetParam->ValueFloat.Copy(SourceParam.ValueFloat);
                break;
            }
            case EMythicaParameterType::Bool:
            {
                TargetParam->ValueBool.Copy(SourceParam.ValueBool);
                break;
            }
            case EMythicaParameterType::String:
            {
                TargetParam->ValueString.Copy(SourceParam.ValueString);
                break;
            }
            case EMythicaParameterType::Enum:
            {
                TargetParam->ValueEnum.Copy(SourceParam.ValueEnum);
                break;
            }
            case EMythicaParameterType::File:
            {
                TargetParam->ValueFile.Copy(SourceParam.ValueFile);
                break;
            }
            case EMythicaParameterType::Curve:
            {
                TargetParam->ValueCurve.Copy(SourceParam.ValueCurve);
                break;
            }
        }
    }
}
