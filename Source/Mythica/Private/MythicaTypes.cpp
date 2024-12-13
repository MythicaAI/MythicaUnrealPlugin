#pragma once

#include "MythicaTypes.h"

const TCHAR* SystemParameters[] =
{
    TEXT("format")
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
                FString FileId = InputFileIds.IsValidIndex(i) ? InputFileIds[i] : FString();

                TSharedPtr<FJsonObject> FileObject = MakeShareable(new FJsonObject);
                FileObject->SetStringField(TEXT("file_id"), FileId);

                OutParamsSet->SetObjectField(Param.Name, FileObject);
                break;
        }
    }
}