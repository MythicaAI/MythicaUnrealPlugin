#pragma once

#include "MythicaTypes.h"

void Mythica::ReadInputs(const TArray<TSharedPtr<FJsonValue>>& InputsDef, FMythicaInputs& OutInputs)
{
    for (TSharedPtr<FJsonValue> InputValue : InputsDef)
    {
        FString InputLabel = InputValue->AsString();
        OutInputs.Inputs.Add({ InputLabel });
    }
}

void Mythica::ReadParameters(const TSharedPtr<FJsonObject>& ParameterDef, FMythicaParameters& OutParameters)
{
    for (auto It = ParameterDef->Values.CreateConstIterator(); It; ++It)
    {
        const FString& Name = It.Key();
        TSharedPtr<FJsonObject> ParameterObject = It.Value()->AsObject();

        FMythicaParameter Parameter;
        Parameter.Name = Name;
        Parameter.Label = ParameterObject->GetStringField(TEXT("label"));;

        FString Type = ParameterObject->GetStringField(TEXT("type"));
        bool IsArray = ParameterObject->HasTypedField<EJson::Array>(TEXT("default"));
        if (Type == "Int")
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
        else if (Type == "Toggle")
        {
            bool DefaultValue = ParameterObject->GetBoolField(TEXT("default"));
            Parameter.Type = EMythicaParameterType::Bool;
            Parameter.ValueBool = FMythicaParameterBool{ DefaultValue, DefaultValue };
        }
        else if (Type == "String")
        {
            FString DefaultValue = ParameterObject->GetStringField(TEXT("default"));
            Parameter.Type = EMythicaParameterType::String;
            Parameter.ValueString = FMythicaParameterString{ DefaultValue, DefaultValue };
        }
        else
        {
            continue;
        }

        OutParameters.Parameters.Add(Parameter);
    }
}

void Mythica::WriteParameters(const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& OutParamsSet)
{
    for (const FMythicaParameter& Param : Parameters.Parameters)
    {
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
        }
    }
}