#pragma once

#include "MythicaTypes.h"

void Mythica::ReadParameters(const TSharedPtr<FJsonObject>& ParameterDef, FMythicaParameters& OutParameters)
{
    for (auto It = ParameterDef->Values.CreateConstIterator(); It; ++It)
    {
        const FString& Name = It.Key();
        TSharedPtr<FJsonObject> ParameterObject = It.Value()->AsObject();

        FString Label = ParameterObject->GetStringField(TEXT("label"));
        FString Type = ParameterObject->GetStringField(TEXT("type"));
        bool IsArray = ParameterObject->HasTypedField<EJson::Array>(TEXT("default"));

        FMythicaParameterValue Value;
        if (Type == "String")
        {
            FString DefaultValue = ParameterObject->GetStringField(TEXT("default"));
            Value.Emplace<FMythicaParameterString>(DefaultValue, DefaultValue);
        }
        else if (Type == "Toggle")
        {
            bool DefaultValue = ParameterObject->GetBoolField(TEXT("default"));
            Value.Emplace<FMythicaParameterBool>(DefaultValue, DefaultValue);
        }
        else if (Type == "Int")
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

            Value.Emplace<FMythicaParameterInt>(DefaultValues, DefaultValues);
        }
        else if (Type == "Float")
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

            Value.Emplace<FMythicaParameterFloat>(DefaultValues, DefaultValues);
        }
        else
        {
            continue;
        }

        OutParameters.Parameters.Add({ Name, Label, Value });
    }
}

void Mythica::WriteParameters(const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& ParameterSet)
{
    for (const FMythicaParameter& Param : Parameters.Parameters)
    {
        if (const FMythicaParameterString* StringParam = Param.Value.TryGet<FMythicaParameterString>())
        {
            ParameterSet->SetStringField(Param.Name, StringParam->Value);
        }            
        else if (const FMythicaParameterBool* BoolParam = Param.Value.TryGet<FMythicaParameterBool>())
        {
            ParameterSet->SetBoolField(Param.Name, BoolParam->Value);
        }
        else if (const FMythicaParameterInt* IntParam = Param.Value.TryGet<FMythicaParameterInt>())
        {
            if (IntParam->Values.Num() == 1)
            {
                ParameterSet->SetNumberField(Param.Name, IntParam->Values[0]);
            }
            else
            {
                TArray<TSharedPtr<FJsonValue>> Array;
                for (int Value : IntParam->Values)
                {
                    Array.Add(MakeShareable(new FJsonValueNumber(Value)));
                }
                ParameterSet->SetArrayField(Param.Name, Array);
            }
        }
        else if (const FMythicaParameterFloat* FloatParam = Param.Value.TryGet<FMythicaParameterFloat>())
        {
            if (FloatParam->Values.Num() == 1)
            {
                ParameterSet->SetNumberField(Param.Name, FloatParam->Values[0]);
            }
            else
            {
                TArray<TSharedPtr<FJsonValue>> Array;
                for (float Value : FloatParam->Values)
                {
                    Array.Add(MakeShareable(new FJsonValueNumber(Value)));
                }
                ParameterSet->SetArrayField(Param.Name, Array);
            }
        }
    }
}