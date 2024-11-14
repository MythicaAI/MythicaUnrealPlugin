#pragma once

#include "MythicaTypes.h"

void Mythica::ReadParameters(const TSharedPtr<FJsonObject>& ParamsSchema, FMythicaInputs& OutInputs, FMythicaParameters& OutParameters)
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

        // Handle Inputs
        FRegexPattern Pattern(TEXT("^input(\\d+)$"));
        FRegexMatcher Matcher(Pattern, Name);

        if (Matcher.FindNext())
        {
            FMythicaInput Input;
            Input.Label = Label;

            int InputIndex = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutInputs.Inputs.SetNum(InputIndex + 1, EAllowShrinking::No);
            OutInputs.Inputs[InputIndex] = Input;

            continue;
        }

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
        else
        {
            continue;
        }

        OutParameters.Parameters.Add(Parameter);
    }
}

void Mythica::WriteParameters(const FMythicaInputs& Inputs, const TArray<FString>& InputFileIds, const FMythicaParameters& Parameters, const TSharedPtr<FJsonObject>& OutParamsSet)
{
    for (int i = 0; i < Inputs.Inputs.Num(); ++i)
    {
        const FMythicaInput& Input = Inputs.Inputs[i];
        
        FString FileId = InputFileIds.IsValidIndex(i) ? InputFileIds[i] : FString();

        TSharedPtr<FJsonObject> FileObject = MakeShareable(new FJsonObject);
        FileObject->SetStringField(TEXT("file_id"), FileId);

        OutParamsSet->SetObjectField(FString::Printf(TEXT("input%d"), i), FileObject);
    }

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

            case EMythicaParameterType::Enum:
                OutParamsSet->SetStringField(Param.Name, Param.ValueEnum.Value);
                break;
        }
    }
}